#include "payroll.h"
#include "employee.h"
#include "attendance.h"
#include "date_utils.h"
#include "storage.h"
#include <string.h>
#include <limits.h>

int findInputIndex(const AppData *app, const char *id, int month, int year) {
    for (size_t index = 0; index < app->inputCount; index++) {
        if (app->inputs[index].month == month && app->inputs[index].year == year &&
            !strcmp(app->inputs[index].employeeId, id)) {
            return (int)index;
        }
    }
    return -1;
}

Result savePayrollInput(AppData *app, const PayrollInput *input) {
    int employeeIndex = findEmployeeIndex(app, input->employeeId), inputIndex;
    if (!validMonth(input->month, input->year) || employeeIndex < 0 ||
        !employeeInMonth(employeeAt(app, employeeIndex), input->month, input->year)) {
        return appError(app, RESULT_INVALID, "Nhân viên không thuộc kỳ lương.");
    }
    if (isLocked(app, input->month, input->year)) {
        return appError(app, RESULT_LOCKED, "Tháng đã chốt.");
    }
    if (input->baseSalary <= 0 || input->baseSalary > MAX_MONEY || input->allowance < 0 ||
        input->allowance > MAX_MONEY || input->bonus < 0 || input->bonus > MAX_MONEY ||
        input->deduction < 0 || input->deduction > MAX_MONEY) {
        return appError(app, RESULT_INVALID, "Thông số tiền không hợp lệ.");
    }
    inputIndex = findInputIndex(app, input->employeeId, input->month, input->year);
    if (inputIndex < 0) {
        if (!reserveArray((void **)&app->inputs, &app->inputCap, app->inputCount + 1,
                          sizeof(PayrollInput))) {
            return appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ.");
        }
        inputIndex = (int)app->inputCount++;
    }
    app->inputs[inputIndex] = *input;
    app->dirty = 1;
    return RESULT_OK;
}

Result ensurePayrollInputs(AppData *app, int month, int year) {
    if (!validMonth(month, year)) {
        return appError(app, RESULT_INVALID, "Kỳ lương không hợp lệ.");
    }
    if (isLocked(app, month, year)) {
        return RESULT_OK;
    }
    /* Duyệt lần lượt các nút; không truy cập danh sách như một mảng. */
    for (EmployeeNode *node = app->employeeHead; node != NULL; node = node->next) {
        const Employee *employee = &node->employee;
        if (!employeeInMonth(employee, month, year)) {
            continue;
        }
        if (findInputIndex(app, employee->id, month, year) >= 0) {
            continue;
        }
        PayrollInput input = {0};
        strcpy(input.employeeId, employee->id);
        input.month = month;
        input.year = year;
        input.baseSalary = employee->baseSalary;
        input.allowance = employee->allowance;
        Result result = savePayrollInput(app, &input);
        if (result != RESULT_OK) {
            return result;
        }
    }
    return RESULT_OK;
}

/* Chia và làm tròn bằng số nguyên: không dùng float cho tiền. */
static long long roundDiv(long long numerator, long long denominator) {
    long long quotient = numerator / denominator;
    long long remainder = numerator % denominator;
    if (remainder >= (denominator + 1) / 2) {
        quotient++;
    }
    return quotient;
}

/*
 * paidUnits: tổng đơn vị nửa giờ hưởng lương; 16 đơn vị = 1 ngày công.
 * overtimeUnits: tổng đơn vị nửa giờ tăng ca; 2 đơn vị = 1 giờ.
 * Ví dụ URD: 384 đơn vị hưởng + 20 đơn vị tăng ca -> 14.437.500 đồng.
 */
Result calculatePayroll(const PayrollInput *input, int standardDays, int paidUnits,
                        int overtimeUnits, Payroll *payroll) {
    if (!payroll || !input || standardDays < 1 || standardDays > 31 || paidUnits < 0 ||
        paidUnits > standardDays * 16 || overtimeUnits < 0 || overtimeUnits > standardDays * 8 ||
        input->baseSalary <= 0 || input->baseSalary > MAX_MONEY || input->allowance < 0 ||
        input->allowance > MAX_MONEY || input->bonus < 0 || input->bonus > MAX_MONEY ||
        input->deduction < 0 || input->deduction > MAX_MONEY) {
        return RESULT_INVALID;
    }
    if (input->baseSalary > LLONG_MAX / ((long long)overtimeUnits * 3 + 1) ||
        input->baseSalary > LLONG_MAX / (paidUnits + 1LL) ||
        input->allowance > LLONG_MAX / (paidUnits + 1LL)) {
        return RESULT_INVALID;
    }
    memset(payroll, 0, sizeof *payroll);
    strcpy(payroll->employeeId, input->employeeId);
    payroll->month = input->month;
    payroll->year = input->year;
    payroll->standardDays = standardDays;
    payroll->paidHalfHours = paidUnits;
    payroll->overtimeHalfHours = overtimeUnits;
    payroll->baseSalary = input->baseSalary;
    payroll->allowance = input->allowance;
    payroll->bonus = input->bonus;
    payroll->deduction = input->deduction;
    /* Mẫu số 16 quy đổi ngày thành nửa giờ; 3/2 là hệ số tăng ca 150%. */
    payroll->regularPay = roundDiv(input->baseSalary * paidUnits, standardDays * 16LL);
    payroll->allowancePay = roundDiv(input->allowance * paidUnits, standardDays * 16LL);
    payroll->overtimePay = roundDiv(input->baseSalary * overtimeUnits * 3, standardDays * 32LL);
    payroll->netPay = payroll->regularPay + payroll->allowancePay + payroll->overtimePay +
                      payroll->bonus - payroll->deduction;
    if (payroll->netPay < 0) {
        return RESULT_INVALID;
    }
    return RESULT_OK;
}

Result buildPayroll(AppData *app, int month, int year, Payroll rows[MAX_EMPLOYEES], size_t *count) {
    int paidUnits[MAX_EMPLOYEES] = {0}, overtimeUnits[MAX_EMPLOYEES] = {0};
    Result result;
    *count = 0;
    if (!validMonth(month, year)) {
        return appError(app, RESULT_INVALID, "Kỳ lương không hợp lệ.");
    }
    if (isLocked(app, month, year)) {
        for (size_t index = 0; index < app->payrollCount; index++) {
            if (app->payrolls[index].month == month && app->payrolls[index].year == year) {
                rows[(*count)++] = app->payrolls[index];
            }
        }
        return RESULT_OK;
    }
    if (missingAttendance(app, month, year, NULL, NULL)) {
        return appError(app, RESULT_INVALID, "Còn thiếu chấm công; xem chức năng kiểm tra thiếu.");
    }
    result = ensurePayrollInputs(app, month, year);
    if (result != RESULT_OK) {
        return result;
    }
    for (size_t index = 0; index < app->attendanceCount; index++) {
        Attendance *payroll = &app->attendance[index];
        if (payroll->date.month != month || payroll->date.year != year) {
            continue;
        }
        int employee = findEmployeeIndex(app, payroll->employeeId);
        if (employee < 0) {
            return appError(app, RESULT_INVALID, "Công tham chiếu mã không tồn tại.");
        }
        paidUnits[employee] += payroll->type == ATT_PAID_LEAVE ? 16 : payroll->regularHalfHours;
        overtimeUnits[employee] += payroll->overtimeHalfHours;
    }
    for (size_t index = 0; index < app->employeeCount; index++) {
        if (employeeInMonth(employeeAt(app, index), month, year)) {
            Employee *employee = employeeAt(app, index);
            int inputIndex = findInputIndex(app, employee->id, month, year);
            result = calculatePayroll(&app->inputs[inputIndex], workingDaysInMonth(month, year),
                                      paidUnits[index], overtimeUnits[index], &rows[*count]);
            if (result != RESULT_OK) {
                return appError(app, RESULT_INVALID,
                                "Lương %s không hợp lệ hoặc thực lĩnh âm; sửa thông số tháng.",
                                employee->id);
            }
            strcpy(rows[*count].employeeName, employee->fullName);
            strcpy(rows[*count].department, employee->department);
            (*count)++;
        }
    }
    return RESULT_OK;
}

Result finalizePayroll(AppData *app, int month, int year) {
    Payroll rows[MAX_EMPLOYEES];
    size_t rowCount;
    Result result;
    if (isLocked(app, month, year)) {
        return appError(app, RESULT_LOCKED, "Tháng đã chốt, không ghi đè.");
    }
    result = buildPayroll(app, month, year, rows, &rowCount);
    if (result != RESULT_OK) {
        return result;
    }
    /* Cấp sẵn bộ nhớ trước khi ghi tệp để không chốt trên đĩa rồi thiếu bộ nhớ. */
    if (!reserveArray((void **)&app->payrolls, &app->payrollCap, app->payrollCount + rowCount,
                      sizeof(Payroll)) ||
        !reserveArray((void **)&app->locks, &app->lockCap, app->lockCount + 1, sizeof(MonthLock))) {
        return appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ để chốt.");
    }
    /* Bước 1: lưu nguồn công và thông số; nếu lỗi thì chưa chốt. */
    result = saveData(app);
    if (result != RESULT_OK) {
        return result;
    }
    /* Bước 2: công bố tệp chính thức; chỉ sau thành công mới thêm khóa kỳ. */
    result = writePayrollSnapshot(app, month, year, rows, rowCount);
    if (result != RESULT_OK) {
        return result;
    }
    for (size_t index = 0; index < rowCount; index++) {
        app->payrolls[app->payrollCount++] = rows[index];
    }
    app->locks[app->lockCount++] = (MonthLock){month, year};
    return RESULT_OK;
}

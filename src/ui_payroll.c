#include "ui.h"
#include "input.h"
#include "date_utils.h"
#include "employee.h"
#include "attendance.h"
#include "payroll.h"
#include "storage.h"
#include <stdlib.h>
#include <string.h>

/* Biểu mẫu lương chỉ gọi nghiệp vụ; không tự cài công thức tính. */
static void payrollInputForm(AppData *app) {
    int month, year;
    char employeeId[16];
    long long money;
    if (!askPeriod(&month, &year) || !askEmployeeId(employeeId)) {
        return;
    }
    if (isLocked(app, month, year)) {
        puts("Tháng đã chốt, không sửa thông số.");
        return;
    }
    int employeeIndex = findEmployeeIndex(app, employeeId);
    if (employeeIndex < 0 || !employeeInMonth(employeeAt(app, employeeIndex), month, year)) {
        puts("Nhân viên không thuộc kỳ.");
        return;
    }
    PayrollInput payrollInput = {0};
    int inputIndex = findInputIndex(app, employeeId, month, year);
    if (inputIndex >= 0) {
        payrollInput = app->inputs[inputIndex];
    } else {
        strcpy(payrollInput.employeeId, employeeId);
        payrollInput.month = month;
        payrollInput.year = year;
        payrollInput.baseSalary = employeeAt(app, employeeIndex)->baseSalary;
        payrollInput.allowance = employeeAt(app, employeeIndex)->allowance;
    }
    printf("Hiện tại: lương %lld, phụ cấp %lld, thưởng %lld, khấu trừ %lld\n",
           payrollInput.baseSalary, payrollInput.allowance, payrollInput.bonus,
           payrollInput.deduction);
    int menuChoice =
        askMenuChoice("1 Nhập toàn bộ thông số | 2 Lấy lại lương phụ cấp từ hồ sơ | 0 Hủy", 2);
    if (!menuChoice) {
        return;
    }
    if (menuChoice == 2) {
        payrollInput.baseSalary = employeeAt(app, employeeIndex)->baseSalary;
        payrollInput.allowance = employeeAt(app, employeeIndex)->allowance;
    } else {
        if (!askInteger("Lương cơ bản: ", 1, MAX_MONEY, &money)) {
            return;
        }
        payrollInput.baseSalary = money;
        if (!askInteger("Phụ cấp: ", 0, MAX_MONEY, &money)) {
            return;
        }
        payrollInput.allowance = money;
        if (!askInteger("Thưởng: ", 0, MAX_MONEY, &money)) {
            return;
        }
        payrollInput.bonus = money;
        if (!askInteger("Khấu trừ: ", 0, MAX_MONEY, &money)) {
            return;
        }
        payrollInput.deduction = money;
    }
    if (confirm("Y lưu thông số tháng, N hủy: ")) {
        reportResult(app, savePayrollInput(app, &payrollInput));
    }
}

static int comparePayrollById(const void *left, const void *right) {
    return strcmp(((const Payroll *)left)->employeeId, ((const Payroll *)right)->employeeId);
}

static int comparePayrollAscending(const void *left, const void *right) {
    const Payroll *leftPayroll = left;
    const Payroll *rightPayroll = right;
    if (leftPayroll->netPay < rightPayroll->netPay) {
        return -1;
    }
    if (leftPayroll->netPay > rightPayroll->netPay) {
        return 1;
    }
    return comparePayrollById(left, right);
}

static int comparePayrollDescending(const void *left, const void *right) {
    const Payroll *leftPayroll = left;
    const Payroll *rightPayroll = right;
    if (leftPayroll->netPay > rightPayroll->netPay) {
        return -1;
    }
    if (leftPayroll->netPay < rightPayroll->netPay) {
        return 1;
    }
    return comparePayrollById(left, right);
}

static void exportPayrollForm(AppData *app, int month, int year) {
    char filename[PATH_CAP], provided[PATH_CAP];
    snprintf(filename, sizeof filename, "exports/bang_luong_%04d_%02d.csv", year, month);
    printf("Mặc định: %s\n", filename);
    if (!askText("Tệp xuất (trống = mặc định): ", provided, sizeof provided, 1)) {
        return;
    }
    if (*provided) {
        strcpy(filename, provided);
    } else if (!makeDirectory("exports")) {
        puts("Không tạo được exports.");
        return;
    }
    int exists = fileExists(filename);
    if (exists && !confirm("Tệp đã có, Y ghi đè N hủy: ")) {
        return;
    }
    reportResult(app, exportPayroll(app, month, year, filename, exists));
}

static void payrollView(AppData *app, int month, int year) {
    while (!inputEnded()) {
        Payroll rows[MAX_EMPLOYEES];
        size_t rowCount;
        Result result = buildPayroll(app, month, year, rows, &rowCount);
        if (result != RESULT_OK) {
            reportResult(app, result);
            return;
        }
        qsort(rows, rowCount, sizeof(Payroll), comparePayrollById);
        printf("\nKỳ %02d/%d\n", month, year);
        printPayroll(rows, rowCount, isLocked(app, month, year), 1);
        int menuChoice = askMenuChoice(
            "1 Phiếu lương | 2 Chốt | 3 Xuất | 4 Sắp tăng | 5 Sắp giảm | 0 Quay lại", 5);
        if (!menuChoice) {
            return;
        }
        if (menuChoice == 1) {
            char employeeId[16];
            int found = 0;
            if (!askEmployeeId(employeeId)) {
                return;
            }
            for (size_t index = 0; index < rowCount; index++) {
                if (!strcmp(rows[index].employeeId, employeeId)) {
                    printPayslip(&rows[index], isLocked(app, month, year));
                    found = 1;
                }
            }
            if (!found) {
                puts("Không có phiếu lương của mã này trong kỳ.");
            }
        }
        if (menuChoice == 2) {
            if (isLocked(app, month, year)) {
                puts("Tháng đã chốt.");
            } else if (confirm("Sau chốt không sửa dữ liệu kỳ. Y chốt N hủy: ")) {
                reportResult(app, finalizePayroll(app, month, year));
            }
        }
        if (menuChoice == 3) {
            exportPayrollForm(app, month, year);
        }
        if (menuChoice == 4 || menuChoice == 5) {
            qsort(rows, rowCount, sizeof(Payroll),
                  menuChoice == 4 ? comparePayrollAscending : comparePayrollDescending);
            printPayroll(rows, rowCount, isLocked(app, month, year), 1);
        }
    }
}

void payrollMenu(AppData *app) {
    while (!inputEnded()) {
        int menuChoice =
            askMenuChoice("\nBẢNG LƯƠNG\n1 Lập tính lại và xem | 2 Thông số thưởng khấu trừ\n3 Xem "
                          "theo tháng | 4 Phiếu lương | 5 Chốt tháng | 6 Xuất CSV | 0 Quay lại",
                          6);
        int month, year;
        if (!menuChoice) {
            return;
        }
        if (menuChoice == 2) {
            payrollInputForm(app);
            continue;
        }
        if (!askPeriod(&month, &year)) {
            return;
        }
        if (menuChoice == 1 || menuChoice == 3) {
            payrollView(app, month, year);
        }
        if (menuChoice == 4) {
            Payroll rows[MAX_EMPLOYEES];
            size_t rowCount;
            char employeeId[16];
            if (!askEmployeeId(employeeId)) {
                return;
            }
            Result result = buildPayroll(app, month, year, rows, &rowCount);
            if (result != RESULT_OK) {
                reportResult(app, result);
                continue;
            }
            int found = 0;
            for (size_t index = 0; index < rowCount; index++) {
                if (!strcmp(rows[index].employeeId, employeeId)) {
                    printPayslip(&rows[index], isLocked(app, month, year));
                    found = 1;
                }
            }
            if (!found) {
                puts("Không tìm thấy phiếu lương.");
            }
        }
        if (menuChoice == 5) {
            Payroll rows[MAX_EMPLOYEES];
            size_t rowCount;
            Result result = buildPayroll(app, month, year, rows, &rowCount);
            if (result != RESULT_OK) {
                reportResult(app, result);
                continue;
            }
            printPayroll(rows, rowCount, isLocked(app, month, year), 0);
            if (confirm("Y chốt tháng, N hủy: ")) {
                reportResult(app, finalizePayroll(app, month, year));
            }
        }
        if (menuChoice == 6) {
            exportPayrollForm(app, month, year);
        }
    }
}

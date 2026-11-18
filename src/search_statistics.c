#include "ui.h"
#include "input.h"
#include "employee.h"
#include "payroll.h"
#include <string.h> /* Dùng cho strcmp, strstr */
#include <stdlib.h> /* Dùng cho qsort */

/* Menu tìm kiếm kết hợp nhiều điều kiện AND cho nhân viên, chấm công hoặc bảng lương */
void searchMenu(AppData *app) {
    long long searchType;
    char employeeId[16], employeeName[101], department[61];
    int month, year;
    if (!askInteger("Tìm 1 nhân viên 2 chấm công 3 lương 0 quay lại: ", 0, 3, &searchType) ||
        !searchType) {
        return;
    }
    if (!askText("Mã (trống = tất cả): ", employeeId, sizeof employeeId, 1)) {
        return;
    }
    normalizeId(employeeId); /* Chuẩn hóa mã tìm kiếm về chữ hoa */
    if (searchType == 1) { /* Tìm kiếm hồ sơ nhân viên */
        long long status;
        if (!askText("Tên chứa (phân biệt hoa thường và dấu): ", employeeName, sizeof employeeName,
                     1) ||
            !askText("Phòng ban (trống = tất cả): ", department, sizeof department, 1) ||
            !askInteger("Trạng thái 0 tất cả 1 đang làm 2 đã nghỉ: ", 0, 2, &status)) {
            return;
        }
        printEmployees(app, employeeId, employeeName, department, (int)status, 1);
        return;
    }
    /* Tìm chấm công (2) hoặc bảng lương (3) đều cần nhập tháng và năm */
    if (!askPeriod(&month, &year)) {
        return;
    }
    if (searchType == 2) { /* Tìm kiếm dữ liệu chấm công trong kỳ */
        printAttendance(app, employeeId, month, year, NULL, 1);
        return;
    }
    /* Tìm kiếm bảng lương trong kỳ theo mã, tên chứa và phòng ban */
    Payroll rows[MAX_EMPLOYEES], filtered[MAX_EMPLOYEES];
    size_t count, rowCount = 0;
    Result result = buildPayroll(app, month, year, rows, &count);
    if (result != RESULT_OK) {
        reportResult(app, result);
        return;
    }
    if (!askText("Tên chứa (trống = tất cả): ", employeeName, sizeof employeeName, 1) ||
        !askText("Phòng ban (trống = tất cả): ", department, sizeof department, 1)) {
        return;
    }
    /* Lọc các dòng thỏa mãn đồng thời mọi tiêu chí (AND) */
    for (size_t index = 0; index < count; index++) {
        Payroll *payroll = &rows[index];
        if (*employeeId && strcmp(employeeId, payroll->employeeId)) {
            continue;
        }
        if (*employeeName && !strstr(payroll->employeeName, employeeName)) {
            continue;
        }
        if (*department && strcmp(department, payroll->department)) {
            continue;
        }
        filtered[rowCount++] = *payroll;
    }
    if (!rowCount) {
        puts("Không có kết quả phù hợp.");
        return;
    }
    printPayroll(filtered, rowCount, isLocked(app, month, year), 1);
    /* Cho phép người dùng chọn xem ngay phiếu lương chi tiết từ kết quả vừa lọc */
    if (confirm("Y xem một phiếu lương, N quay lại: ") &&
        askText("Mã: ", employeeId, sizeof employeeId, 0)) {
        normalizeId(employeeId);
        int found = 0;
        for (size_t index = 0; index < rowCount; index++) {
            if (!strcmp(employeeId, filtered[index].employeeId)) {
                printPayslip(&filtered[index], isLocked(app, month, year));
                found = 1;
            }
        }
        if (!found) {
            puts("Mã không nằm trong kết quả.");
        }
    }
}

/* Hàm so sánh giảm dần theo thực lĩnh để xếp hạng nhóm lương cao nhất (dùng cho qsort) */
static int descending(const void *left, const void *right) {
    const Payroll *leftPayroll = left, *rightPayroll = right;
    return leftPayroll->netPay == rightPayroll->netPay
               ? strcmp(leftPayroll->employeeId, rightPayroll->employeeId)
               : (leftPayroll->netPay < rightPayroll->netPay) -
                     (leftPayroll->netPay > rightPayroll->netPay);
}

/* Menu thống kê tổng hợp: 1-Nhân sự, 2-Ngày công & tăng ca, 3-Quỹ lương & Top thu nhập */
void statisticsMenu(AppData *app) {
    long long menuChoice;
    if (!askInteger("Thống kê 1 nhân sự 2 công 3 quỹ lương 0 quay lại: ", 0, 3, &menuChoice) ||
        !menuChoice) {
        return;
    }
    if (menuChoice == 1) { /* Thống kê cơ cấu nhân sự toàn công ty và theo từng phòng ban */
        size_t active = 0, left = 0;
        for (size_t index = 0; index < app->employeeCount; index++) {
            if (employeeAt(app, index)->status == 1) {
                active++;
            } else {
                left++;
            }
        }
        printf("Tổng %zu | Đang làm %zu | Đã nghỉ %zu\n", app->employeeCount, active, left);
        /* Duyệt qua từng nhân viên, gom nhóm theo tên phòng ban để đếm (tránh đếm trùng phòng đã gặp) */
        for (size_t index = 0; index < app->employeeCount; index++) {
            int seen = 0;
            size_t rowCount = 0, departmentActive = 0;
            for (size_t otherIndex = 0; otherIndex < index; otherIndex++) { /* Kiểm tra phòng ban này đã thống kê trước đó chưa */
                if (!strcmp(employeeAt(app, otherIndex)->department,
                            employeeAt(app, index)->department)) {
                    seen = 1;
                }
            }
            if (seen) {
                continue;
            }
            for (size_t otherIndex = 0; otherIndex < app->employeeCount; otherIndex++) { /* Đếm tổng số người của phòng ban đó */
                if (!strcmp(employeeAt(app, otherIndex)->department,
                            employeeAt(app, index)->department)) {
                    rowCount++;
                    if (employeeAt(app, otherIndex)->status == 1) {
                        departmentActive++;
                    }
                }
            }
            printf("%s | Tổng %zu | Đang làm %zu | Đã nghỉ %zu\n",
                   employeeAt(app, index)->department, rowCount, departmentActive,
                   rowCount - departmentActive);
        }
        return;
    }
    int month, year;
    if (!askPeriod(&month, &year)) {
        return;
    }
    if (menuChoice == 2) { /* Thống kê tổng hợp các bản ghi chấm công trong tháng */
        int workDays = 0, paidLeaveDays = 0, unpaidLeaveDays = 0, regularUnits = 0,
            overtimeUnits = 0;
        for (size_t index = 0; index < app->attendanceCount; index++) {
            Attendance *record = &app->attendance[index];
            if (record->date.month != month || record->date.year != year) {
                continue;
            }
            if (record->type == ATT_WORK) {
                workDays++;
            }
            if (record->type == ATT_PAID_LEAVE) {
                paidLeaveDays++;
            }
            if (record->type == ATT_UNPAID_LEAVE) {
                unpaidLeaveDays++;
            }
            regularUnits += record->regularHalfHours;
            overtimeUnits += record->overtimeHalfHours;
        }
        printf("BẢN GHI CÔNG %02d/%d\nNgày đi làm: %d | Nghỉ có lương: %d | Nghỉ không lương: "
               "%d\nGiờ thường: %.1f | Giờ tăng ca: %.1f\nCông hưởng: %.2f\nSố liệu từ bản ghi đã "
               "nhập, có thể còn thiếu công.\n",
               month, year, workDays, paidLeaveDays, unpaidLeaveDays, regularUnits / 2.0,
               overtimeUnits / 2.0, (regularUnits + paidLeaveDays * 16) / 16.0);
        return;
    }
    /* Thống kê quỹ lương: phân biệt rạch ròi nguồn chính thức (đã chốt) và nguồn tạm */
    long long source;
    if (!askInteger("Nguồn 1 chính thức 2 tạm: ", 1, 2, &source)) {
        return;
    }
    if (source == 1 && !isLocked(app, month, year)) {
        puts("Chưa chốt kỳ này. Không có quỹ lương chính thức.");
        return;
    }
    if (source == 2 && isLocked(app, month, year)) {
        puts("Kỳ đã chốt; hiển thị nguồn chính thức.");
    }
    Payroll rows[MAX_EMPLOYEES];
    size_t rowCount;
    Result result = buildPayroll(app, month, year, rows, &rowCount);
    if (result != RESULT_OK) {
        reportResult(app, result);
        return;
    }
    qsort(rows, rowCount, sizeof(Payroll), descending); /* Sắp xếp giảm dần theo thực lĩnh */
    printPayroll(rows, rowCount, isLocked(app, month, year), 1);
    puts("XẾP HẠNG THỰC LĨNH CAO NHẤT");
    for (size_t index = 0; index < rowCount && index < 5; index++) { /* In Top 5 người thực lĩnh cao nhất */
        printf("%zu | %s | %s | %lld\n", index + 1, rows[index].employeeId,
               rows[index].employeeName, rows[index].netPay);
    }
}

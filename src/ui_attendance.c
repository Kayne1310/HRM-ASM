#include "ui.h"
#include "input.h"
#include "date_utils.h"
#include "employee.h"
#include "attendance.h"
#include "payroll.h"
#include "storage.h"
#include <stdlib.h>
#include <string.h>

/* Nhập và xem chấm công; quy tắc hợp lệ nằm trong attendance.c. */
static void attendanceForm(AppData *app, const char *givenEmployeeId, Date date, int editing) {
    Attendance record = {0};
    long long type;
    if (givenEmployeeId) {
        strcpy(record.employeeId, givenEmployeeId);
    } else if (!askEmployeeId(record.employeeId) || !askDate("Ngày công: ", &date)) {
        return;
    }
    record.date = date;
    if (isLocked(app, date.month, date.year)) {
        puts("Tháng đã chốt, chỉ xem.");
        return;
    }
    int recordIndex = findAttendanceIndex(app, record.employeeId, date);
    if (editing) {
        if (recordIndex < 0) {
            puts("Không có bản ghi để sửa.");
            return;
        }
        record = app->attendance[recordIndex];
    } else if (recordIndex >= 0) {
        puts("Bản ghi đã có. Chọn sửa.");
        return;
    }
    printf("Chấm %s ngày %02d/%02d/%d\n", record.employeeId, date.day, date.month, date.year);
    if (!askInteger("Loại 1 đi làm 2 nghỉ có lương 3 nghỉ không lương: ", 1, 3, &type)) {
        return;
    }
    record.type = (AttendanceType)type;
    record.regularHalfHours = 0;
    record.overtimeHalfHours = 0;
    if (record.type == ATT_WORK && (!askHours("Giờ thường: ", 1, 16, &record.regularHalfHours) ||
                                    !askHours("Giờ tăng ca: ", 0, 8, &record.overtimeHalfHours))) {
        return;
    }
    if (!askText("Ghi chú: ", record.note, sizeof record.note, 1)) {
        return;
    }
    if (confirm("Y ghi nhận, N hủy hoặc bỏ qua: ")) {
        reportResult(app, saveAttendance(app, &record, editing));
    }
}

void attendanceMenu(AppData *app) {
    while (!inputEnded()) {
        int menuChoice =
            askMenuChoice("\nCHẤM CÔNG\n1 Một người | 2 Lần lượt theo ngày | 3 Xem theo ngày\n4 "
                          "Xem theo người tháng | 5 Sửa | 6 Xóa | 7 Kiểm tra thiếu | 0 Quay lại",
                          7);
        if (!menuChoice) {
            return;
        }
        if (menuChoice == 1 || menuChoice == 5) {
            attendanceForm(app, NULL, (Date){0}, menuChoice == 5);
        }
        if (menuChoice == 2) {
            Date selectedDate;
            if (!askDate("Ngày chấm: ", &selectedDate)) {
                return;
            }
            if (!isWorkingDay(selectedDate) ||
                isLocked(app, selectedDate.month, selectedDate.year)) {
                puts("Ngày không phải ngày làm hoặc tháng đã chốt.");
                continue;
            }
            for (size_t employeeIndex = 0; employeeIndex < app->employeeCount && !inputEnded();
                 employeeIndex++) {
                if (employeeOnDate(employeeAt(app, employeeIndex), selectedDate)) {
                    printf("Nhân viên %zu/%zu: %s\n", employeeIndex + 1, app->employeeCount,
                           employeeAt(app, employeeIndex)->fullName);
                    attendanceForm(app, employeeAt(app, employeeIndex)->id, selectedDate, 0);
                }
            }
        }
        if (menuChoice == 3) {
            Date selectedDate;
            if (!askDate("Ngày xem: ", &selectedDate)) {
                return;
            }
            printAttendance(app, NULL, 0, 0, &selectedDate, 1);
        }
        if (menuChoice == 4 || menuChoice == 7) {
            char employeeId[16];
            int month, year;
            if (!askPeriod(&month, &year) ||
                !askText("Mã (trống = tất cả): ", employeeId, sizeof employeeId, 1)) {
                return;
            }
            normalizeId(employeeId);
            if (*employeeId && findEmployeeIndex(app, employeeId) < 0) {
                puts("Không tìm thấy nhân viên.");
                continue;
            }
            if (menuChoice == 4) {
                printAttendance(app, employeeId, month, year, NULL, 1);
            } else {
                int missingCount = missingAttendance(app, month, year, employeeId, stdout);
                printf("Số ngày thiếu: %d\n", missingCount);
                if (missingCount && confirm("Y bổ sung một bản ghi, N quay lại: ")) {
                    attendanceForm(app, NULL, (Date){0}, 0);
                }
            }
        }
        if (menuChoice == 6) {
            char employeeId[16];
            Date selectedDate;
            if (!askEmployeeId(employeeId) || !askDate("Ngày xóa: ", &selectedDate)) {
                return;
            }
            if (confirm("Y xóa công, N hủy: ")) {
                reportResult(app, deleteAttendance(app, employeeId, selectedDate));
            }
        }
    }
}

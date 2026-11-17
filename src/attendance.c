#include "attendance.h"
#include "employee.h"
#include "date_utils.h"
#include <string.h>

int findAttendanceIndex(const AppData *app, const char *employeeId, Date date) {
    for (size_t index = 0; index < app->attendanceCount; index++) {
        if (!strcmp(app->attendance[index].employeeId, employeeId) &&
            !compareDate(app->attendance[index].date, date)) {
            return (int)index;
        }
    }
    return -1;
}

Result validateAttendance(AppData *app, const Attendance *record) {
    int employeeIndex = findEmployeeIndex(app, record->employeeId);
    if (employeeIndex < 0) {
        return appError(app, RESULT_NOT_FOUND, "Nhân viên %s không tồn tại.", record->employeeId);
    }
    if (!isWorkingDay(record->date) ||
        !employeeOnDate(employeeAt(app, employeeIndex), record->date)) {
        return appError(app, RESULT_INVALID, "Ngày công không thuộc lịch hoặc thời gian làm việc.");
    }
    if (!validText(record->note, 200, 1)) {
        return appError(app, RESULT_INVALID, "Ghi chú không hợp lệ.");
    }
    if (record->type == ATT_WORK) {
        if (record->regularHalfHours < 1 || record->regularHalfHours > 16 ||
            record->overtimeHalfHours < 0 || record->overtimeHalfHours > 8) {
            return appError(app, RESULT_INVALID, "Giờ thường 0.5 đến 8; tăng ca 0 đến 4.");
        }
    } else if ((record->type != ATT_PAID_LEAVE && record->type != ATT_UNPAID_LEAVE) ||
               record->regularHalfHours || record->overtimeHalfHours) {
        return appError(app, RESULT_INVALID, "Loại công sai hoặc ngày nghỉ có số giờ khác 0.");
    }
    return RESULT_OK;
}

Result saveAttendance(AppData *app, const Attendance *record, int editing) {
    int recordIndex;
    Result result;
    if (isLocked(app, record->date.month, record->date.year)) {
        return appError(app, RESULT_LOCKED, "Tháng đã chốt, chỉ được xem.");
    }
    result = validateAttendance(app, record);
    if (result != RESULT_OK) {
        return result;
    }
    recordIndex = findAttendanceIndex(app, record->employeeId, record->date);
    if (editing && recordIndex < 0) {
        return appError(app, RESULT_NOT_FOUND, "Không có bản ghi để sửa.");
    }
    if (!editing && recordIndex >= 0) {
        return appError(app, RESULT_DUPLICATE, "Bản ghi đã có; dùng chức năng sửa.");
    }
    if (!editing) {
        if (app->attendanceCount >= MAX_ATTENDANCE) {
            return appError(app, RESULT_FULL, "Đã đầy danh sách công.");
        }
        if (!reserveArray((void **)&app->attendance, &app->attendanceCap, app->attendanceCount + 1,
                          sizeof(Attendance))) {
            return appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ.");
        }
        recordIndex = (int)app->attendanceCount++;
    }
    app->attendance[recordIndex] = *record;
    app->dirty = 1;
    return RESULT_OK;
}

Result deleteAttendance(AppData *app, const char *employeeId, Date date) {
    int recordIndex = findAttendanceIndex(app, employeeId, date);
    if (isLocked(app, date.month, date.year)) {
        return appError(app, RESULT_LOCKED, "Tháng đã chốt.");
    }
    if (recordIndex < 0) {
        return appError(app, RESULT_NOT_FOUND, "Không tìm thấy công.");
    }
    memmove(&app->attendance[recordIndex], &app->attendance[recordIndex + 1],
            (app->attendanceCount - (size_t)recordIndex - 1) * sizeof(Attendance));
    app->attendanceCount--;
    app->dirty = 1;
    return RESULT_OK;
}

/* Bảng present[nhân viên][ngày] giúp nhận biết ngày chưa chấm, không coi là nghỉ. */
int missingAttendance(const AppData *app, int month, int year, const char *employeeId,
                      FILE *output) {
    unsigned char present[MAX_EMPLOYEES][32] = {{0}};
    int missing = 0;
    if (!validMonth(month, year)) {
        return -1;
    }
    for (size_t index = 0; index < app->attendanceCount; index++) {
        const Attendance *record = &app->attendance[index];
        if (record->date.month == month && record->date.year == year) {
            int employeeIndex = findEmployeeIndex(app, record->employeeId);
            if (employeeIndex >= 0 && record->date.day >= 1 && record->date.day <= 31) {
                present[employeeIndex][record->date.day] = 1;
            }
        }
    }
    for (size_t index = 0; index < app->employeeCount; index++) {
        const Employee *employee = employeeAt(app, index);
        if (employeeId && *employeeId && strcmp(employeeId, employee->id)) {
            continue;
        }
        for (int day = 1; day <= daysInMonth(month, year); day++) {
            Date date = {day, month, year};
            if (isWorkingDay(date) && employeeOnDate(employee, date) && !present[index][day]) {
                missing++;
                if (output) {
                    fprintf(output, "%s | %s | %02d/%02d/%d\n", employee->id, employee->fullName,
                            day, month, year);
                }
            }
        }
    }
    return missing;
}

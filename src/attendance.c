#include "attendance.h"
#include "employee.h"
#include "date_utils.h"
#include <string.h>
#include <time.h>

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

Result generateMonthlyAttendance(AppData *app, const char *employeeId, int month, int year,
                                 const int leaveDays[31], size_t leaveDayCount,
                                 AttendanceType leaveType, size_t *createdCount) {
    Attendance generated[31];
    size_t generatedCount = 0;
    int employeeIndex = findEmployeeIndex(app, employeeId);
    if (createdCount) {
        *createdCount = 0;
    }
    if (!validMonth(month, year) || employeeIndex < 0) {
        return appError(app, RESULT_INVALID, "Nhân viên hoặc kỳ chấm công không hợp lệ.");
    }
    if (isLocked(app, month, year)) {
        return appError(app, RESULT_LOCKED, "Tháng đã chốt.");
    }
    if (leaveDayCount > 31 || (leaveDayCount && !leaveDays) ||
        (leaveType != ATT_PAID_LEAVE && leaveType != ATT_UNPAID_LEAVE)) {
        return appError(app, RESULT_INVALID, "Danh sách ngày nghỉ không hợp lệ.");
    }

    time_t now = time(NULL);
    struct tm *currentTime = localtime(&now);
    if (!currentTime) {
        return appError(app, RESULT_INVALID, "Không xác định được ngày hiện tại.");
    }
    int lastDay = daysInMonth(month, year);
    if (year > currentTime->tm_year + 1900 ||
        (year == currentTime->tm_year + 1900 && month > currentTime->tm_mon + 1)) {
        return appError(app, RESULT_INVALID, "Không thể tạo công cho tháng trong tương lai.");
    }
    if (year == currentTime->tm_year + 1900 && month == currentTime->tm_mon + 1 &&
        lastDay > currentTime->tm_mday) {
        lastDay = currentTime->tm_mday;
    }

    const Employee *employee = employeeAt(app, employeeIndex);
    for (size_t index = 0; index < leaveDayCount; index++) {
        int day = leaveDays[index];
        Date date = {day, month, year};
        if (day < 1 || day > lastDay || !isWorkingDay(date) || !employeeOnDate(employee, date)) {
            return appError(app, RESULT_INVALID,
                            "Ngày nghỉ phải là ngày làm thuộc thời gian làm việc trong kỳ.");
        }
        for (size_t otherIndex = 0; otherIndex < index; otherIndex++) {
            if (leaveDays[otherIndex] == day) {
                return appError(app, RESULT_DUPLICATE, "Ngày nghỉ bị nhập trùng.");
            }
        }
        if (findAttendanceIndex(app, employeeId, date) >= 0) {
            return appError(app, RESULT_DUPLICATE,
                            "Ngày nghỉ đã có bản ghi chấm công; hãy kiểm tra trước khi tạo nhanh.");
        }
    }

    for (int day = 1; day <= lastDay; day++) {
        Date date = {day, month, year};
        if (!isWorkingDay(date) || !employeeOnDate(employee, date)) {
            continue;
        }
        if (findAttendanceIndex(app, employeeId, date) >= 0) {
            continue;
        }
        int isLeaveDay = 0;
        for (size_t index = 0; index < leaveDayCount; index++) {
            if (leaveDays[index] == day) {
                isLeaveDay = 1;
                break;
            }
        }
        Attendance *record = &generated[generatedCount++];
        *record = (Attendance){0};
        strcpy(record->employeeId, employeeId);
        record->date = date;
        record->type = isLeaveDay ? leaveType : ATT_WORK;
        record->regularHalfHours = isLeaveDay ? 0 : 16;
        record->overtimeHalfHours = 0;
        if (isLeaveDay) {
            strcpy(record->note, "Nghỉ từ chấm công nhanh");
        }
    }

    if (!generatedCount) {
        return appError(app, RESULT_INVALID, "Kỳ này không còn ngày làm nào cần tạo.");
    }
    if (app->attendanceCount + generatedCount > MAX_ATTENDANCE) {
        return appError(app, RESULT_FULL, "Đã đầy danh sách công.");
    }
    if (!reserveArray((void **)&app->attendance, &app->attendanceCap,
                      app->attendanceCount + generatedCount, sizeof(Attendance))) {
        return appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ.");
    }
    memcpy(&app->attendance[app->attendanceCount], generated,
           generatedCount * sizeof(Attendance));
    app->attendanceCount += generatedCount;
    app->dirty = 1;
    if (createdCount) {
        *createdCount = generatedCount;
    }
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

#include "attendance.h"
#include "employee.h"
#include "date_utils.h"
#include <string.h> /* Dùng cho strcmp, strcpy, memmove */
#include <time.h>   /* Dùng cho time, localtime lấy ngày hiện tại (chấm công nhanh) */

/* Tìm vị trí bản ghi chấm công theo cặp khóa (employeeId, date); trả về chỉ số mảng hoặc -1 */
int findAttendanceIndex(const AppData *app, const char *employeeId, Date date) {
    for (size_t index = 0; index < app->attendanceCount; index++) {
        if (!strcmp(app->attendance[index].employeeId, employeeId) &&
            !compareDate(app->attendance[index].date, date)) { /* Trùng cả mã nhân viên lẫn ngày */
            return (int)index;
        }
    }
    return -1;
}

/* Kiểm tra toàn bộ quy tắc hợp lệ của 1 bản ghi chấm công */
Result validateAttendance(AppData *app, const Attendance *record) {
    int employeeIndex = findEmployeeIndex(app, record->employeeId);
    if (employeeIndex < 0) { /* Nhân viên phải tồn tại trong danh sách */
        return appError(app, RESULT_NOT_FOUND, "Nhân viên %s không tồn tại.", record->employeeId);
    }
    /* Ngày chấm phải là ngày đi làm (T2-T7) và nằm trong khoảng ngày vào -> ngày nghỉ của nhân viên */
    if (!isWorkingDay(record->date) ||
        !employeeOnDate(employeeAt(app, employeeIndex), record->date)) {
        return appError(app, RESULT_INVALID, "Ngày công không thuộc lịch hoặc thời gian làm việc.");
    }
    if (!validText(record->note, 200, 1)) { /* Ghi chú được phép rỗng (allowEmpty = 1), tối đa 200 byte */
        return appError(app, RESULT_INVALID, "Ghi chú không hợp lệ.");
    }
    if (record->type == ATT_WORK) { /* Nếu đi làm: giờ thường 0.5-8h (1-16 đv), tăng ca 0-4h (0-8 đv) */
        if (record->regularHalfHours < 1 || record->regularHalfHours > 16 ||
            record->overtimeHalfHours < 0 || record->overtimeHalfHours > 8) {
            return appError(app, RESULT_INVALID, "Giờ thường 0.5 đến 8; tăng ca 0 đến 4.");
        }
    } else if ((record->type != ATT_PAID_LEAVE && record->type != ATT_UNPAID_LEAVE) ||
               record->regularHalfHours || record->overtimeHalfHours) {
        /* Ngày nghỉ (có lương / không lương) bắt buộc phải có giờ thường = 0 và giờ tăng ca = 0 */
        return appError(app, RESULT_INVALID, "Loại công sai hoặc ngày nghỉ có số giờ khác 0.");
    }
    return RESULT_OK;
}

/* Lưu bản ghi chấm công mới hoặc ghi đè khi sửa (bị chặn nếu tháng đã chốt) */
Result saveAttendance(AppData *app, const Attendance *record, int editing) {
    int recordIndex;
    Result result;
    if (isLocked(app, record->date.month, record->date.year)) { /* Không cho thay đổi dữ liệu kỳ đã chốt */
        return appError(app, RESULT_LOCKED, "Tháng đã chốt, chỉ được xem.");
    }
    result = validateAttendance(app, record); /* Kiểm tra hợp lệ trước khi lưu */
    if (result != RESULT_OK) {
        return result;
    }
    recordIndex = findAttendanceIndex(app, record->employeeId, record->date);
    if (editing && recordIndex < 0) { /* Sửa nhưng không tìm thấy bản ghi */
        return appError(app, RESULT_NOT_FOUND, "Không có bản ghi để sửa.");
    }
    if (!editing && recordIndex >= 0) { /* Thêm mới nhưng ngày này đã chấm rồi */
        return appError(app, RESULT_DUPLICATE, "Bản ghi đã có; dùng chức năng sửa.");
    }
    if (!editing) { /* Nếu là thêm mới: kiểm tra sức chứa và mở rộng mảng động nếu cần */
        if (app->attendanceCount >= MAX_ATTENDANCE) {
            return appError(app, RESULT_FULL, "Đã đầy danh sách công.");
        }
        if (!reserveArray((void **)&app->attendance, &app->attendanceCap, app->attendanceCount + 1,
                          sizeof(Attendance))) {
            return appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ.");
        }
        recordIndex = (int)app->attendanceCount++; /* Lấy chỉ số cuối mảng mới mở */
    }
    app->attendance[recordIndex] = *record; /* Ghi bản ghi vào vị trí */
    app->dirty = 1;                         /* Đánh dấu có thay đổi chưa lưu */
    return RESULT_OK;
}

/* Xóa một bản ghi chấm công (dùng memmove dồn các phần tử phía sau lên) */
Result deleteAttendance(AppData *app, const char *employeeId, Date date) {
    int recordIndex = findAttendanceIndex(app, employeeId, date);
    if (isLocked(app, date.month, date.year)) {
        return appError(app, RESULT_LOCKED, "Tháng đã chốt.");
    }
    if (recordIndex < 0) {
        return appError(app, RESULT_NOT_FOUND, "Không tìm thấy công.");
    }
    /* Dịch chuyển toàn bộ phần tử sau recordIndex lên trước 1 ô để lấp chỗ trống */
    memmove(&app->attendance[recordIndex], &app->attendance[recordIndex + 1],
            (app->attendanceCount - (size_t)recordIndex - 1) * sizeof(Attendance));
    app->attendanceCount--; /* Giảm tổng số bản ghi công */
    app->dirty = 1;
    return RESULT_OK;
}

/* Chức năng chấm công nhanh cả tháng cho 1 nhân viên (từ ngày 1 tới hết tháng hoặc ngày hiện tại) */
Result generateMonthlyAttendance(AppData *app, const char *employeeId, int month, int year,
                                 const int leaveDays[31], size_t leaveDayCount,
                                 AttendanceType leaveType, size_t *createdCount) {
    Attendance generated[31]; /* Mảng tạm chứa tối đa 31 ngày công chuẩn bị tạo cho tháng đó */
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

    time_t now = time(NULL);                  /* Lấy thời gian hệ thống hiện tại (số giây Unix) */
    struct tm *currentTime = localtime(&now); /* Đổi sang ngày/tháng/năm địa phương */
    if (!currentTime) {
        return appError(app, RESULT_INVALID, "Không xác định được ngày hiện tại.");
    }
    int lastDay = daysInMonth(month, year); /* Mặc định tạo đến ngày cuối cùng của tháng */
    /* Không cho tạo nhanh cho tháng ở tương lai */
    if (year > currentTime->tm_year + 1900 ||
        (year == currentTime->tm_year + 1900 && month > currentTime->tm_mon + 1)) {
        return appError(app, RESULT_INVALID, "Không thể tạo công cho tháng trong tương lai.");
    }
    /* Nếu là tháng hiện tại thì chỉ tạo tối đa đến ngày hôm nay */
    if (year == currentTime->tm_year + 1900 && month == currentTime->tm_mon + 1 &&
        lastDay > currentTime->tm_mday) {
        lastDay = currentTime->tm_mday;
    }

    const Employee *employee = employeeAt(app, employeeIndex);
    /* Kiểm tra trước toàn bộ danh sách các ngày nghỉ người dùng nhập vào xem có hợp lệ không */
    for (size_t index = 0; index < leaveDayCount; index++) {
        int day = leaveDays[index];
        Date date = {day, month, year};
        if (day < 1 || day > lastDay || !isWorkingDay(date) || !employeeOnDate(employee, date)) {
            return appError(app, RESULT_INVALID,
                            "Ngày nghỉ phải là ngày làm thuộc thời gian làm việc trong kỳ.");
        }
        for (size_t otherIndex = 0; otherIndex < index; otherIndex++) { /* Kiểm tra trùng ngay trong mảng nhập */
            if (leaveDays[otherIndex] == day) {
                return appError(app, RESULT_DUPLICATE, "Ngày nghỉ bị nhập trùng.");
            }
        }
        if (findAttendanceIndex(app, employeeId, date) >= 0) { /* Nếu ngày nghỉ đã từng chấm công rồi -> báo lỗi */
            return appError(app, RESULT_DUPLICATE,
                            "Ngày nghỉ đã có bản ghi chấm công; hãy kiểm tra trước khi tạo nhanh.");
        }
    }

    /* Duyệt qua từng ngày từ ngày 1 tới lastDay để sinh các bản ghi còn thiếu */
    for (int day = 1; day <= lastDay; day++) {
        Date date = {day, month, year};
        if (!isWorkingDay(date) || !employeeOnDate(employee, date)) { /* Bỏ qua Chủ nhật hoặc ngoài ngày làm việc */
            continue;
        }
        if (findAttendanceIndex(app, employeeId, date) >= 0) { /* Ngày nào đã chấm rồi thì giữ nguyên, bỏ qua */
            continue;
        }
        int isLeaveDay = 0; /* Kiểm tra ngày này có nằm trong danh sách ngày nghỉ đã khai báo không */
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
        record->type = isLeaveDay ? leaveType : ATT_WORK;      /* Nghỉ theo loại đã chọn, ngược lại đi làm */
        record->regularHalfHours = isLeaveDay ? 0 : 16;        /* Đi làm tự điền đủ 8 giờ (16 nửa giờ) */
        record->overtimeHalfHours = 0;                         /* Mặc định không tăng ca */
        if (isLeaveDay) {
            strcpy(record->note, "Nghỉ từ chấm công nhanh");
        }
    }

    if (!generatedCount) { /* Toàn bộ các ngày trong tháng đã được chấm sẵn từ trước */
        return appError(app, RESULT_INVALID, "Kỳ này không còn ngày làm nào cần tạo.");
    }
    if (app->attendanceCount + generatedCount > MAX_ATTENDANCE) {
        return appError(app, RESULT_FULL, "Đã đầy danh sách công.");
    }
    /* Cấp phát đủ chỗ chứa cho tất cả bản ghi mới cùng lúc rồi chép hàng loạt vào mảng */
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

/* Dùng ma trận đánh dấu present[nhân viên][ngày] để phát hiện chính xác ngày chưa chấm công (không tự coi là nghỉ). */
int missingAttendance(const AppData *app, int month, int year, const char *employeeId,
                      FILE *output) {
    unsigned char present[MAX_EMPLOYEES][32] = {{0}}; /* Bảng 2 chiều lưu 0 (chưa chấm) hoặc 1 (đã chấm) */
    int missing = 0;
    if (!validMonth(month, year)) {
        return -1;
    }
    /* Bước 1: Duyệt mảng công 1 lượt để đánh dấu các ô đã chấm công trong tháng/năm cần xét */
    for (size_t index = 0; index < app->attendanceCount; index++) {
        const Attendance *record = &app->attendance[index];
        if (record->date.month == month && record->date.year == year) {
            int employeeIndex = findEmployeeIndex(app, record->employeeId);
            if (employeeIndex >= 0 && record->date.day >= 1 && record->date.day <= 31) {
                present[employeeIndex][record->date.day] = 1;
            }
        }
    }
    /* Bước 2: Duyệt từng nhân viên và từng ngày làm việc của tháng; ô nào bằng 0 tức là thiếu công */
    for (size_t index = 0; index < app->employeeCount; index++) {
        const Employee *employee = employeeAt(app, index);
        if (employeeId && *employeeId && strcmp(employeeId, employee->id)) { /* Nếu lọc riêng 1 mã thì bỏ qua người khác */
            continue;
        }
        for (int day = 1; day <= daysInMonth(month, year); day++) {
            Date date = {day, month, year};
            if (isWorkingDay(date) && employeeOnDate(employee, date) && !present[index][day]) {
                missing++;
                if (output) { /* Nếu truyền luồng in (vd: stdout) thì in thông tin ngày bị thiếu ra màn hình */
                    fprintf(output, "%s | %s | %02d/%02d/%d\n", employee->id, employee->fullName,
                            day, month, year);
                }
            }
        }
    }
    return missing; /* Trả về tổng số ngày thiếu công */
}

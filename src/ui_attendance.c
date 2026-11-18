#include "ui.h"
#include "input.h"
#include "date_utils.h"
#include "employee.h"
#include "attendance.h"
#include "payroll.h"
#include "storage.h"
#include <stdlib.h>
#include <string.h> /* Dùng cho strcpy */

/* Biểu mẫu nhập hoặc sửa 1 bản ghi chấm công; toàn bộ quy tắc hợp lệ do attendance.c kiểm tra. */
static void attendanceForm(AppData *app, const char *givenEmployeeId, Date date, int editing) {
    Attendance record = {0};
    long long type;
    if (givenEmployeeId) { /* Trường hợp chấm hàng loạt theo ngày: đã biết sẵn mã nhân viên và ngày */
        strcpy(record.employeeId, givenEmployeeId);
    } else if (!askEmployeeId(record.employeeId) || !askDate("Ngày công: ", &date)) {
        return;
    }
    record.date = date;
    if (isLocked(app, date.month, date.year)) { /* Không cho chấm hoặc sửa vào tháng đã chốt */
        puts("Tháng đã chốt, chỉ xem.");
        return;
    }
    int recordIndex = findAttendanceIndex(app, record.employeeId, date);
    if (editing) {
        if (recordIndex < 0) {
            puts("Không có bản ghi để sửa.");
            return;
        }
        record = app->attendance[recordIndex]; /* Nạp bản ghi cũ lên để sửa */
    } else if (recordIndex >= 0) {
        puts("Bản ghi đã có. Chọn sửa.");
        return;
    }
    printf("Chấm %s ngày %02d/%02d/%d\n", record.employeeId, date.day, date.month, date.year);
    if (!askInteger("Loại 1 đi làm 2 nghỉ có lương 3 nghỉ không lương: ", 1, 3, &type)) {
        return;
    }
    record.type = (AttendanceType)type;
    record.regularHalfHours = 0;  /* Mặc định đặt 0 giờ (nếu là ngày nghỉ sẽ giữ nguyên 0) */
    record.overtimeHalfHours = 0;
    /* Chỉ hỏi giờ làm thường và giờ tăng ca khi chọn loại 1 (ATT_WORK - đi làm) */
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

/* Biểu mẫu tạo công nhanh cả tháng cho 1 nhân viên (tự điền 8h các ngày làm việc, trừ ngày nghỉ) */
static void quickAttendanceForm(AppData *app) {
    char employeeId[16];
    int month, year;
    long long leaveCountInput;
    int leaveDays[31];
    size_t createdCount = 0;

    if (!askPeriod(&month, &year) || !askEmployeeId(employeeId)) {
        return;
    }
    if (isLocked(app, month, year)) {
        puts("Tháng đã chốt, chỉ xem.");
        return;
    }
    if (!askInteger("Số ngày nghỉ trong tháng (0 nếu không có): ", 0, 31, &leaveCountInput)) {
        return;
    }
    size_t leaveCount = (size_t)leaveCountInput;
    for (size_t index = 0; index < leaveCount; index++) { /* Nhập từng ngày nghỉ cụ thể trong tháng */
        Date leaveDate;
        printf("Ngày nghỉ %zu/%zu\n", index + 1, leaveCount);
        if (!askDate("Ngày nghỉ: ", &leaveDate)) {
            return;
        }
        if (leaveDate.month != month || leaveDate.year != year) { /* Phải khớp với kỳ tháng/năm vừa chọn */
            puts("Ngày nghỉ phải nằm trong tháng đã chọn.");
            return;
        }
        leaveDays[index] = leaveDate.day; /* Chỉ lưu ngày (1..31) vào mảng leaveDays */
    }

    AttendanceType leaveType = ATT_UNPAID_LEAVE;
    if (leaveCount) { /* Nếu có ít nhất 1 ngày nghỉ -> hỏi loại nghỉ có lương hay không lương */
        long long selectedType;
        if (!askInteger("Loại nghỉ 1 có lương, 2 không lương: ", 1, 2, &selectedType)) {
            return;
        }
        leaveType = selectedType == 1 ? ATT_PAID_LEAVE : ATT_UNPAID_LEAVE;
    }
    printf("Tạo công thứ Hai đến thứ Bảy, bỏ Chủ nhật, cho %s, kỳ %02d/%d. ",
           employeeId, month, year);
    if (!confirm("Ngày nghỉ đã nhập sẽ dùng loại đã chọn. Y tiếp tục, N hủy: ")) {
        return;
    }

    /* Gọi hàm nghiệp vụ generateMonthlyAttendance để tạo hàng loạt */
    Result result = generateMonthlyAttendance(app, employeeId, month, year, leaveDays,
                                             leaveCount, leaveType, &createdCount);
    if (result == RESULT_OK) {
        printf("Đã tạo %zu bản ghi. Hãy lưu dữ liệu trước khi thoát.\n", createdCount);
    } else {
        reportResult(app, result);
    }
}

/* Vòng lặp menu chức năng Chấm công */
void attendanceMenu(AppData *app) {
    while (!inputEnded()) {
        int menuChoice =
            askMenuChoice("\nCHẤM CÔNG\n"
                          " 1. Tạo công nhanh cả tháng cho một nhân viên\n"
                          " 2. Chấm công một ngày cho một người\n"
                          " 3. Chấm lần lượt theo ngày\n"
                          " 4. Xem công theo ngày\n"
                          " 5. Xem công theo người và tháng\n"
                          " 6. Sửa bản ghi công\n"
                          " 7. Xóa bản ghi công\n"
                          " 8. Kiểm tra ngày công còn thiếu\n"
                          " 0. Quay lại",
                          8);
        if (!menuChoice) {
            return;
        }
        if (menuChoice == 1) { /* Tạo công nhanh cả tháng */
            quickAttendanceForm(app);
        }
        if (menuChoice == 2 || menuChoice == 6) { /* Thêm công 1 người (2) hoặc Sửa công (6) */
            attendanceForm(app, NULL, (Date){0}, menuChoice == 6);
        }
        if (menuChoice == 3) { /* Chấm công lần lượt cho tất cả nhân viên đang làm trong 1 ngày */
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
        if (menuChoice == 4) { /* Xem danh sách chấm công của tất cả nhân viên trong 1 ngày */
            Date selectedDate;
            if (!askDate("Ngày xem: ", &selectedDate)) {
                return;
            }
            printAttendance(app, NULL, 0, 0, &selectedDate, 1);
        }
        if (menuChoice == 5 || menuChoice == 8) { /* Xem công theo người/tháng (5) hoặc Kiểm tra ngày thiếu (8) */
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
            if (menuChoice == 5) {
                printAttendance(app, employeeId, month, year, NULL, 1);
            } else {
                int missingCount = missingAttendance(app, month, year, employeeId, stdout);
                printf("Số ngày thiếu: %d\n", missingCount);
                if (missingCount && confirm("Y bổ sung một bản ghi, N quay lại: ")) { /* Cho phép bổ sung ngay */
                    attendanceForm(app, NULL, (Date){0}, 0);
                }
            }
        }
        if (menuChoice == 7) { /* Xóa một bản ghi chấm công */
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

#ifndef ATTENDANCE_H /* Tránh nạp trùng header chấm công nhiều lần */
#define ATTENDANCE_H
#include "models.h" /* Dùng AppData, Attendance, Date, Result */

/* Tìm vị trí (index) của bản ghi chấm công theo mã nhân viên và ngày; trả về -1 nếu không thấy */
int findAttendanceIndex(const AppData *app, const char *id, Date date);
/* Kiểm tra tính hợp lệ của một bản ghi chấm công (nhân viên tồn tại, đúng ngày làm việc, số giờ đúng quy định) */
Result validateAttendance(AppData *app, const Attendance *record);
/* Lưu bản ghi chấm công vào mảng động (editing = 0: thêm mới; editing = 1: sửa bản ghi cũ) */
Result saveAttendance(AppData *app, const Attendance *record, int editing);
/* Xóa một bản ghi chấm công theo mã nhân viên và ngày (bị chặn nếu tháng đã chốt) */
Result deleteAttendance(AppData *app, const char *id, Date date);
/* Chấm công nhanh cả tháng cho 1 nhân viên: điền đủ 8h các ngày đi làm, trừ danh sách ngày nghỉ leaveDays */
Result generateMonthlyAttendance(AppData *app, const char *id, int month, int year,
                                 const int leaveDays[31], size_t leaveDayCount,
                                 AttendanceType leaveType, size_t *createdCount);
/* Kiểm tra các ngày còn thiếu chấm công trong tháng/năm; nếu stream != NULL sẽ in ra danh sách ngày thiếu; trả về số ngày thiếu */
int missingAttendance(const AppData *app, int month, int year, const char *id, FILE *stream);
#endif

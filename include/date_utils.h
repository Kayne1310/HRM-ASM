#ifndef DATE_UTILS_H /* Tránh nạp trùng header xử lý ngày tháng */
#define DATE_UTILS_H
#include "models.h" /* Dùng kiểu dữ liệu Date */

/* Kiểm tra năm nhuận theo lịch Gregory (chia hết 400 hoặc chia hết 4 nhưng không chia hết 100) */
int isLeapYear(int year);
/* Trả về số ngày của tháng trong năm (có tính tháng 2 năm nhuận 29 ngày) */
int daysInMonth(int month, int year);
/* Kiểm tra một ngày có hợp lệ hay không (năm 2000-2100, tháng 1-12, ngày đúng với tháng) */
int isValidDate(Date date);
/* So sánh hai ngày: trả về -1 nếu a < b, 0 nếu a == b, 1 nếu a > b */
int compareDate(Date a, Date b);
/* Tính thứ trong tuần theo công thức Sakamoto (0 = Chủ nhật, 1 = Thứ Hai, ..., 6 = Thứ Bảy) */
int dayOfWeek(Date date);
/* Kiểm tra có phải ngày làm việc tiêu chuẩn hay không (Chủ nhật = 0 không phải ngày làm việc) */
int isWorkingDay(Date date);
/* Đếm tổng số ngày làm việc chuẩn (từ Thứ 2 đến Thứ 7) trong một tháng/năm */
int workingDaysInMonth(int month, int year);
/* Chuyển chuỗi ngày (yyyy-mm-dd hoặc dd/mm/yyyy) sang struct Date; trả về 1 nếu hợp lệ */
int parseDate(const char *text, Date *date);
/* Chuyển struct Date thành chuỗi chuẩn ISO "yyyy-mm-dd" (cần mảng ít nhất 11 byte) */
void dateIso(Date date, char output[11]);
#endif

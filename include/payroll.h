#ifndef PAYROLL_H /* Tránh nạp trùng header tính lương */
#define PAYROLL_H
#include "models.h" /* Dùng AppData, PayrollInput, Payroll, Result */

/* Tìm vị trí bản ghi thông số lương tháng của nhân viên id; trả về -1 nếu chưa có */
int findInputIndex(const AppData *app, const char *id, int month, int year);
/* Đảm bảo mọi nhân viên làm trong tháng/năm đều đã được tạo bản sao thông số lương mặc định từ hồ sơ */
Result ensurePayrollInputs(AppData *app, int month, int year);
/* Kiểm tra hợp lệ và lưu thông số lương tháng (lương, phụ cấp, thưởng, khấu trừ) vào mảng động */
Result savePayrollInput(AppData *app, const PayrollInput *input);
/* Hàm thuần túy tính lương cho 1 nhân viên bằng số nguyên (tránh sai số float); không nhập phím/ghi tệp */
Result calculatePayroll(const PayrollInput *input, int days, int paidUnits, int overtimeUnits,
                        Payroll *result);
/* Tổng hợp công từ Attendance, kết hợp PayrollInput để tạo bảng lương tạm hoặc lấy bản đã chốt của cả kỳ */
Result buildPayroll(AppData *app, int month, int year, Payroll rows[MAX_EMPLOYEES], size_t *count);
/* Chốt bảng lương tháng: kiểm tra đủ công, lưu nguồn, xuất snapshot payroll_YYYY_MM.txt và khóa tháng */
Result finalizePayroll(AppData *app, int month, int year);
#endif

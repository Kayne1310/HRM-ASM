#ifndef UI_H /* Tránh nạp trùng header giao diện console */
#define UI_H
#include "models.h" /* Dùng AppData, Payroll, Date, Result */

/* In tiêu đề menu và hỏi người dùng chọn số nguyên từ 0 đến maxChoice */
int askMenuChoice(const char *title, int maxChoice);
/* Nhắc nhập mã nhân viên, tự động chuẩn hóa chữ hoa và kiểm tra hợp lệ */
int askEmployeeId(char employeeId[16]);
/* Vòng lặp menu quản lý hồ sơ nhân viên */
void employeeMenu(AppData *app);
/* Vòng lặp menu chấm công */
void attendanceMenu(AppData *app);
/* Vòng lặp menu tính toán, xem và chốt bảng lương */
void payrollMenu(AppData *app);
/* Vòng lặp điều hướng menu chính của toàn bộ chương trình console */
void runConsole(AppData *app);
/* In bảng lương cả kỳ ra màn hình (hỗ trợ phân trang 10 dòng nếu paginate = 1) */
void printPayroll(const Payroll *rows, size_t count, int final, int paginate);
/* In phiếu lương chi tiết của 1 nhân viên (từng khoản lương, phụ cấp, OT, thưởng, khấu trừ) */
void printPayslip(const Payroll *row, int final);
/* Vòng lặp menu tìm kiếm đa điều kiện (nhân viên, công, bảng lương) */
void searchMenu(AppData *app);
/* Vòng lặp menu thống kê nhân sự, ngày công, tăng ca và quỹ lương */
void statisticsMenu(AppData *app);
/* In danh sách nhân viên theo các bộ lọc (id, name, department, status) */
void printEmployees(const AppData *app, const char *id, const char *name, const char *department,
                    int status, int paginate);
/* In danh sách bản ghi chấm công theo nhân viên, tháng/năm hoặc ngày cụ thể */
void printAttendance(const AppData *app, const char *id, int month, int year, const Date *date,
                     int paginate);
/* Hỏi người dùng nhập kỳ lương gồm tháng (1..12) và năm (2000..2100) */
int askPeriod(int *month, int *year);
/* Hiển thị thông báo kết quả (Thành công hoặc chi tiết lỗi trong app->error) */
void reportResult(AppData *app, Result result);
/* Cắt chuỗi UTF-8 an toàn theo độ dài byte tối đa để hiển thị cột không bị cắt ngang ký tự tiếng Việt */
void utf8Prefix(const char *input, size_t bytes, char *output, size_t outputSize);
#endif

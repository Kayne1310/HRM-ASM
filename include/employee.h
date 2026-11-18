#ifndef EMPLOYEE_H /* Tránh nạp trùng header quản lý nhân viên */
#define EMPLOYEE_H
#include "models.h" /* Dùng AppData, Employee, Result */

/* Lấy con trỏ tới hồ sơ nhân viên tại vị trí thứ index trong danh sách liên kết đơn */
Employee *employeeAt(const AppData *app, size_t index);
/* Tìm nhân viên theo mã employeeId trong danh sách liên kết; trả về NULL nếu không thấy */
Employee *findEmployee(const AppData *app, const char *employeeId);
/* Sắp xếp danh sách liên kết nhân viên tăng dần theo mã bằng thuật toán Merge Sort đệ quy */
void sortEmployeesById(AppData *app);
/* Tìm vị trí (chỉ số 0, 1, 2...) của nhân viên trong danh sách liên kết; trả về -1 nếu không có */
int findEmployeeIndex(const AppData *app, const char *id);
/* Kiểm tra hợp lệ và lưu hồ sơ nhân viên (editing = 0: thêm mới vào cuối DS; editing = 1: sửa trực tiếp) */
Result saveEmployee(AppData *app, const Employee *employee, int editing);
/* Xóa một nút nhân viên khỏi danh sách liên kết (chỉ cho xóa khi chưa phát sinh công hay lương) */
Result deleteEmployee(AppData *app, const char *id);
#endif

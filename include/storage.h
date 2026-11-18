#ifndef STORAGE_H /* Tránh nạp trùng header lưu trữ tệp */
#define STORAGE_H
#include "models.h" /* Dùng AppData, Payroll, Result */

/* Nạp toàn bộ dữ liệu (nhân viên, công, thông số, các bảng chốt) từ đĩa vào vùng nhớ tạm rồi mới gán vào app */
Result loadData(AppData *app);
/* Lưu toàn bộ dữ liệu nguồn ra đĩa theo quy trình an toàn 3 bước (.tmp -> .bak -> tệp chính) */
Result saveData(AppData *app);
/* Ghi bảng lương chính thức của một kỳ đã chốt ra tệp snapshot payroll_YYYY_MM.txt */
Result writePayrollSnapshot(AppData *app, int month, int year, const Payroll *rows, size_t count);
/* Xuất bảng lương tháng ra tệp CSV (có UTF-8 BOM để mở Excel không lỗi font tiếng Việt) */
Result exportPayroll(AppData *app, int month, int year, const char *filename, int overwrite);
/* Kiểm tra đường dẫn tệp/thư mục có tồn tại trên ổ đĩa hay không (1 = có, 0 = không) */
int fileExists(const char *filename);
/* Tạo thư mục nếu chưa tồn tại (trả về 1 nếu thành công hoặc đã có sẵn) */
int makeDirectory(const char *directory);
#endif

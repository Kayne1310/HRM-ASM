#ifndef INPUT_H /* Tránh nạp trùng header nhập liệu */
#define INPUT_H
#include "models.h" /* Dùng Date và FILE */

/* Đọc 1 dòng từ stream (stdin hoặc file) bằng fgets; trả về 1 (OK), 0 (hết file/EOF), -1 (dòng quá dài) */
int readLine(FILE *stream, char *buffer, size_t size);
/* Chuyển chuỗi sang số nguyên long long nằm trong khoảng [min, max], không chấp nhận ký tự rác phía sau */
int parseInteger(const char *text, long long min, long long max, long long *value);
/* Chuyển chuỗi giờ (vd: 8, 3.5) sang số đơn vị nửa giờ (16, 7), tránh sai số kiểu float */
int parseHalfHours(const char *text, int maxUnits, int *value);
/* Hiển thị lời nhắc prompt và lặp lại cho tới khi người dùng nhập chuỗi hợp lệ (trả về 0 nếu EOF) */
int askText(const char *prompt, char *output, size_t size, int allowEmpty);
/* Nhắc và đọc số nguyên hợp lệ trong đoạn [min, max] */
int askInteger(const char *prompt, long long min, long long max, long long *value);
/* Nhắc và đọc ngày tháng hợp lệ (dd/mm/yyyy hoặc yyyy-mm-dd) */
int askDate(const char *prompt, Date *date);
/* Nhắc và đọc số giờ (bước 0.5h) quy đổi sang số đơn vị nửa giờ trong đoạn [minUnits, maxUnits] */
int askHours(const char *prompt, int minUnits, int maxUnits, int *value);
/* Hỏi xác nhận Có/Không (Y/N): trả về 1 nếu chọn Y/y, 0 nếu chọn N/n/0 */
int confirm(const char *prompt);
/* Kiểm tra xem luồng nhập liệu (stdin) đã kết thúc (gặp EOF) hay chưa */
int inputEnded(void);
#endif

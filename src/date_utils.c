#include "date_utils.h"
#include <string.h> /* Dùng cho strlen */
#include <ctype.h>  /* Dùng cho isdigit kiểm tra ký tự chữ số */

/* Kiểm tra năm nhuận: chia hết cho 400 HOẶC (chia hết cho 4 và không chia hết cho 100) */
int isLeapYear(int year) {
    return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
}

/* Trả về tổng số ngày của tháng trong năm */
int daysInMonth(int month, int year) {
    /* Mảng hằng lưu số ngày chuẩn của 12 tháng (năm thường) */
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) { /* Tháng ngoài 1..12 thì trả về 0 */
        return 0;
    }
    /* Nếu là tháng 2 của năm nhuận thì cộng thêm 1 ngày (28 + 1 = 29) */
    return days[month - 1] + (month == 2 && isLeapYear(year));
}

/* Kiểm tra struct Date có hợp lệ về năm, tháng và ngày trong tháng đó không */
int isValidDate(Date date) {
    return validMonth(date.month, date.year) && date.day >= 1 &&
           date.day <= daysInMonth(date.month, date.year);
}

/* So sánh 2 ngày: trả về -1 nếu firstDate < secondDate, 0 nếu bằng nhau, 1 nếu lớn hơn */
int compareDate(Date firstDate, Date secondDate) {
    if (firstDate.year != secondDate.year) { /* So sánh năm trước */
        return firstDate.year < secondDate.year ? -1 : 1;
    }
    if (firstDate.month != secondDate.month) { /* Cùng năm thì so sánh tháng */
        return firstDate.month < secondDate.month ? -1 : 1;
    }
    /* Cùng năm và tháng thì so sánh ngày: trả về 1, -1 hoặc 0 */
    return (firstDate.day > secondDate.day) - (firstDate.day < secondDate.day);
}

/* Tính thứ trong tuần của một ngày (0 = Chủ nhật, 1 = Thứ Hai, ..., 6 = Thứ Bảy) */
int dayOfWeek(Date date) {
    /* Công thức Sakamoto cho lịch Gregory: Chủ nhật = 0, thứ Hai = 1. */
    static const int monthOffsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4}; /* Độ lệch từng tháng theo công thức Sakamoto */
    int year = date.year;
    if (!isValidDate(date)) { /* Ngày không hợp lệ thì trả về -1 */
        return -1;
    }
    if (date.month < 3) { /* Theo công thức Sakamoto, tháng 1 và tháng 2 tính lùi năm đi 1 */
        year--;
    }
    return (year + year / 4 - year / 100 + year / 400 + monthOffsets[date.month - 1] + date.day) %
           7;
}

/* Kiểm tra xem có phải ngày đi làm chuẩn (từ Thứ Hai đến Thứ Bảy, khác Chủ nhật = 0) */
int isWorkingDay(Date date) {
    return isValidDate(date) && dayOfWeek(date) != 0;
}

/* Đếm tổng số ngày làm việc tiêu chuẩn (không phải Chủ nhật) trong một tháng */
int workingDaysInMonth(int month, int year) {
    int workingDays = 0;
    if (!validMonth(month, year)) { /* Kỳ không hợp lệ thì công chuẩn = 0 */
        return 0;
    }
    for (int day = 1; day <= daysInMonth(month, year); day++) { /* Duyệt từ ngày 1 đến hết tháng */
        if (isWorkingDay((Date){day, month, year})) {          /* Nếu không phải Chủ nhật thì cộng 1 công */
            workingDays++;
        }
    }
    return workingDays;
}

/* Chuyển đổi chuỗi ngày dạng yyyy-mm-dd hoặc dd/mm/yyyy thành struct Date; trả về 1 nếu hợp lệ */
int parseDate(const char *text, Date *date) {
    Date parsedDate;
    if (strlen(text) != 10) { /* Cả 2 định dạng chuẩn đều phải đúng 10 ký tự */
        return 0;
    }
    if (text[4] == '-' && text[7] == '-') { /* Trường hợp định dạng ISO: yyyy-mm-dd */
        for (int index = 0; index < 10; index++) {
            if (index != 4 && index != 7 && !isdigit((unsigned char)text[index])) { /* Các vị trí khác dấu '-' phải là chữ số */
                return 0;
            }
        }
        /* Tách từng ký tự số và nhân theo trọng số thập phân để lấy năm, tháng, ngày */
        parsedDate.year =
            (text[0] - '0') * 1000 + (text[1] - '0') * 100 + (text[2] - '0') * 10 + text[3] - '0';
        parsedDate.month = (text[5] - '0') * 10 + text[6] - '0';
        parsedDate.day = (text[8] - '0') * 10 + text[9] - '0';
    } else if (text[2] == '/' && text[5] == '/') { /* Trường hợp định dạng Việt Nam: dd/mm/yyyy */
        for (int index = 0; index < 10; index++) {
            if (index != 2 && index != 5 && !isdigit((unsigned char)text[index])) { /* Các vị trí khác dấu '/' phải là chữ số */
                return 0;
            }
        }
        parsedDate.day = (text[0] - '0') * 10 + text[1] - '0';
        parsedDate.month = (text[3] - '0') * 10 + text[4] - '0';
        parsedDate.year =
            (text[6] - '0') * 1000 + (text[7] - '0') * 100 + (text[8] - '0') * 10 + text[9] - '0';
    } else { /* Không khớp định dạng nào */
        return 0;
    }
    if (!isValidDate(parsedDate)) { /* Kiểm tra ngày vừa tách có thực tế không (ví dụ loại 30/02, 31/04) */
        return 0;
    }
    *date = parsedDate; /* Gán kết quả vào con trỏ đầu ra */
    return 1;
}

/* Chuyển struct Date thành chuỗi định dạng chuẩn ISO yyyy-mm-dd */
void dateIso(Date date, char output[11]) {
    snprintf(output, 11, "%04d-%02d-%02d", date.year, date.month, date.day); /* Điền số 0 đứng trước nếu thiếu chữ số */
}

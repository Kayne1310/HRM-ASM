#include "models.h"
#include "date_utils.h"
#include <stdlib.h> /* Dùng cho malloc, realloc, free */
#include <string.h> /* Dùng cho memset, strlen, memmove */
#include <stdarg.h> /* Dùng cho va_list, va_start, va_end (nhận tham số biến đổi như printf) */
#include <stdint.h> /* Dùng cho SIZE_MAX để kiểm tra tràn số nguyên khi cấp phát mảng */
#include <ctype.h>  /* Dùng cho isspace kiểm tra khoảng trắng */

/* Khởi tạo cấu trúc AppData về giá trị 0/NULL ban đầu và lưu đường dẫn thư mục dữ liệu */
void appInit(AppData *app, const char *dataDirectory) {
    memset(app, 0, sizeof *app); /* Gán toàn bộ byte của struct AppData thành 0 (con trỏ thành NULL, biến đếm = 0) */
    snprintf(app->dataDir, sizeof app->dataDir, "%s", dataDirectory); /* Sao chép an toàn tên thư mục vào app->dataDir */
}

/* Giải phóng toàn bộ bộ nhớ động mà chương trình đã cấp phát trong AppData */
void appFree(AppData *app) {
    /* Giải phóng từng nút trong danh sách liên kết đơn, không dùng đệ quy để tránh tràn stack. */
    EmployeeNode *current = app->employeeHead; /* Bắt đầu duyệt từ nút đầu tiên */
    while (current != NULL) {
        EmployeeNode *next = current->next; /* Phải lưu địa chỉ nút kế tiếp TRƯỚC khi free(current) */
        free(current);                      /* Giải phóng nút hiện tại */
        current = next;                     /* Di chuyển sang nút kế tiếp đã lưu */
    }
    app->employeeHead = NULL; /* Đặt con trỏ đầu về NULL để tránh trỏ vào vùng nhớ đã giải phóng (dangling pointer) */
    app->employeeCount = 0;   /* Đặt lại số lượng nhân viên bằng 0 */
    free(app->attendance);    /* Giải phóng mảng động chấm công */
    free(app->inputs);        /* Giải phóng mảng động thông số lương tháng */
    free(app->payrolls);      /* Giải phóng mảng động bảng lương */
    free(app->locks);         /* Giải phóng mảng động danh sách tháng đã chốt */
    app->attendance = NULL;   /* Đưa các con trỏ về NULL sau khi giải phóng */
    app->inputs = NULL;
    app->payrolls = NULL;
    app->locks = NULL;
}

/* Lưu thông báo lỗi vào app->error (hỗ trợ định dạng giống printf) và trả về mã lỗi Result tương ứng */
Result appError(AppData *app, Result code, const char *format, ...) {
    va_list args;                                         /* Khai báo danh sách tham số biến đổi */
    va_start(args, format);                               /* Bắt đầu lấy các tham số truyền sau format */
    vsnprintf(app->error, sizeof app->error, format, args); /* Ghi chuỗi định dạng vào app->error, chống tràn bộ đệm */
    va_end(args);                                         /* Kết thúc sử dụng va_list */
    return code;                                          /* Trả về mã lỗi cho hàm gọi xử lý */
}

/* Tăng dung lượng mảng động khi cần thêm phần tử mới; chỉ thay đổi con trỏ gốc nếu realloc thành công */
int reserveArray(void **array, size_t *capacity, size_t wantedCount, size_t elementSize) {
    size_t newCapacity; /* Sức chứa mới dự kiến */
    void *resizedArray; /* Con trỏ tạm nhận kết quả của realloc */
    if (wantedCount <= *capacity) { /* Nếu sức chứa hiện tại đã đủ thì không cần cấp phát thêm */
        return 1;
    }
    if (!elementSize || wantedCount > SIZE_MAX / elementSize) { /* Kiểm tra chống tràn số khi nhân wantedCount * elementSize */
        return 0;
    }
    newCapacity = *capacity ? *capacity : 64; /* Nếu mảng đang rỗng (cap = 0) thì khởi đầu là 64 phần tử */
    while (newCapacity < wantedCount) {       /* Nhân đôi dung lượng cho đến khi đủ chứa wantedCount */
        if (newCapacity > SIZE_MAX / 2) {     /* Tránh tràn biến size_t khi nhân 2 */
            newCapacity = wantedCount;
            break;
        }
        newCapacity *= 2;
    }
    if (newCapacity > SIZE_MAX / elementSize) { /* Kiểm tra lại lần cuối trước khi nhân với kích thước 1 phần tử */
        newCapacity = wantedCount;
    }
    resizedArray = realloc(*array, newCapacity * elementSize); /* Xin cấp phát vùng nhớ mới kích thước lớn hơn */
    if (!resizedArray) { /* Nếu hết RAM (realloc trả về NULL), giữ nguyên *array cũ và báo thất bại */
        return 0;
    }
    *array = resizedArray;   /* Cập nhật con trỏ mảng gốc trỏ tới vùng nhớ mới */
    *capacity = newCapacity; /* Cập nhật sức chứa mới */
    return 1;
}

/* Kiểm tra chuỗi văn bản UTF-8 hợp lệ: đúng độ dài byte, không chứa ký tự điều khiển hoặc dấu '|' phân cách tệp */
int validText(const char *text, size_t maxBytes, int allowEmpty) {
    size_t length = strlen(text); /* Độ dài chuỗi tính theo byte */
    int nonspace = 0;             /* Cờ kiểm tra xem chuỗi có ít nhất 1 ký tự khác khoảng trắng không */
    if (length > maxBytes || (!allowEmpty && !length)) { /* Quá dài hoặc bị rỗng khi không cho phép rỗng */
        return 0;
    }
    for (size_t index = 0; index < length; index++) {
        unsigned char byte = (unsigned char)text[index]; /* Ép sang unsigned char để so sánh đúng các byte >= 128 */
        if (byte < 32 || byte == 127 || byte == '|') {   /* Cấm ký tự điều khiển ASCII (xuống dòng, tab...) và dấu '|' */
            return 0;
        }
        if (byte != ' ') { /* Ghi nhận đã gặp ký tự không phải dấu cách */
            nonspace = 1;
        }
        /* Kiểm tra cấu trúc UTF-8 đa byte để không lưu ký tự lỗi hoặc thiếu byte. */
        if (byte >= 128) {
            size_t continuationCount;      /* Số byte nối tiếp theo sau byte đầu của ký tự UTF-8 */
            unsigned codePoint, minCodePoint; /* Mã Unicode và giá trị tối thiểu hợp lệ tránh mã hóa dư thừa */
            if (byte >= 0xc2 && byte <= 0xdf) { /* Ký tự UTF-8 gồm 2 byte */
                continuationCount = 1;
                codePoint = byte & 31u;
                minCodePoint = 128;
            } else if (byte >= 0xe0 && byte <= 0xef) { /* Ký tự UTF-8 gồm 3 byte (tiếng Việt có dấu nằm ở nhóm này) */
                continuationCount = 2;
                codePoint = byte & 15u;
                minCodePoint = 2048;
            } else if (byte >= 0xf0 && byte <= 0xf4) { /* Ký tự UTF-8 gồm 4 byte */
                continuationCount = 3;
                codePoint = byte & 7u;
                minCodePoint = 65536;
            } else { /* Byte khởi đầu UTF-8 không hợp lệ */
                return 0;
            }
            if (index + continuationCount >= length) { /* Chuỗi kết thúc đột ngột trước khi đủ số byte nối tiếp */
                return 0;
            }
            for (size_t byteIndex = 1; byteIndex <= continuationCount; byteIndex++) {
                unsigned char continuationByte = (unsigned char)text[index + byteIndex];
                if ((continuationByte & 0xc0) != 0x80) { /* Mỗi byte nối tiếp UTF-8 bắt buộc phải có dạng 10xxxxxx */
                    return 0;
                }
                codePoint = (codePoint << 6) | (continuationByte & 63u); /* Ghép 6 bit dữ liệu vào mã Unicode */
            }
            if (codePoint < minCodePoint || codePoint > 0x10ffff ||
                (codePoint >= 0xd800 && codePoint <= 0xdfff)) { /* Loại bỏ mã Unicode ngoài chuẩn hoặc vùng surrogate */
                return 0;
            }
            index += continuationCount; /* Nhảy qua các byte nối tiếp đã kiểm tra xong */
        }
    }
    return allowEmpty || nonspace; /* Trả về 1 nếu cho phép rỗng hoặc chuỗi có ký tự thực sự (không chỉ toàn dấu cách) */
}

/* Chuẩn hóa mã nhân viên: xóa khoảng trắng thừa đầu/cuối và đổi toàn bộ chữ thường sang chữ HOA */
void normalizeId(char *text) {
    size_t start = 0, length = strlen(text);
    while (start < length && isspace((unsigned char)text[start])) { /* Tìm vị trí ký tự đầu tiên không phải khoảng trắng */
        start++;
    }
    while (length > start && isspace((unsigned char)text[length - 1])) { /* Thu hẹp độ dài từ cuối chuỗi bỏ khoảng trắng */
        length--;
    }
    memmove(text, text + start, length - start); /* Dịch phần chuỗi sạch về đầu mảng (memmove an toàn khi vùng nhớ chồng lấn) */
    text[length - start] = 0;                    /* Đặt ký tự kết thúc chuỗi '\0' */
    for (size_t index = 0; text[index]; index++) { /* Duyệt từng ký tự còn lại để viết hoa */
        if (text[index] >= 'a' && text[index] <= 'z') {
            text[index] = (char)(text[index] - 'a' + 'A'); /* Đổi chữ thường sang chữ hoa theo bảng mã ASCII */
        }
    }
}

/* Kiểm tra mã nhân viên hợp lệ: dài 1..15 ký tự, chỉ chứa A-Z, 0-9, dấu gạch dưới '_' hoặc gạch ngang '-' */
int validId(const char *text) {
    size_t length = strlen(text);
    if (!length || length > 15) { /* Không cho phép rỗng hoặc vượt quá 15 ký tự */
        return 0;
    }
    for (size_t index = 0; index < length; index++) {
        if (!((text[index] >= 'A' && text[index] <= 'Z') ||
              (text[index] >= '0' && text[index] <= '9') || text[index] == '_' ||
              text[index] == '-')) {
            return 0; /* Gặp ký tự lạ -> không hợp lệ */
        }
    }
    return 1;
}

/* Kiểm tra kỳ tháng/năm hợp lệ (tháng 1..12, năm 2000..2100) */
int validMonth(int month, int year) {
    return month >= 1 && month <= 12 && year >= 2000 && year <= 2100;
}

/* Kiểm tra xem tháng và năm truyền vào có nằm trong mảng các kỳ đã chốt khóa hay không */
int isLocked(const AppData *app, int month, int year) {
    for (size_t index = 0; index < app->lockCount; index++) {
        if (app->locks[index].month == month && app->locks[index].year == year) {
            return 1; /* Đã tìm thấy kỳ này trong danh sách khóa */
        }
    }
    return 0; /* Chưa chốt */
}

/* Kiểm tra nhân viên có đang thuộc thời gian làm việc vào ngày date hay không */
int employeeOnDate(const Employee *employee, Date date) {
    /* Ngày xét phải >= ngày vào làm VÀ (chưa nghỉ việc HOẶC <= ngày làm cuối) */
    return compareDate(date, employee->startDate) >= 0 &&
           (!employee->hasEndDate || compareDate(date, employee->endDate) <= 0);
}

/* Kiểm tra nhân viên có làm việc ít nhất 1 ngày trong tháng/năm được xét hay không */
int employeeInMonth(const Employee *employee, int month, int year) {
    Date first = {1, month, year}, last = {daysInMonth(month, year), month, year}; /* Ngày đầu tháng và ngày cuối tháng */
    /* Ngày vào làm <= ngày cuối tháng VÀ (chưa nghỉ HOẶC ngày nghỉ >= ngày đầu tháng) */
    return compareDate(employee->startDate, last) <= 0 &&
           (!employee->hasEndDate || compareDate(employee->endDate, first) >= 0);
}

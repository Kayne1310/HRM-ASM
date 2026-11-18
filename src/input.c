#include "input.h"
#include "date_utils.h"
#include <string.h> /* Dùng cho strlen, strcmp */
#include <stdlib.h> /* Dùng cho strtoll (chuyển chuỗi sang số nguyên an toàn) */
#include <errno.h>  /* Dùng cho errno, ERANGE phát hiện số bị tràn khi dùng strtoll */
#include <ctype.h>  /* Dùng cho isspace, isdigit */

static int ended = 0; /* Biến tĩnh ghi nhớ luồng nhập (stdin) đã gặp EOF hay chưa */

/* Trả về 1 nếu người dùng đã kết thúc luồng nhập (nhấn Ctrl+Z/Ctrl+D hoặc hết file input) */
int inputEnded(void) {
    return ended;
}

/* Đọc hết một dòng từ stream. Dòng quá dài bị xả bỏ phần dư để tránh trôi lệnh ở lần nhập kế tiếp. */
int readLine(FILE *stream, char *buffer, size_t bufferSize) {
    size_t length;
    int character;
    if (bufferSize < 2 || !fgets(buffer, (int)bufferSize, stream)) { /* Đọc thất bại hoặc chạm EOF */
        return 0;
    }
    length = strlen(buffer);
    if (length && buffer[length - 1] == '\n') { /* Nếu đọc trọn vẹn cả ký tự xuống dòng '\n' */
        buffer[--length] = 0;                   /* Xóa ký tự '\n' ở cuối chuỗi */
    } else if (!feof(stream)) {                 /* Nếu chưa có '\n' và chưa hết luồng -> dòng nhập dài hơn bufferSize */
        character = fgetc(stream);              /* Đọc tiếp 1 ký tự xem có phải '\n' ngay sát đó không */
        if (character != '\n' && character != EOF) {
            while ((character = fgetc(stream)) != '\n' && character != EOF) {
                /* Đọc bỏ toàn bộ phần ký tự thừa còn kẹt trong bộ đệm cho tới hết dòng */
            }
            buffer[0] = 0; /* Xóa trắng buffer */
            return -1;     /* Báo mã lỗi -1: dòng quá dài */
        }
    }
    if (length && buffer[length - 1] == '\r') { /* Xử lý ký tự '\r' của định dạng xuống dòng Windows (\r\n) */
        buffer[length - 1] = 0;
    }
    return 1; /* Đọc dòng thành công */
}

/* Chuyển chuỗi thành số nguyên long long; kiểm tra chặt chẽ ký tự rác (vd: 123abc) và giới hạn [minValue, maxValue]. */
int parseInteger(const char *text, long long minValue, long long maxValue, long long *value) {
    char *endPosition;     /* Con trỏ trỏ tới ký tự đầu tiên không phải số sau khi strtoll chạy */
    long long parsedValue; /* Giá trị số chuyển đổi được */
    while (isspace((unsigned char)*text)) { /* Bỏ qua các khoảng trắng ở đầu chuỗi */
        text++;
    }
    /* Ký tự đầu sau khoảng trắng phải là dấu '-', '+' hoặc chữ số */
    if (!*text || (*text != '-' && *text != '+' && !isdigit((unsigned char)*text))) {
        return 0;
    }
    errno = 0; /* Đặt lại cờ lỗi hệ thống trước khi gọi strtoll */
    parsedValue = strtoll(text, &endPosition, 10); /* Chuyển chuỗi hệ cơ số 10 sang long long */
    if (errno == ERANGE || endPosition == text) { /* Bị tràn phạm vi long long hoặc không đọc được số nào */
        return 0;
    }
    while (isspace((unsigned char)*endPosition)) { /* Bỏ qua khoảng trắng thừa ở cuối chuỗi */
        endPosition++;
    }
    /* Nếu vẫn còn ký tự lạ phía sau hoặc số nằm ngoài khoảng [minValue, maxValue] -> báo sai */
    if (*endPosition || parsedValue < minValue || parsedValue > maxValue) {
        return 0;
    }
    *value = parsedValue; /* Lưu kết quả hợp lệ ra ngoài */
    return 1;
}

/* Đổi trực tiếp chuỗi giờ (vd "3.5" hoặc "3,5") thành 7 đơn vị nửa giờ, hoàn toàn không dùng float để tránh sai số. */
int parseHalfHours(const char *text, int maxValue, int *value) {
    unsigned units = 0; /* Phần giờ nguyên */
    int half = 0;       /* Phần lẻ nửa giờ (0 hoặc 1) */
    while (isspace((unsigned char)*text)) { /* Bỏ qua khoảng trắng đầu */
        text++;
    }
    if (!isdigit((unsigned char)*text)) { /* Bắt buộc phải bắt đầu bằng chữ số */
        return 0;
    }
    while (isdigit((unsigned char)*text)) { /* Đọc từng chữ số của phần giờ nguyên */
        units = units * 10 + (unsigned)(*text++ - '0');
        if (units > 1000) { /* Chặn số quá lớn gây tràn */
            return 0;
        }
    }
    if (*text == '.' || *text == ',') { /* Nếu có phần thập phân (.5, .0 hoặc ,5, ,0) */
        text++;
        if (*text == '5') {
            half = 1; /* .5 giờ tương ứng +1 đơn vị nửa giờ */
            text++;
        } else if (*text == '0') {
            text++; /* .0 giờ thì không cộng thêm */
        } else {
            return 0; /* Không chấp nhận bước khác ngoài .0 và .5 (vd .25, .7) */
        }
    }
    while (isspace((unsigned char)*text)) { /* Bỏ qua khoảng trắng cuối */
        text++;
    }
    /* Không được còn ký tự rác phía sau và tổng đơn vị nửa giờ không vượt quá trần maxValue */
    if (*text || units * 2 + (unsigned)half > (unsigned)maxValue) {
        return 0;
    }
    *value = (int)(units * 2) + half; /* Quy đổi: 1 giờ = 2 đơn vị nửa giờ */
    return 1;
}

/* In lời nhắc và lặp lại yêu cầu nhập chuỗi cho tới khi đúng quy chuẩn hoặc gặp EOF */
int askText(const char *prompt, char *buffer, size_t bufferSize, int allowEmpty) {
    for (;;) { /* Vòng lặp vô hạn cho đến khi nhập đúng hoặc hết luồng nhập */
        int readStatus;
        printf("%s", prompt);
        fflush(stdout); /* Đẩy nội dung lời nhắc ra màn hình ngay lập tức trước khi chờ bàn phím */
        readStatus = readLine(stdin, buffer, bufferSize);
        if (!readStatus) { /* Gặp EOF (người dùng thoát luồng nhập) */
            ended = 1;
            return 0;
        }
        if (readStatus < 0) { /* Chuỗi nhập vào vượt kích thước bộ đệm */
            puts("Dòng nhập quá dài. Hãy nhập lại.");
            continue;
        }
        if (!validText(buffer, bufferSize - 1, allowEmpty)) { /* Kiểm tra chuỗi rỗng, UTF-8 và cấm dấu '|' */
            puts("Chuỗi rỗng hoặc có ký tự không hợp lệ.");
            continue;
        }
        return 1; /* Hợp lệ */
    }
}

/* Hỏi và kiểm tra cho tới khi nhận được số nguyên hợp lệ trong đoạn [minValue, maxValue] */
int askInteger(const char *prompt, long long minValue, long long maxValue, long long *value) {
    char buffer[128];
    for (;;) {
        if (!askText(prompt, buffer, sizeof buffer, 0)) { /* Nếu gặp EOF thì dừng */
            return 0;
        }
        if (parseInteger(buffer, minValue, maxValue, value)) { /* Phân tích và kiểm tra miền giá trị */
            return 1;
        }
        printf("Nhập số nguyên trong [%lld, %lld].\n", minValue, maxValue);
    }
}

/* Hỏi và kiểm tra cho tới khi nhận được ngày tháng hợp lệ */
int askDate(const char *prompt, Date *date) {
    char buffer[64];
    for (;;) {
        if (!askText(prompt, buffer, sizeof buffer, 0)) {
            return 0;
        }
        if (parseDate(buffer, date)) {
            return 1;
        }
        puts("Ngày không hợp lệ. Dùng dd/mm/yyyy hoặc yyyy-mm-dd, năm 2000 đến 2100.");
    }
}

/* Hỏi và kiểm tra số giờ hợp lệ (bước 0.5h), trả về số đơn vị nửa giờ trong đoạn [minValue, maxValue] */
int askHours(const char *prompt, int minValue, int maxValue, int *value) {
    char buffer[64];
    for (;;) {
        if (!askText(prompt, buffer, sizeof buffer, 0)) {
            return 0;
        }
        if (parseHalfHours(buffer, maxValue, value) && *value >= minValue) {
            return 1;
        }
        printf("Giờ trong [%.1f, %.1f], bước 0.5.\n", minValue / 2.0, maxValue / 2.0);
    }
}

/* Hỏi xác nhận Có/Không (Y hoặc N) */
int confirm(const char *prompt) {
    char buffer[16];
    for (;;) {
        if (!askText(prompt, buffer, sizeof buffer, 0)) {
            return 0;
        }
        if (!strcmp(buffer, "Y") || !strcmp(buffer, "y")) { /* Đồng ý */
            return 1;
        }
        if (!strcmp(buffer, "N") || !strcmp(buffer, "n") || !strcmp(buffer, "0")) { /* Từ chối / Hủy */
            return 0;
        }
        puts("Chọn Y hoặc N.");
    }
}

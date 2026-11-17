#include "input.h"
#include "date_utils.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
static int ended = 0;

int inputEnded(void) {
    return ended;
}

/* Đọc hết một dòng. Dòng quá dài bị bỏ phần dư trước khi yêu cầu nhập lại. */
int readLine(FILE *stream, char *buffer, size_t bufferSize) {
    size_t length;
    int character;
    if (bufferSize < 2 || !fgets(buffer, (int)bufferSize, stream)) {
        return 0;
    }
    length = strlen(buffer);
    if (length && buffer[length - 1] == '\n') {
        buffer[--length] = 0;
    } else if (!feof(stream)) {
        character = fgetc(stream);
        if (character != '\n' && character != EOF) {
            while ((character = fgetc(stream)) != '\n' && character != EOF) {
            }
            buffer[0] = 0;
            return -1;
        }
    }
    if (length && buffer[length - 1] == '\r') {
        buffer[length - 1] = 0;
    }
    return 1;
}

/* Kiểm tra cả phần ký tự còn dư, không chấp nhận 123abc hoặc số bị tràn. */
int parseInteger(const char *text, long long minValue, long long maxValue, long long *value) {
    char *endPosition;
    long long parsedValue;
    while (isspace((unsigned char)*text)) {
        text++;
    }
    if (!*text || (*text != '-' && *text != '+' && !isdigit((unsigned char)*text))) {
        return 0;
    }
    errno = 0;
    parsedValue = strtoll(text, &endPosition, 10);
    if (errno == ERANGE || endPosition == text) {
        return 0;
    }
    while (isspace((unsigned char)*endPosition)) {
        endPosition++;
    }
    if (*endPosition || parsedValue < minValue || parsedValue > maxValue) {
        return 0;
    }
    *value = parsedValue;
    return 1;
}

/* Đổi trực tiếp 3.5 giờ thành 7 đơn vị nửa giờ, tránh sai số dấu phẩy động. */
int parseHalfHours(const char *text, int maxValue, int *value) {
    unsigned units = 0;
    int half = 0;
    while (isspace((unsigned char)*text)) {
        text++;
    }
    if (!isdigit((unsigned char)*text)) {
        return 0;
    }
    while (isdigit((unsigned char)*text)) {
        units = units * 10 + (unsigned)(*text++ - '0');
        if (units > 1000) {
            return 0;
        }
    }
    if (*text == '.' || *text == ',') {
        text++;
        if (*text == '5') {
            half = 1;
            text++;
        } else if (*text == '0') {
            text++;
        } else {
            return 0;
        }
    }
    while (isspace((unsigned char)*text)) {
        text++;
    }
    if (*text || units * 2 + (unsigned)half > (unsigned)maxValue) {
        return 0;
    }
    *value = (int)(units * 2) + half;
    return 1;
}

int askText(const char *prompt, char *buffer, size_t bufferSize, int allowEmpty) {
    for (;;) {
        int readStatus;
        printf("%s", prompt);
        fflush(stdout);
        readStatus = readLine(stdin, buffer, bufferSize);
        if (!readStatus) {
            ended = 1;
            return 0;
        }
        if (readStatus < 0) {
            puts("Dòng nhập quá dài. Hãy nhập lại.");
            continue;
        }
        if (!validText(buffer, bufferSize - 1, allowEmpty)) {
            puts("Chuỗi rỗng hoặc có ký tự không hợp lệ.");
            continue;
        }
        return 1;
    }
}

int askInteger(const char *prompt, long long minValue, long long maxValue, long long *value) {
    char buffer[128];
    for (;;) {
        if (!askText(prompt, buffer, sizeof buffer, 0)) {
            return 0;
        }
        if (parseInteger(buffer, minValue, maxValue, value)) {
            return 1;
        }
        printf("Nhập số nguyên trong [%lld, %lld].\n", minValue, maxValue);
    }
}

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

int confirm(const char *prompt) {
    char buffer[16];
    for (;;) {
        if (!askText(prompt, buffer, sizeof buffer, 0)) {
            return 0;
        }
        if (!strcmp(buffer, "Y") || !strcmp(buffer, "y")) {
            return 1;
        }
        if (!strcmp(buffer, "N") || !strcmp(buffer, "n") || !strcmp(buffer, "0")) {
            return 0;
        }
        puts("Chọn Y hoặc N.");
    }
}

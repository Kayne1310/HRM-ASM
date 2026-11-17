#include "models.h"
#include "date_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>

void appInit(AppData *app, const char *dataDirectory) {
    memset(app, 0, sizeof *app);
    snprintf(app->dataDir, sizeof app->dataDir, "%s", dataDirectory);
}

void appFree(AppData *app) {
    /* Giải phóng từng nút, không dùng đệ quy để tránh tích lũy stack. */
    EmployeeNode *current = app->employeeHead;
    while (current != NULL) {
        EmployeeNode *next = current->next;
        free(current);
        current = next;
    }
    app->employeeHead = NULL;
    app->employeeCount = 0;
    free(app->attendance);
    free(app->inputs);
    free(app->payrolls);
    free(app->locks);
    app->attendance = NULL;
    app->inputs = NULL;
    app->payrolls = NULL;
    app->locks = NULL;
}

Result appError(AppData *app, Result code, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(app->error, sizeof app->error, format, args);
    va_end(args);
    return code;
}

int reserveArray(void **array, size_t *capacity, size_t wantedCount, size_t elementSize) {
    size_t newCapacity;
    void *resizedArray;
    if (wantedCount <= *capacity) {
        return 1;
    }
    if (!elementSize || wantedCount > SIZE_MAX / elementSize) {
        return 0;
    }
    newCapacity = *capacity ? *capacity : 64;
    while (newCapacity < wantedCount) {
        if (newCapacity > SIZE_MAX / 2) {
            newCapacity = wantedCount;
            break;
        }
        newCapacity *= 2;
    }
    if (newCapacity > SIZE_MAX / elementSize) {
        newCapacity = wantedCount;
    }
    resizedArray = realloc(*array, newCapacity * elementSize);
    if (!resizedArray) {
        return 0;
    }
    *array = resizedArray;
    *capacity = newCapacity;
    return 1;
}

int validText(const char *text, size_t maxBytes, int allowEmpty) {
    size_t length = strlen(text);
    int nonspace = 0;
    if (length > maxBytes || (!allowEmpty && !length)) {
        return 0;
    }
    for (size_t index = 0; index < length; index++) {
        unsigned char byte = (unsigned char)text[index];
        if (byte < 32 || byte == 127 || byte == '|') {
            return 0;
        }
        if (byte != ' ') {
            nonspace = 1;
        }
        /* Kiểm tra UTF-8 để không lưu ký tự thiếu byte, mã quá dài hoặc mã không hợp lệ. */
        if (byte >= 128) {
            size_t continuationCount;
            unsigned codePoint, minCodePoint;
            if (byte >= 0xc2 && byte <= 0xdf) {
                continuationCount = 1;
                codePoint = byte & 31u;
                minCodePoint = 128;
            } else if (byte >= 0xe0 && byte <= 0xef) {
                continuationCount = 2;
                codePoint = byte & 15u;
                minCodePoint = 2048;
            } else if (byte >= 0xf0 && byte <= 0xf4) {
                continuationCount = 3;
                codePoint = byte & 7u;
                minCodePoint = 65536;
            } else {
                return 0;
            }
            if (index + continuationCount >= length) {
                return 0;
            }
            for (size_t byteIndex = 1; byteIndex <= continuationCount; byteIndex++) {
                unsigned char continuationByte = (unsigned char)text[index + byteIndex];
                if ((continuationByte & 0xc0) != 0x80) {
                    return 0;
                }
                codePoint = (codePoint << 6) | (continuationByte & 63u);
            }
            if (codePoint < minCodePoint || codePoint > 0x10ffff ||
                (codePoint >= 0xd800 && codePoint <= 0xdfff)) {
                return 0;
            }
            index += continuationCount;
        }
    }
    return allowEmpty || nonspace;
}

void normalizeId(char *text) {
    size_t start = 0, length = strlen(text);
    while (start < length && isspace((unsigned char)text[start])) {
        start++;
    }
    while (length > start && isspace((unsigned char)text[length - 1])) {
        length--;
    }
    memmove(text, text + start, length - start);
    text[length - start] = 0;
    for (size_t index = 0; text[index]; index++) {
        if (text[index] >= 'a' && text[index] <= 'z') {
            text[index] = (char)(text[index] - 'a' + 'A');
        }
    }
}

int validId(const char *text) {
    size_t length = strlen(text);
    if (!length || length > 15) {
        return 0;
    }
    for (size_t index = 0; index < length; index++) {
        if (!((text[index] >= 'A' && text[index] <= 'Z') ||
              (text[index] >= '0' && text[index] <= '9') || text[index] == '_' ||
              text[index] == '-')) {
            return 0;
        }
    }
    return 1;
}

int validMonth(int month, int year) {
    return month >= 1 && month <= 12 && year >= 2000 && year <= 2100;
}

int isLocked(const AppData *app, int month, int year) {
    for (size_t index = 0; index < app->lockCount; index++) {
        if (app->locks[index].month == month && app->locks[index].year == year) {
            return 1;
        }
    }
    return 0;
}

int employeeOnDate(const Employee *employee, Date date) {
    return compareDate(date, employee->startDate) >= 0 &&
           (!employee->hasEndDate || compareDate(date, employee->endDate) <= 0);
}

int employeeInMonth(const Employee *employee, int month, int year) {
    Date first = {1, month, year}, last = {daysInMonth(month, year), month, year};
    return compareDate(employee->startDate, last) <= 0 &&
           (!employee->hasEndDate || compareDate(employee->endDate, first) >= 0);
}

#include "date_utils.h"
#include <string.h>
#include <ctype.h>

int isLeapYear(int year) {
    return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
}

int daysInMonth(int month, int year) {
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) {
        return 0;
    }
    return days[month - 1] + (month == 2 && isLeapYear(year));
}

int isValidDate(Date date) {
    return validMonth(date.month, date.year) && date.day >= 1 &&
           date.day <= daysInMonth(date.month, date.year);
}

int compareDate(Date firstDate, Date secondDate) {
    if (firstDate.year != secondDate.year) {
        return firstDate.year < secondDate.year ? -1 : 1;
    }
    if (firstDate.month != secondDate.month) {
        return firstDate.month < secondDate.month ? -1 : 1;
    }
    return (firstDate.day > secondDate.day) - (firstDate.day < secondDate.day);
}

int dayOfWeek(Date date) {
    /* Công thức Sakamoto cho lịch Gregory: Chủ nhật = 0, thứ Hai = 1. */ static const int
        monthOffsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    int year = date.year;
    if (!isValidDate(date)) {
        return -1;
    }
    if (date.month < 3) {
        year--;
    }
    return (year + year / 4 - year / 100 + year / 400 + monthOffsets[date.month - 1] + date.day) %
           7;
}

int isWorkingDay(Date date) {
    return isValidDate(date) && dayOfWeek(date) != 0;
}

int workingDaysInMonth(int month, int year) {
    int workingDays = 0;
    if (!validMonth(month, year)) {
        return 0;
    }
    for (int day = 1; day <= daysInMonth(month, year); day++) {
        if (isWorkingDay((Date){day, month, year})) {
            workingDays++;
        }
    }
    return workingDays;
}

int parseDate(const char *text, Date *date) {
    Date parsedDate;
    if (strlen(text) != 10) {
        return 0;
    }
    if (text[4] == '-' && text[7] == '-') {
        for (int index = 0; index < 10; index++) {
            if (index != 4 && index != 7 && !isdigit((unsigned char)text[index])) {
                return 0;
            }
        }
        parsedDate.year =
            (text[0] - '0') * 1000 + (text[1] - '0') * 100 + (text[2] - '0') * 10 + text[3] - '0';
        parsedDate.month = (text[5] - '0') * 10 + text[6] - '0';
        parsedDate.day = (text[8] - '0') * 10 + text[9] - '0';
    } else if (text[2] == '/' && text[5] == '/') {
        for (int index = 0; index < 10; index++) {
            if (index != 2 && index != 5 && !isdigit((unsigned char)text[index])) {
                return 0;
            }
        }
        parsedDate.day = (text[0] - '0') * 10 + text[1] - '0';
        parsedDate.month = (text[3] - '0') * 10 + text[4] - '0';
        parsedDate.year =
            (text[6] - '0') * 1000 + (text[7] - '0') * 100 + (text[8] - '0') * 10 + text[9] - '0';
    } else {
        return 0;
    }
    if (!isValidDate(parsedDate)) {
        return 0;
    }
    *date = parsedDate;
    return 1;
}

void dateIso(Date date, char output[11]) {
    snprintf(output, 11, "%04d-%02d-%02d", date.year, date.month, date.day);
}

#ifndef DATE_UTILS_H
#define DATE_UTILS_H
#include "models.h"
int isLeapYear(int year);
int daysInMonth(int month, int year);
int isValidDate(Date date);
int compareDate(Date a, Date b);
int dayOfWeek(Date date);
/* Sunday = 0 */ int isWorkingDay(Date date);
int workingDaysInMonth(int month, int year);
int parseDate(const char *text, Date *date);
void dateIso(Date date, char output[11]);
#endif

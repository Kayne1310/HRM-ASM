#ifndef ATTENDANCE_H
#define ATTENDANCE_H
#include "models.h"
int findAttendanceIndex(const AppData *app, const char *id, Date date);
Result validateAttendance(AppData *app, const Attendance *record);
Result saveAttendance(AppData *app, const Attendance *record, int editing);
Result deleteAttendance(AppData *app, const char *id, Date date);
Result generateMonthlyAttendance(AppData *app, const char *id, int month, int year,
                                 const int leaveDays[31], size_t leaveDayCount,
                                 AttendanceType leaveType, size_t *createdCount);
/* Prints missing days if stream is non-NULL; returns number of missing days. */ int
missingAttendance(const AppData *app, int month, int year, const char *id, FILE *stream);
#endif

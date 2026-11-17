#ifndef UI_H
#define UI_H
#include "models.h"
int askMenuChoice(const char *title, int maxChoice);
int askEmployeeId(char employeeId[16]);
void employeeMenu(AppData *app);
void attendanceMenu(AppData *app);
void payrollMenu(AppData *app);
void runConsole(AppData *app);
void printPayroll(const Payroll *rows, size_t count, int final, int paginate);
void printPayslip(const Payroll *row, int final);
void searchMenu(AppData *app);
void statisticsMenu(AppData *app);
void printEmployees(const AppData *app, const char *id, const char *name, const char *department,
                    int status, int paginate);
void printAttendance(const AppData *app, const char *id, int month, int year, const Date *date,
                     int paginate);
int askPeriod(int *month, int *year);
void reportResult(AppData *app, Result result);
void utf8Prefix(const char *input, size_t bytes, char *output, size_t outputSize);
#endif

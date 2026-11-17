#ifndef STORAGE_H
#define STORAGE_H
#include "models.h"
Result loadData(AppData *app);
Result saveData(AppData *app);
Result writePayrollSnapshot(AppData *app, int month, int year, const Payroll *rows, size_t count);
Result exportPayroll(AppData *app, int month, int year, const char *filename, int overwrite);
int fileExists(const char *filename);
int makeDirectory(const char *directory);
#endif

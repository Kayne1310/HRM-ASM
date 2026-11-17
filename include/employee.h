#ifndef EMPLOYEE_H
#define EMPLOYEE_H
#include "models.h"
Employee *employeeAt(const AppData *app, size_t index);
Employee *findEmployee(const AppData *app, const char *employeeId);
void sortEmployeesById(AppData *app);
int findEmployeeIndex(const AppData *app, const char *id);
Result saveEmployee(AppData *app, const Employee *employee, int editing);
Result deleteEmployee(AppData *app, const char *id);
#endif

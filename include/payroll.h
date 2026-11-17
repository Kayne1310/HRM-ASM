#ifndef PAYROLL_H
#define PAYROLL_H
#include "models.h"
int findInputIndex(const AppData *app, const char *id, int month, int year);
Result ensurePayrollInputs(AppData *app, int month, int year);
Result savePayrollInput(AppData *app, const PayrollInput *input);
Result calculatePayroll(const PayrollInput *input, int days, int paidUnits, int overtimeUnits,
                        Payroll *result);
Result buildPayroll(AppData *app, int month, int year, Payroll rows[MAX_EMPLOYEES], size_t *count);
Result finalizePayroll(AppData *app, int month, int year);
#endif

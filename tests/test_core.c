#include "models.h"
#include "date_utils.h"
#include "input.h"
#include "employee.h"
#include "attendance.h"
#include "payroll.h"
#include "storage.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
static int checks = 0, failures = 0;
#define CHECK(expr)                                                                                \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(expr)) {                                                                             \
            failures++;                                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr);                        \
        }                                                                                          \
    } while (0)

static Employee employee(const char *id, Date start, long long salary) {
    Employee e = {0};
    strcpy(e.id, id);
    strcpy(e.fullName, "Nguyễn Văn An");
    strcpy(e.department, "Nhân sự");
    strcpy(e.position, "Chuyên viên");
    e.startDate = start;
    e.baseSalary = salary;
    e.allowance = salary / 10;
    e.status = 1;
    return e;
}

static Attendance attendance(const char *id, Date d) {
    Attendance r = {0};
    strcpy(r.employeeId, id);
    r.date = d;
    r.type = ATT_WORK;
    r.regularHalfHours = 16;
    return r;
}

static void datesAndInput(void) {
    Date d;
    long long n;
    int q;
    char s[16];
    CHECK(isLeapYear(2000));
    CHECK(!isLeapYear(2100));
    CHECK(isLeapYear(2028));
    CHECK(!isLeapYear(2027));
    CHECK(parseDate("29/02/2028", &d));
    CHECK(!parseDate("29/02/2027", &d));
    CHECK(!parseDate("2026-04-31", &d));
    CHECK(!parseDate("1/01/2026", &d));
    CHECK(!parseDate("2026-01-01x", &d));
    CHECK(dayOfWeek((Date){1, 3, 2026}) == 0);
    CHECK(dayOfWeek((Date){2, 3, 2026}) == 1);
    CHECK(dayOfWeek((Date){5, 11, 2026}) == 4);
    CHECK(workingDaysInMonth(3, 2026) == 26);
    CHECK(workingDaysInMonth(2, 2028) == 25);
    CHECK(!isWorkingDay((Date){1, 3, 2026}));
    CHECK(compareDate((Date){31, 12, 2025}, (Date){1, 1, 2026}) < 0);
    dateIso((Date){2, 3, 2026}, s);
    CHECK(!strcmp(s, "2026-03-02"));
    CHECK(parseInteger(" 123 ", 0, 1000, &n) && n == 123);
    CHECK(!parseInteger("12x", 0, 1000, &n));
    CHECK(!parseInteger("999999999999999999999", 0, LLONG_MAX, &n));
    CHECK(!parseInteger("", 0, 100, &n));
    CHECK(!parseInteger("-1", 0, 100, &n));
    CHECK(parseHalfHours("3.5", 16, &q) && q == 7);
    CHECK(parseHalfHours("0", 8, &q) && q == 0);
    CHECK(parseHalfHours("0,5", 16, &q) && q == 1);
    CHECK(!parseHalfHours("0.25", 16, &q));
    CHECK(!parseHalfHours("8.5", 16, &q));
    CHECK(!parseHalfHours("-1", 16, &q));
    CHECK(validText("Nguyễn Văn An", 100, 0));
    CHECK(!validText("A|B", 100, 0));
    CHECK(!validText("   ", 100, 0));
    CHECK(!validText("\xc0\xaf", 100, 0));
    CHECK(!validText("\xed\xa0\x80", 100, 0));
    CHECK(validText("", 0, 1));
    strcpy(s, " nv001 ");
    normalizeId(s);
    CHECK(!strcmp(s, "NV001"));
    CHECK(validId(s));
    CHECK(!validId("nv001"));
    FILE *f = tmpfile();
    CHECK(f != NULL);
    if (f) {
        fputs("abcdef\nok\n1234\nlast", f);
        rewind(f);
        char b[5];
        CHECK(readLine(f, b, sizeof b) == -1);
        CHECK(readLine(f, b, sizeof b) == 1 && !strcmp(b, "ok"));
        CHECK(readLine(f, b, sizeof b) == 1 && !strcmp(b, "1234"));
        CHECK(readLine(f, b, sizeof b) == 1 && !strcmp(b, "last"));
        CHECK(readLine(f, b, sizeof b) == 0);
        fclose(f);
    }
}

static void salaryMath(void) {
    PayrollInput p = {0};
    Payroll r;
    strcpy(p.employeeId, "NV001");
    p.month = 3;
    p.year = 2026;
    p.baseSalary = 13000000;
    p.allowance = 1300000;
    p.bonus = 500000;
    p.deduction = 200000;
    CHECK(calculatePayroll(&p, 26, 384, 20, &r) == RESULT_OK);
    CHECK(r.regularPay == 12000000);
    CHECK(r.allowancePay == 1200000);
    CHECK(r.overtimePay == 937500);
    CHECK(r.netPay == 14437500);
    CHECK(calculatePayroll(&p, 26, 416, 0, &r) == RESULT_OK && r.regularPay == 13000000 &&
          r.allowancePay == 1300000);
    p.bonus = 0;
    p.deduction = 0;
    CHECK(calculatePayroll(&p, 26, 8, 0, &r) == RESULT_OK && r.regularPay == 250000);
    CHECK(calculatePayroll(&p, 26, 0, 0, &r) == RESULT_OK && r.netPay == 0);
    p.deduction = 1;
    CHECK(calculatePayroll(&p, 26, 0, 0, &r) == RESULT_INVALID);
    CHECK(calculatePayroll(&p, 0, 0, 0, &r) == RESULT_INVALID);
    CHECK(calculatePayroll(&p, 26, 417, 0, &r) == RESULT_INVALID);
    CHECK(calculatePayroll(&p, 26, 0, 209, &r) == RESULT_INVALID);
    p.deduction = 0;
    p.baseSalary = 1;
    p.allowance = 0;
    CHECK(calculatePayroll(&p, 1, 8, 0, &r) == RESULT_OK && r.regularPay == 1);
    p.baseSalary = MAX_MONEY;
    CHECK(calculatePayroll(&p, 31, 496, 248, &r) == RESULT_OK && r.netPay == 1750000000LL);
}

static void application(const char *directory) {
    AppData *a = calloc(1, sizeof *a), *b = calloc(1, sizeof *b);
    CHECK(a && b);
    if (!a || !b) {
        free(a);
        free(b);
        return;
    }
    appInit(a, directory);
    CHECK(loadData(a) == RESULT_OK);
    Employee e = employee("NV001", (Date){5, 1, 2026}, 13000000);
    CHECK(saveEmployee(a, &e, 0) == RESULT_OK);
    CHECK(saveEmployee(a, &e, 0) == RESULT_DUPLICATE);
    Employee bad = e;
    strcpy(bad.id, "BAD");
    bad.baseSalary = 0;
    CHECK(saveEmployee(a, &bad, 0) == RESULT_INVALID);
    bad = e;
    strcpy(bad.id, "BAD");
    bad.hasEndDate = 1;
    bad.status = 2;
    bad.endDate = (Date){1, 1, 2025};
    CHECK(saveEmployee(a, &bad, 0) == RESULT_INVALID);
    CHECK(missingAttendance(a, 3, 2026, NULL, NULL) == 26);
    Attendance r = attendance("NV001", (Date){1, 3, 2026});
    CHECK(saveAttendance(a, &r, 0) == RESULT_INVALID);
    r = attendance("NO", (Date){2, 3, 2026});
    CHECK(saveAttendance(a, &r, 0) == RESULT_NOT_FOUND);
    r = attendance("NV001", (Date){2, 3, 2026});
    r.regularHalfHours = 17;
    CHECK(saveAttendance(a, &r, 0) == RESULT_INVALID);
    r.type = ATT_PAID_LEAVE;
    r.regularHalfHours = 0;
    r.overtimeHalfHours = 1;
    CHECK(saveAttendance(a, &r, 0) == RESULT_INVALID);
    int index = 0;
    for (int d = 1; d <= 31; d++) {
        Date date = {d, 3, 2026};
        if (!isWorkingDay(date)) {
            continue;
        }
        index++;
        r = attendance("NV001", date);
        if (index <= 5) {
            r.overtimeHalfHours = 4;
        }
        if (index == 24) {
            r.type = ATT_PAID_LEAVE;
            r.regularHalfHours = 0;
        }
        if (index >= 25) {
            r.type = ATT_UNPAID_LEAVE;
            r.regularHalfHours = 0;
        }
        CHECK(saveAttendance(a, &r, 0) == RESULT_OK);
    }
    CHECK(missingAttendance(a, 3, 2026, NULL, NULL) == 0);
    CHECK(saveAttendance(a, &r, 0) == RESULT_DUPLICATE);
    CHECK(deleteEmployee(a, "NV001") == RESULT_INVALID);
    Payroll rows[MAX_EMPLOYEES];
    size_t n;
    CHECK(buildPayroll(a, 3, 2026, rows, &n) == RESULT_OK && n == 1);
    int pi = findInputIndex(a, "NV001", 3, 2026);
    CHECK(pi >= 0);
    PayrollInput p = a->inputs[pi];
    p.bonus = 500000;
    p.deduction = 200000;
    CHECK(savePayrollInput(a, &p) == RESULT_OK);
    CHECK(buildPayroll(a, 3, 2026, rows, &n) == RESULT_OK && rows[0].netPay == 14437500);
    CHECK(deleteAttendance(a, "NV001", (Date){2, 3, 2026}) == RESULT_OK);
    CHECK(missingAttendance(a, 3, 2026, NULL, NULL) == 1);
    CHECK(buildPayroll(a, 3, 2026, rows, &n) == RESULT_INVALID);
    r = attendance("NV001", (Date){2, 3, 2026});
    r.overtimeHalfHours = 4;
    CHECK(saveAttendance(a, &r, 0) == RESULT_OK);
    Employee mid = employee("NV002", (Date){16, 3, 2026}, 2600000);
    mid.hasEndDate = 1;
    mid.endDate = (Date){21, 3, 2026};
    mid.status = 2;
    CHECK(saveEmployee(a, &mid, 0) == RESULT_OK);
    for (int d = 16; d <= 21; d++) {
        r = attendance("NV002", (Date){d, 3, 2026});
        CHECK(saveAttendance(a, &r, 0) == RESULT_OK);
    }
    CHECK(missingAttendance(a, 3, 2026, "NV002", NULL) == 0);
    CHECK(buildPayroll(a, 3, 2026, rows, &n) == RESULT_OK && n == 2 && rows[1].netPay == 660000);
    CHECK(saveData(a) == RESULT_OK && !a->dirty);
    CHECK(finalizePayroll(a, 3, 2026) == RESULT_OK);
    CHECK(isLocked(a, 3, 2026));
    CHECK(finalizePayroll(a, 3, 2026) == RESULT_LOCKED);
    CHECK(saveAttendance(a, &r, 1) == RESULT_LOCKED);
    CHECK(deleteAttendance(a, "NV001", (Date){2, 3, 2026}) == RESULT_LOCKED);
    CHECK(savePayrollInput(a, &p) == RESULT_LOCKED);
    e.startDate = (Date){6, 1, 2026};
    CHECK(saveEmployee(a, &e, 1) == RESULT_OK);
    e.startDate = (Date){3, 3, 2026};
    CHECK(saveEmployee(a, &e, 1) != RESULT_OK);
    e.startDate = (Date){6, 1, 2026};
    strcpy(e.fullName, "Nguyễn \"An\", A");
    e.baseSalary = 15000000;
    CHECK(saveEmployee(a, &e, 1) == RESULT_OK);
    CHECK(buildPayroll(a, 3, 2026, rows, &n) == RESULT_OK && rows[0].netPay == 14437500 &&
          !strcmp(rows[0].employeeName, "Nguyễn Văn An"));
    Employee retro = employee("RETRO", (Date){1, 3, 2026}, 1000000);
    CHECK(saveEmployee(a, &retro, 0) == RESULT_LOCKED);
    Employee future = employee("FUTURE", (Date){1, 4, 2026}, 1000000);
    CHECK(saveEmployee(a, &future, 0) == RESULT_OK);
    CHECK(saveData(a) == RESULT_OK);
    appInit(b, directory);
    CHECK(loadData(b) == RESULT_OK);
    CHECK(b->employeeCount == 3 && b->attendanceCount == 32 && b->lockCount == 1);
    CHECK(buildPayroll(b, 3, 2026, rows, &n) == RESULT_OK && n == 2 && rows[0].netPay == 14437500);
    char filename[PATH_CAP];
    snprintf(filename, sizeof filename, "%s/report.csv", directory);
    CHECK(exportPayroll(b, 3, 2026, filename, 0) == RESULT_OK);
    CHECK(exportPayroll(b, 3, 2026, filename, 0) == RESULT_DUPLICATE);
    CHECK(exportPayroll(b, 3, 2026, filename, 1) == RESULT_OK);
    CHECK(exportPayroll(b, 3, 2026, directory, 1) == RESULT_INVALID);
    CHECK(makeDirectory(directory));
    snprintf(filename, sizeof filename, "%s/employees.txt.tmp", directory);
    FILE *f = fopen(filename, "wb");
    CHECK(f != NULL);
    if (f) {
        fputs("unfinished", f);
        fclose(f);
        CHECK(loadData(b) == RESULT_IO_ERROR);
        CHECK(b->employeeCount == 3 && b->lockCount == 1);
        CHECK(remove(filename) == 0);
    }
    snprintf(filename, sizeof filename, "%s/attendance.txt", directory);
    f = fopen(filename, "ab");
    CHECK(f != NULL);
    if (f) {
        fputs("NV001|2026-03-02|1|16|4|duplicate\n", f);
        fclose(f);
        CHECK(loadData(b) == RESULT_DUPLICATE);
        CHECK(b->attendanceCount == 32);
        CHECK(saveData(a) == RESULT_OK);
        CHECK(loadData(b) == RESULT_OK);
    }
    snprintf(filename, sizeof filename, "%s/payroll_2026_04.txt", directory);
    f = fopen(filename, "wb");
    CHECK(f != NULL);
    if (f) {
        fputs("PAYROLL|1|2026|4|1\nBROKEN\n", f);
        fclose(f);
        CHECK(loadData(b) != RESULT_OK);
        CHECK(b->lockCount == 1);
        CHECK(remove(filename) == 0);
    }
    snprintf(filename, sizeof filename, "%s/not_a_directory", directory);
    f = fopen(filename, "wb");
    CHECK(f != NULL);
    if (f) {
        fputs("blocked", f);
        fclose(f);
        char old[PATH_CAP];
        strcpy(old, a->dataDir);
        strcpy(a->dataDir, filename);
        CHECK(finalizePayroll(a, 12, 2025) == RESULT_IO_ERROR);
        CHECK(!isLocked(a, 12, 2025));
        strcpy(a->dataDir, old);
    }
    appFree(a);
    appFree(b);
    free(a);
    free(b);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "test_core requires an empty temporary directory\n");
        return 2;
    }
    datesAndInput();
    salaryMath();
    application(argv[1]);
    printf("CHECKS=%d FAILURES=%d\n", checks, failures);
    return failures ? 1 : 0;
}

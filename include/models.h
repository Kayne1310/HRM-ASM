#ifndef MODELS_H
#define MODELS_H

#include <stddef.h>
#include <stdio.h>

#define MAX_EMPLOYEES 200
#define MAX_ATTENDANCE 150000
#define MAX_MONEY 1000000000LL
#define PATH_CAP 1024

/* Ngày được kiểm tra trong phạm vi năm 2000 đến 2100. */
typedef struct {
    int day;
    int month;
    int year;
} Date;

typedef enum { ATT_WORK = 1, ATT_PAID_LEAVE = 2, ATT_UNPAID_LEAVE = 3 } AttendanceType;

typedef struct {
    char id[16];
    char fullName[101];
    char department[61];
    char position[61];
    Date startDate;
    Date endDate;
    int hasEndDate;       /* 0: chưa nghỉ; 1: có ngày làm cuối. */
    int status;           /* 1: đang làm; 2: đã nghỉ. */
    long long baseSalary; /* Tiền nguyên đồng. */
    long long allowance;
} Employee;

/* Nút danh sách liên kết đơn: dữ liệu và con trỏ tới nút tiếp theo. */
typedef struct EmployeeNode {
    Employee employee;
    struct EmployeeNode *next;
} EmployeeNode;

typedef struct {
    char employeeId[16];
    Date date;
    AttendanceType type;
    int regularHalfHours;  /* 16 đơn vị = 8 giờ = một ngày công. */
    int overtimeHalfHours; /* 2 đơn vị = một giờ. */
    char note[201];
} Attendance;

/* Bản sao thông số theo tháng; đổi hồ sơ không tự đổi dữ liệu này. */
typedef struct {
    char employeeId[16];
    int month;
    int year;
    long long baseSalary;
    long long allowance;
    long long bonus;
    long long deduction;
} PayrollInput;

/* Kết quả đã chốt giữ cả tên và phòng ban tại thời điểm chốt. */
typedef struct {
    char employeeId[16];
    char employeeName[101];
    char department[61];
    int month;
    int year;
    int standardDays;
    int paidHalfHours;
    int overtimeHalfHours;
    long long baseSalary;
    long long allowance;
    long long bonus;
    long long deduction;
    long long regularPay;
    long long allowancePay;
    long long overtimePay;
    long long netPay;
} Payroll;

typedef struct {
    int month;
    int year;
} MonthLock;

typedef enum {
    RESULT_OK,
    RESULT_INVALID,
    RESULT_NOT_FOUND,
    RESULT_DUPLICATE,
    RESULT_FULL,
    RESULT_LOCKED,
    RESULT_IO_ERROR,
    RESULT_NO_MEMORY
} Result;

/*
 * AppData sở hữu mọi vùng nhớ: khởi tạo bằng appInit, giải phóng bằng appFree.
 * Nhân viên dùng danh sách liên kết; công và lương dùng mảng động để tổng hợp.
 */
typedef struct {
    EmployeeNode *employeeHead;
    size_t employeeCount;
    Attendance *attendance;
    size_t attendanceCount;
    size_t attendanceCap;
    PayrollInput *inputs;
    size_t inputCount;
    size_t inputCap;
    Payroll *payrolls;
    size_t payrollCount;
    size_t payrollCap;
    MonthLock *locks;
    size_t lockCount;
    size_t lockCap;
    int dirty; /* 1: còn thay đổi trong bộ nhớ chưa ghi tệp. */
    char dataDir[PATH_CAP];
    char error[512];
} AppData;

void appInit(AppData *app, const char *dataDirectory);
void appFree(AppData *app);
Result appError(AppData *app, Result code, const char *format, ...);
/* Chỉ đổi con trỏ sau khi realloc thành công; capacity là số phần tử. */
int reserveArray(void **array, size_t *capacity, size_t wantedCount, size_t elementSize);
int validText(const char *text, size_t maxBytes, int allowEmpty);
void normalizeId(char *employeeId);
int validId(const char *employeeId);
int validMonth(int month, int year);
int isLocked(const AppData *app, int month, int year);
int employeeInMonth(const Employee *employee, int month, int year);
int employeeOnDate(const Employee *employee, Date date);
#endif

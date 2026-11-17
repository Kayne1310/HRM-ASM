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
    Employee employee;          // Dữ liệu của 1 nhân viên
    struct EmployeeNode *next;  // Con trỏ trỏ tới nút (nhân viên) kế tiếp
} EmployeeNode;

typedef struct {
    char employeeId[16];
    Date date;
    AttendanceType type;
    int regularHalfHours;  /* 16 đơn vị = 8 giờ = một ngày công. */
    int overtimeHalfHours; /* 2 đơn vị = một giờ. */
    char note[201];
} Attendance;

/* Dữ liệu đầu vào tính lương theo tháng (lương, phụ cấp, thưởng, phạt riêng của từng tháng) */
typedef struct {
    char employeeId[16];
    int month;
    int year;
    long long baseSalary;
    long long allowance;// trợ cấp
    long long bonus;// thưởng
    long long deduction;// tiền phạt
} PayrollInput;

/* Phiếu lương / Kết quả cuối cùng */
typedef struct {
    char employeeId[16];
    char employeeName[101]; // Lưu cứng tên tại thời điểm chốt
    char department[61];    // Lưu cứng phòng ban tại thời điểm chốt
    int month;
    int year;
    int standardDays;       // Số công chuẩn của tháng (ví dụ 26 ngày)
    int paidHalfHours;      // Tổng nửa giờ được hưởng lương (đi làm + phép có lương)
    int overtimeHalfHours;  // Tổng nửa giờ tăng ca
    long long baseSalary;
    long long allowance;
    long long bonus;
    long long deduction;
    long long regularPay;   // Tiền lương ngày công thường
    long long allowancePay; // Tiền phụ cấp thực nhận theo công
    long long overtimePay;  // Tiền tăng ca (nhân hệ số 150%)
    long long netPay;       // THỰC LĨNH (tổng cộng các khoản - khấu trừ)
} Payroll;

typedef struct {
    int month;
    int year;
} MonthLock;

typedef enum {
    RESULT_OK,        // Thành công
    RESULT_INVALID,   // Dữ liệu nhập sai quy tắc (ví dụ: ngày 30/02, lương âm)
    RESULT_NOT_FOUND, // Không tìm thấy mã nhân viên / bản ghi
    RESULT_DUPLICATE, // Trùng lặp (ví dụ: trùng mã NV hoặc đã chấm công ngày này rồi)
    RESULT_FULL,      // Đầy bộ nhớ cho phép (vượt quá 200 NV hoặc 150.000 dòng công)
    RESULT_LOCKED,    // Tháng này đã bị khóa (chốt lương rồi), cấm sửa!
    RESULT_IO_ERROR,  // Lỗi đọc/ghi file trên ổ cứng
    RESULT_NO_MEMORY  // Hết RAM khi gọi malloc/realloc
} Result;

/*
 * AppData sở hữu mọi vùng nhớ: khởi tạo bằng appInit, giải phóng bằng appFree.
 * Nhân viên dùng danh sách liên kết; công và lương dùng mảng động để tổng hợp.
 */
typedef struct {
    /* Danh sách liên kết đơn nhân viên */
    EmployeeNode *employeeHead; /* Con trỏ tới nhân viên đầu danh sách */
    size_t employeeCount;       /* Tổng số nhân viên */

    /* Mảng động dữ liệu chấm công */
    Attendance *attendance;     /* Con trỏ mảng chấm công */
    size_t attendanceCount;     /* Số bản ghi công hiện có */
    size_t attendanceCap;       /* Sức chứa tối đa của mảng công */

    /* Mảng động đầu vào tính lương theo tháng (mức lương, thưởng, phạt riêng từng tháng) */
    PayrollInput *inputs;       /* Con trỏ mảng đầu vào tính lương tháng */
    size_t inputCount;          /* Số bản ghi đầu vào lương hiện có */
    size_t inputCap;            /* Sức chứa tối đa của mảng đầu vào lương */

    /* Mảng động bảng lương kết quả */
    Payroll *payrolls;          /* Con trỏ mảng bảng lương */
    size_t payrollCount;        /* Số bản ghi lương hiện có */
    size_t payrollCap;          /* Sức chứa tối đa của mảng lương */

    /* Mảng động lưu các tháng đã chốt kỳ */
    MonthLock *locks;           /* Con trỏ mảng kỳ đã chốt */
    size_t lockCount;           /* Số kỳ đã chốt */
    size_t lockCap;             /* Sức chứa tối đa của mảng kỳ chốt */

    /* Trạng thái chung */
    int dirty;                  /* 1: có thay đổi chưa lưu ra tệp; 0: đã lưu */
    char dataDir[PATH_CAP];     /* Đường dẫn thư mục dữ liệu (vd: "demo-run") */
    char error[512];            /* Chuỗi lưu thông báo lỗi gần nhất */
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

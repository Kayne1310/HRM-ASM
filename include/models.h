#ifndef MODELS_H /* Tránh nạp trùng lặp file header này nhiều lần khi biên dịch */
#define MODELS_H

#include <stddef.h> /* Dùng cho kiểu size_t (kích thước bộ nhớ, số lượng phần tử) */
#include <stdio.h>  /* Dùng cho kiểu FILE* khi thao tác nhập xuất */

#define MAX_EMPLOYEES 200        /* Giới hạn tối đa 200 nhân viên */
#define MAX_ATTENDANCE 150000    /* Giới hạn tối đa 150.000 bản ghi chấm công */
#define MAX_MONEY 1000000000LL   /* Giới hạn số tiền tối đa mỗi khoản: 1 tỷ đồng (long long) */
#define PATH_CAP 1024            /* Độ dài tối đa của chuỗi đường dẫn thư mục/tệp */

/* Kiểu dữ liệu ngày tháng năm, kiểm tra trong phạm vi năm 2000 đến 2100. */
typedef struct {
    int day;   /* Ngày (1..31) */
    int month; /* Tháng (1..12) */
    int year;  /* Năm (2000..2100) */
} Date;

/* Các loại chấm công trong một ngày làm việc */
typedef enum {
    ATT_WORK = 1,         /* 1: Đi làm bình thường (có giờ công, có thể có tăng ca) */
    ATT_PAID_LEAVE = 2,   /* 2: Nghỉ có phép / có lương (hưởng đủ 1 ngày công) */
    ATT_UNPAID_LEAVE = 3  /* 3: Nghỉ không lương (0 công) */
} AttendanceType;

/* Cấu trúc lưu thông tin hồ sơ của một nhân viên */
typedef struct {
    char id[16];          /* Mã nhân viên (tối đa 15 ký tự + ký tự kết thúc '\0') */
    char fullName[101];   /* Họ tên đầy đủ (tối đa 100 byte UTF-8) */
    char department[61];  /* Tên phòng ban (tối đa 60 byte UTF-8) */
    char position[61];    /* Chức vụ (tối đa 60 byte UTF-8) */
    Date startDate;       /* Ngày bắt đầu vào làm việc */
    Date endDate;         /* Ngày làm việc cuối cùng (nếu đã nghỉ việc) */
    int hasEndDate;       /* Cờ đánh dấu: 0 = chưa nghỉ; 1 = có ngày làm cuối. */
    int status;           /* Trạng thái: 1 = đang làm; 2 = đã nghỉ việc. */
    long long baseSalary; /* Lương cơ bản (tính theo tiền nguyên đồng) */
    long long allowance;  /* Phụ cấp cố định hàng tháng (nguyên đồng) */
} Employee;

/* Nút danh sách liên kết đơn: dữ liệu và con trỏ tới nút tiếp theo. */
typedef struct EmployeeNode {
    Employee employee;          // Dữ liệu của 1 nhân viên
    struct EmployeeNode *next;  // Con trỏ trỏ tới nút (nhân viên) kế tiếp
} EmployeeNode;

/* Cấu trúc lưu một bản ghi chấm công trong ngày của một nhân viên */
typedef struct {
    char employeeId[16];   /* Mã nhân viên được chấm công (khóa ngoại trỏ tới Employee.id) */
    Date date;             /* Ngày chấm công */
    AttendanceType type;   /* Loại công: đi làm, nghỉ có lương hay nghỉ không lương */
    int regularHalfHours;  /* Số nửa giờ làm thường: 16 đơn vị = 8 giờ = 1 ngày công. */
    int overtimeHalfHours; /* Số nửa giờ tăng ca: 2 đơn vị = 1 giờ (tối đa 8 đv = 4 giờ). */
    char note[201];        /* Ghi chú thêm cho ngày công (tối đa 200 byte) */
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

/* Cấu trúc lưu thông tin tháng/năm đã được chốt lương */
typedef struct {
    int month; /* Tháng đã chốt (1..12) */
    int year;  /* Năm đã chốt (2000..2100) */
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

/* Khởi tạo cấu trúc AppData về trạng thái rỗng và gán thư mục dữ liệu */
void appInit(AppData *app, const char *dataDirectory);
/* Giải phóng toàn bộ bộ nhớ động (danh sách liên kết + các mảng) của AppData */
void appFree(AppData *app);
/* Ghi thông báo lỗi vào app->error theo định dạng giống printf và trả về mã lỗi code */
Result appError(AppData *app, Result code, const char *format, ...);
/* Tăng dung lượng mảng động bằng realloc an toàn; chỉ đổi con trỏ khi cấp phát thành công */
int reserveArray(void **array, size_t *capacity, size_t wantedCount, size_t elementSize);
/* Kiểm tra chuỗi UTF-8 hợp lệ: không vượt quá maxBytes, không chứa ký tự điều khiển hay '|' */
int validText(const char *text, size_t maxBytes, int allowEmpty);
/* Chuẩn hóa mã nhân viên: cắt khoảng trắng hai đầu và chuyển chữ thường thành chữ IN HOA */
void normalizeId(char *employeeId);
/* Kiểm tra mã nhân viên hợp lệ (1-15 ký tự gồm A-Z, 0-9, '_' hoặc '-') */
int validId(const char *employeeId);
/* Kiểm tra tháng (1..12) và năm (2000..2100) hợp lệ */
int validMonth(int month, int year);
/* Kiểm tra xem tháng/năm truyền vào đã được chốt lương hay chưa (1 = đã chốt, 0 = chưa) */
int isLocked(const AppData *app, int month, int year);
/* Kiểm tra nhân viên có làm việc trong tháng/năm đó không (dựa vào ngày vào và ngày nghỉ) */
int employeeInMonth(const Employee *employee, int month, int year);
/* Kiểm tra nhân viên có đang làm việc tại một ngày cụ thể hay không */
int employeeOnDate(const Employee *employee, Date date);
#endif

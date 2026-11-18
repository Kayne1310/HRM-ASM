#include "storage.h"
#include "input.h"
#include "date_utils.h"
#include "employee.h"
#include "attendance.h"
#include "payroll.h"
#include <string.h>   /* Dùng cho strcmp, strncmp, strcpy, strlen, strerror */
#include <stdlib.h>   /* Dùng cho qsort */
#include <errno.h>    /* Dùng cho errno, ENOENT phát hiện lỗi mở/tạo tệp */
#include <limits.h>   /* Dùng cho LLONG_MAX */
#include <sys/stat.h> /* Dùng cho hàm stat kiểm tra tệp/thư mục tồn tại */
#ifdef _WIN32
#include <windows.h> /* Dùng Win32 FindFirstFileA / FindNextFileA duyệt thư mục trên Windows */
#include <direct.h>  /* Dùng _mkdir tạo thư mục trên Windows */
#else
#include <dirent.h>  /* Dùng opendir / readdir duyệt thư mục trên Linux/macOS */
#include <unistd.h>
#endif

/* Kiểm tra đường dẫn filePath có tồn tại trên đĩa hay không (1 = tồn tại, 0 = không) */
int fileExists(const char *filePath) {
    struct stat fileStatus;
    return stat(filePath, &fileStatus) == 0;
}

/* Kiểm tra đường dẫn có phải là một tệp tin thông thường (không phải thư mục) */
static int regularFile(const char *filePath) {
    struct stat fileStatus;
    if (stat(filePath, &fileStatus) != 0) {
        return 0;
    }
#ifdef _WIN32
    return (fileStatus.st_mode & _S_IFREG) != 0;
#else
    return S_ISREG(fileStatus.st_mode);
#endif
}

/* Tạo thư mục mới nếu chưa có; nếu thư mục đã tồn tại sẵn thì vẫn trả về 1 (thành công) */
int makeDirectory(const char *directory) {
    struct stat fileStatus;
    if (stat(directory, &fileStatus) == 0) {
#ifdef _WIN32
        return (fileStatus.st_mode & _S_IFDIR) != 0; /* Đảm bảo đường dẫn là thư mục chứ không phải file trùng tên */
#else
        return S_ISDIR(fileStatus.st_mode);
#endif
    }
#ifdef _WIN32
    return _mkdir(directory) == 0;
#else
    return mkdir(directory, 0700) == 0;
#endif
}

/* Ghép tên thư mục và tên tệp thành đường dẫn hoàn chỉnh "directory/fileName" */
static int joinPath(char output[PATH_CAP], const char *directory, const char *fileName) {
    int pathLength = snprintf(output, PATH_CAP, "%s/%s", directory, fileName);
    return pathLength >= 0 && pathLength < PATH_CAP;
}

/*
 * Hàm tự viết để tách chuỗi theo dấu '|' mà vẫn GIỮ NGUYÊN các trường rỗng (ví dụ: A||B).
 * Không dùng strtok() vì strtok() tự bỏ qua các dấu phân cách đứng liền nhau làm sai vị trí cột.
 */
static int splitFields(char *lineBuffer, char **fields, int maxFields) {
    int fieldCount = 1;
    fields[0] = lineBuffer; /* Trường đầu tiên bắt đầu từ đầu dòng */
    for (char *cursor = lineBuffer; *cursor; cursor++) {
        if (*cursor == '|') {
            *cursor = 0; /* Cắt chuỗi ngay tại dấu '|' bằng cách thay bằng ký tự kết thúc '\0' */
            if (fieldCount == maxFields) {
                return -1; /* Vượt quá số lượng cột cho phép */
            }
            fields[fieldCount++] = cursor + 1; /* Trường tiếp theo bắt đầu ngay sau ký tự '\0' vừa đặt */
        }
    }
    return fieldCount;
}

/* Hàm bọc hỗ trợ đọc trường số nguyên long long trong đoạn [minValue, maxValue] */
static int parseNumberField(const char *text, long long minValue, long long maxValue,
                            long long *value) {
    return parseInteger(text, minValue, maxValue, value);
}

/* Hàm bọc đọc trường số nguyên kiểu int trong đoạn [minValue, maxValue] */
static int parseIntField(const char *text, int minValue, int maxValue, int *value) {
    long long parsedValue;
    if (!parseNumberField(text, minValue, maxValue, &parsedValue)) {
        return 0;
    }
    *value = (int)parsedValue;
    return 1;
}

/* Kiểm tra chuỗi UTF-8 hợp lệ rồi mới sao chép vào vùng nhớ đích destination */
static int copyValidatedText(char *destination, size_t capacity, const char *source,
                             int allowEmpty) {
    if (!validText(source, capacity - 1, allowEmpty)) {
        return 0;
    }
    strcpy(destination, source);
    return 1;
}

/* Đọc ngày từ tệp dữ liệu: bắt buộc đúng chuẩn ISO yyyy-mm-dd dài 10 ký tự */
static int parseIsoDate(const char *text, Date *date) {
    return strlen(text) == 10 && text[4] == '-' && parseDate(text, date);
}

/* Tạo thông báo lỗi định dạng tệp kèm tên tệp và số dòng bị lỗi */
static Result invalidFileRow(AppData *app, const char *fileName, int lineNumber) {
    return appError(app, RESULT_INVALID, "Tệp %s dòng %d: sai trường, dữ liệu hoặc khóa trùng.",
                    fileName, lineNumber);
}

/* Nạp lần lượt từng loại tệp nguồn: fileKind 0 = employees.txt, 1 = attendance.txt, 2 = payroll_inputs.txt */
static Result readBase(AppData *app, const char *fileName, int fileKind) {
    char filePath[PATH_CAP], lineBuffer[4096], *fields[20];
    FILE *file;
    int lineNumber = 1, readStatus, fieldCount;
    Result result = RESULT_OK;
    /* Dòng đầu tiên (header) bắt buộc phải khớp phiên bản quy định */
    const char *header = fileKind == 0   ? "EMPLOYEES|1"
                         : fileKind == 1 ? "ATTENDANCE|1"
                                         : "PAYROLL_INPUTS|1";
    if (!joinPath(filePath, app->dataDir, fileName)) {
        return appError(app, RESULT_INVALID, "Đường dẫn quá dài.");
    }
    file = fopen(filePath, "rb"); /* Mở ở chế độ đọc nhị phân "rb" để không bị Windows tự sửa đổi xuống dòng */
    if (!file) {
        if (errno == ENOENT) { /* Tệp chính chưa tồn tại (ví dụ lần đầu khởi tạo dự án trống) */
            char bak[PATH_CAP];
            int k = snprintf(bak, sizeof bak, "%s.bak", filePath);
            /* Kiểm tra nếu thiếu tệp chính nhưng lại có bản sao lưu .bak -> có dấu hiệu mất tệp bất thường -> chặn! */
            if (k > 0 && k < (int)sizeof bak && fileExists(bak)) {
                return appError(app, RESULT_IO_ERROR,
                                "Thiếu tệp chính %s nhưng còn .bak; hãy phục hồi trước khi chạy.",
                                fileName);
            }
            return RESULT_OK; /* Lần đầu chưa có cả tệp chính lẫn .bak -> cho phép rỗng */
        }
        return appError(app, RESULT_IO_ERROR, "Không mở được %s.", filePath);
    }
    /* Đọc và kiểm tra dòng 1 (header định danh tệp) */
    if (readLine(file, lineBuffer, sizeof lineBuffer) != 1 || strcmp(lineBuffer, header)) {
        result = invalidFileRow(app, fileName, 1);
        goto done;
    }
    /* Đọc từng dòng dữ liệu tiếp theo cho đến hết tệp */
    while ((readStatus = readLine(file, lineBuffer, sizeof lineBuffer)) != 0) {
        lineNumber++;
        if (readStatus != 1) { /* Dòng quá dài vượt kích thước lineBuffer */
            result = invalidFileRow(app, fileName, lineNumber);
            break;
        }
        fieldCount = splitFields(lineBuffer, fields, 20); /* Tách dòng thành các cột dựa vào dấu '|' */
        if (fileKind == 0) { /* Tệp hồ sơ nhân viên (9 cột) */
            Employee employee = {0};
            if (fieldCount != 9 ||
                !copyValidatedText(employee.id, sizeof employee.id, fields[0], 0) ||
                !validId(employee.id) ||
                !copyValidatedText(employee.fullName, sizeof employee.fullName, fields[1], 0) ||
                !copyValidatedText(employee.department, sizeof employee.department, fields[2], 0) ||
                !copyValidatedText(employee.position, sizeof employee.position, fields[3], 0) ||
                !parseIsoDate(fields[4], &employee.startDate) ||
                (*fields[5] && !parseIsoDate(fields[5], &employee.endDate)) ||
                !parseNumberField(fields[6], 1, MAX_MONEY, &employee.baseSalary) ||
                !parseNumberField(fields[7], 0, MAX_MONEY, &employee.allowance) ||
                !parseIntField(fields[8], 1, 2, &employee.status)) {
                result = invalidFileRow(app, fileName, lineNumber);
                break;
            }
            employee.hasEndDate = *fields[5] != 0; /* Trường ngày nghỉ khác rỗng -> có ngày làm cuối */
            result = saveEmployee(app, &employee, 0); /* Thêm vào danh sách liên kết và kiểm tra trùng mã */
        } else if (fileKind == 1) { /* Tệp chấm công (6 cột) */
            Attendance record = {0};
            int attendanceType;
            if (fieldCount != 6 ||
                !copyValidatedText(record.employeeId, sizeof record.employeeId, fields[0], 0) ||
                !parseIsoDate(fields[1], &record.date) ||
                !parseIntField(fields[2], 1, 3, &attendanceType) ||
                !parseIntField(fields[3], 0, 16, &record.regularHalfHours) ||
                !parseIntField(fields[4], 0, 8, &record.overtimeHalfHours) ||
                !copyValidatedText(record.note, sizeof record.note, fields[5], 1)) {
                result = invalidFileRow(app, fileName, lineNumber);
                break;
            }
            record.type = (AttendanceType)attendanceType;
            result = validateAttendance(app, &record);
            if (result == RESULT_OK) { /* Nạp thẳng vào mảng attendance; việc kiểm tra trùng khóa thực hiện bằng qsort ở loadData */
                if (app->attendanceCount >= MAX_ATTENDANCE) {
                    result = appError(app, RESULT_FULL, "Đầy danh sách công.");
                } else if (!reserveArray((void **)&app->attendance, &app->attendanceCap,
                                         app->attendanceCount + 1, sizeof(Attendance))) {
                    result = appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ.");
                } else {
                    app->attendance[app->attendanceCount++] = record;
                }
            }
        } else { /* Tệp thông số lương tháng (7 cột) */
            PayrollInput input = {0};
            if (fieldCount != 7 ||
                !copyValidatedText(input.employeeId, sizeof input.employeeId, fields[0], 0) ||
                !parseIntField(fields[1], 1, 12, &input.month) ||
                !parseIntField(fields[2], 2000, 2100, &input.year) ||
                !parseNumberField(fields[3], 1, MAX_MONEY, &input.baseSalary) ||
                !parseNumberField(fields[4], 0, MAX_MONEY, &input.allowance) ||
                !parseNumberField(fields[5], 0, MAX_MONEY, &input.bonus) ||
                !parseNumberField(fields[6], 0, MAX_MONEY, &input.deduction) ||
                findInputIndex(app, input.employeeId, input.month, input.year) >= 0) {
                result = invalidFileRow(app, fileName, lineNumber);
                break;
            }
            result = savePayrollInput(app, &input);
        }
        if (result != RESULT_OK) { /* Nếu dòng này sai nghiệp vụ -> gắn thêm tên tệp và số dòng rồi dừng nạp */
            char detail[300];
            snprintf(detail, sizeof detail, "%.290s", app->error);
            result = appError(app, result, "Tệp %s dòng %d: %s", fileName, lineNumber, detail);
            break;
        }
    }
    if (result == RESULT_OK && ferror(file)) { /* Kiểm tra lỗi phần cứng/đĩa khi đọc */
        result = appError(app, RESULT_IO_ERROR, "Lỗi đọc %s.", fileName);
    }
done:
    if (fclose(file) && result == RESULT_OK) {
        result = appError(app, RESULT_IO_ERROR, "Lỗi đóng %s.", fileName);
    }
    return result;
}

/* So sánh toàn bộ các khoản số và khóa của 2 dòng lương để kiểm chứng tính nhất quán của Snapshot */
static int samePayroll(const Payroll *left, const Payroll *right) {
    return !strcmp(left->employeeId, right->employeeId) && left->month == right->month &&
           left->year == right->year && left->standardDays == right->standardDays &&
           left->paidHalfHours == right->paidHalfHours &&
           left->overtimeHalfHours == right->overtimeHalfHours &&
           left->baseSalary == right->baseSalary && left->allowance == right->allowance &&
           left->bonus == right->bonus && left->deduction == right->deduction &&
           left->regularPay == right->regularPay && left->allowancePay == right->allowancePay &&
           left->overtimePay == right->overtimePay && left->netPay == right->netPay;
}

/* Đọc tệp bảng lương đã chốt (payroll_YYYY_MM.txt), kiểm tra công thức và đối chiếu khớp hoàn toàn với dữ liệu nguồn. */
static Result readSnapshot(AppData *app, const char *fileName) {
    char filePath[PATH_CAP], lineBuffer[4096], *fields[20], expected[80];
    FILE *file;
    int month, year, expectedCount, fieldCount, readStatus, lineNumber = 1;
    size_t firstPayrollIndex = app->payrollCount;
    Result result = RESULT_OK;
    if (!joinPath(filePath, app->dataDir, fileName)) {
        return appError(app, RESULT_INVALID, "Đường dẫn quá dài.");
    }
    file = fopen(filePath, "rb");
    if (!file) {
        return appError(app, RESULT_IO_ERROR, "Không mở được %s.", fileName);
    }
    /* Kiểm tra dòng đầu: PAYROLL|1|năm|tháng|số_lượng_bản_ghi */
    if (readLine(file, lineBuffer, sizeof lineBuffer) != 1 ||
        splitFields(lineBuffer, fields, 20) != 5 || strcmp(fields[0], "PAYROLL") ||
        strcmp(fields[1], "1") || !parseIntField(fields[2], 2000, 2100, &year) ||
        !parseIntField(fields[3], 1, 12, &month) ||
        !parseIntField(fields[4], 0, 200, &expectedCount)) {
        result = invalidFileRow(app, fileName, 1);
        goto done;
    }
    /* Tên tệp trên đĩa phải khớp đúng với tháng/năm ghi bên trong header */
    snprintf(expected, sizeof expected, "payroll_%04d_%02d.txt", year, month);
    if (strcmp(fileName, expected) || isLocked(app, month, year)) {
        result = invalidFileRow(app, fileName, 1);
        goto done;
    }
    /* Đọc từng dòng lương (16 cột) trong bản chốt */
    while ((readStatus = readLine(file, lineBuffer, sizeof lineBuffer)) != 0) {
        Payroll payroll = {0}, recalculated;
        PayrollInput input = {0};
        int employeeIndex;
        lineNumber++;
        if (readStatus != 1 || (fieldCount = splitFields(lineBuffer, fields, 20)) != 16 ||
            !copyValidatedText(payroll.employeeId, sizeof payroll.employeeId, fields[0], 0) ||
            !copyValidatedText(payroll.employeeName, sizeof payroll.employeeName, fields[1], 0) ||
            !copyValidatedText(payroll.department, sizeof payroll.department, fields[2], 0) ||
            !parseIntField(fields[3], month, month, &payroll.month) ||
            !parseIntField(fields[4], year, year, &payroll.year) ||
            !parseIntField(fields[5], 1, 31, &payroll.standardDays) ||
            !parseIntField(fields[6], 0, 496, &payroll.paidHalfHours) ||
            !parseIntField(fields[7], 0, 248, &payroll.overtimeHalfHours) ||
            !parseNumberField(fields[8], 1, MAX_MONEY, &payroll.baseSalary) ||
            !parseNumberField(fields[9], 0, MAX_MONEY, &payroll.allowance) ||
            !parseNumberField(fields[10], 0, MAX_MONEY, &payroll.bonus) ||
            !parseNumberField(fields[11], 0, MAX_MONEY, &payroll.deduction) ||
            !parseNumberField(fields[12], 0, LLONG_MAX, &payroll.regularPay) ||
            !parseNumberField(fields[13], 0, LLONG_MAX, &payroll.allowancePay) ||
            !parseNumberField(fields[14], 0, LLONG_MAX, &payroll.overtimePay) ||
            !parseNumberField(fields[15], 0, LLONG_MAX, &payroll.netPay)) {
            result = invalidFileRow(app, fileName, lineNumber);
            goto done;
        }
        /* Kiểm tra nhân viên tồn tại, làm việc trong kỳ, đúng số công chuẩn và có bản ghi thông số tháng */
        employeeIndex = findEmployeeIndex(app, payroll.employeeId);
        if (employeeIndex < 0 || !employeeInMonth(employeeAt(app, employeeIndex), month, year) ||
            payroll.standardDays != workingDaysInMonth(month, year) ||
            findInputIndex(app, payroll.employeeId, month, year) < 0) {
            result = invalidFileRow(app, fileName, lineNumber);
            goto done;
        }
        /* Chống trùng lặp mã nhân viên trong cùng 1 tệp snapshot */
        for (size_t index = firstPayrollIndex; index < app->payrollCount; index++) {
            if (!strcmp(app->payrolls[index].employeeId, payroll.employeeId)) {
                result = invalidFileRow(app, fileName, lineNumber);
                goto done;
            }
        }
        /* Tự tính lại bằng calculatePayroll để phát hiện nếu tệp snapshot bị sửa lậu số tiền từ bên ngoài */
        strcpy(input.employeeId, payroll.employeeId);
        input.month = month;
        input.year = year;
        input.baseSalary = payroll.baseSalary;
        input.allowance = payroll.allowance;
        input.bonus = payroll.bonus;
        input.deduction = payroll.deduction;
        if (calculatePayroll(&input, payroll.standardDays, payroll.paidHalfHours,
                             payroll.overtimeHalfHours, &recalculated) != RESULT_OK ||
            !samePayroll(&payroll, &recalculated)) {
            result = invalidFileRow(app, fileName, lineNumber);
            goto done;
        }
        if (app->payrollCount - firstPayrollIndex >= MAX_EMPLOYEES ||
            !reserveArray((void **)&app->payrolls, &app->payrollCap, app->payrollCount + 1,
                          sizeof(Payroll))) {
            result = appError(app, RESULT_FULL, "Bảng lương quá lớn hoặc không đủ bộ nhớ.");
            goto done;
        }
        app->payrolls[app->payrollCount++] = payroll;
    }
    if (ferror(file)) {
        result = appError(app, RESULT_IO_ERROR, "Lỗi đọc %s.", fileName);
        goto done;
    }
    /* Số dòng thực tế đọc được phải khớp đúng với số dòng khai báo ở header */
    if (app->payrollCount - firstPayrollIndex != (size_t)expectedCount) {
        result = invalidFileRow(app, fileName, lineNumber);
        goto done;
    }
    /* Đối chiếu toàn bộ bảng lương đã chốt với dữ liệu nguồn (công + thông số tháng) hiện tại */
    {
        Payroll rows[MAX_EMPLOYEES];
        size_t count = 0;
        result = buildPayroll(app, month, year, rows, &count);
        if (result != RESULT_OK) {
            goto done;
        }
        if (count != (size_t)expectedCount) {
            result = invalidFileRow(app, fileName, lineNumber);
            goto done;
        }
        for (size_t index = 0; index < count; index++) {
            int found = 0;
            for (size_t otherIndex = firstPayrollIndex; otherIndex < app->payrollCount;
                 otherIndex++) {
                if (samePayroll(&rows[index], &app->payrolls[otherIndex])) {
                    found = 1;
                }
            }
            if (!found) { /* Phát hiện tệp chấm công hoặc thông số đã bị sửa lệch so với bản chốt */
                result = appError(app, RESULT_INVALID,
                                  "Tệp %s không khớp công hoặc thông số tháng.", fileName);
                goto done;
            }
        }
    }
    /* Ghi nhận kỳ tháng/năm này vào danh sách đã chốt khóa */
    if (!reserveArray((void **)&app->locks, &app->lockCap, app->lockCount + 1, sizeof(MonthLock))) {
        result = appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ.");
        goto done;
    }
    app->locks[app->lockCount++] = (MonthLock){month, year};
done:
    if (fclose(file) && result == RESULT_OK) {
        result = appError(app, RESULT_IO_ERROR, "Lỗi đóng %s.", fileName);
    }
    return result;
}

/* Quét toàn bộ thư mục dữ liệu để phát hiện tệp tạm (.tmp) còn sót và nạp các tệp chốt payroll_YYYY_MM.txt */
static Result scanData(AppData *app) {
    Result result = RESULT_OK;
#ifdef _WIN32
    char pattern[PATH_CAP];
    WIN32_FIND_DATAA entry;
    HANDLE handle;
    if (!joinPath(pattern, app->dataDir, "*")) {
        return appError(app, RESULT_INVALID, "Đường dẫn quá dài.");
    }
    handle = FindFirstFileA(pattern, &entry);
    if (handle == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            return RESULT_OK;
        }
        return appError(app, RESULT_IO_ERROR, "Không liệt kê được thư mục dữ liệu.");
    }
    do {
        const char *fileName = entry.cFileName;
#else
    DIR *directory = opendir(app->dataDir);
    struct dirent *entry;
    if (!directory) {
        return appError(app, RESULT_IO_ERROR, "Không mở thư mục dữ liệu.");
    }
    while ((entry = readdir(directory)) != NULL) {
        const char *fileName = entry->d_name;
#endif
        size_t nameLength = strlen(fileName);
        /* Nếu phát hiện còn tệp .tmp sót lại từ sự cố tắt máy trước đó -> dừng và yêu cầu kiểm tra phục hồi */
        if (nameLength >= 4 && !strcmp(fileName + nameLength - 4, ".tmp")) {
            result = appError(app, RESULT_IO_ERROR,
                              "Còn tệp tạm %s. Kiểm tra và phục hồi trước khi nạp; xem README.",
                              fileName);
            break;
        }
        /* Nạp tất cả các tệp có tên dạng payroll_*.txt (trừ payroll_inputs.txt) */
        if (strcmp(fileName, "payroll_inputs.txt") && !strncmp(fileName, "payroll_", 8) &&
            nameLength >= 4 && !strcmp(fileName + nameLength - 4, ".txt")) {
            result = readSnapshot(app, fileName);
            if (result != RESULT_OK) {
                break;
            }
        }
#ifdef _WIN32
    } while (FindNextFileA(handle, &entry));
    FindClose(handle);
#else
    }
    closedir(directory);
#endif
    return result;
}

/* Hàm so sánh 2 bản ghi chấm công theo cặp khóa (employeeId, date) để dùng với qsort phát hiện trùng khóa */
static int attendanceCompare(const void *leftPointer, const void *rightPointer) {
    const Attendance *left = leftPointer, *right = rightPointer;
    int comparison = strcmp(left->employeeId, right->employeeId);
    return comparison ? comparison : compareDate(left->date, right->date);
}

/* Nạp toàn bộ dữ liệu vào cấu trúc tạm loadedData; chỉ thay thế dữ liệu đang dùng khi mọi bước nạp đều hợp lệ 100%. */
Result loadData(AppData *app) {
    AppData *loadedData = calloc(1, sizeof *loadedData); /* Xin cấp phát 1 AppData tạm trên Heap */
    Result result;
    if (!loadedData) {
        return appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ nạp dữ liệu.");
    }
    appInit(loadedData, app->dataDir);
    if (!makeDirectory(app->dataDir)) {
        free(loadedData);
        return appError(app, RESULT_IO_ERROR, "Không tạo hoặc mở được thư mục dữ liệu.");
    }
    result = readBase(loadedData, "employees.txt", 0); /* 1. Đọc hồ sơ nhân viên */
    if (result == RESULT_OK) {
        result = readBase(loadedData, "attendance.txt", 1); /* 2. Đọc dữ liệu chấm công */
    }
    /* Sắp xếp mảng công O(A log A) rồi duyệt 1 lượt kiểm tra 2 phần tử liền kề để bắt trùng khóa (nhanh hơn O(A^2)) */
    if (result == RESULT_OK && loadedData->attendanceCount > 1) {
        qsort(loadedData->attendance, loadedData->attendanceCount, sizeof(Attendance),
              attendanceCompare);
        for (size_t index = 1; index < loadedData->attendanceCount; index++) {
            if (!attendanceCompare(&loadedData->attendance[index - 1],
                                   &loadedData->attendance[index])) {
                result = appError(loadedData, RESULT_DUPLICATE,
                                  "Tệp attendance.txt có khóa nhân viên và ngày trùng: %s.",
                                  loadedData->attendance[index].employeeId);
                break;
            }
        }
    }
    if (result == RESULT_OK) {
        result = readBase(loadedData, "payroll_inputs.txt", 2); /* 3. Đọc thông số lương theo tháng */
    }
    if (result == RESULT_OK) {
        result = scanData(loadedData); /* 4. Kiểm tra .tmp và đọc các bảng chốt payroll_YYYY_MM.txt */
    }
    if (result == RESULT_OK) { /* Nạp toàn bộ thành công -> chuyển giao dữ liệu mới sang app */
        loadedData->dirty = 0;
        appFree(app);
        *app = *loadedData;
    } else { /* Thất bại -> giữ nguyên dữ liệu cũ của app, chỉ chép thông báo lỗi ra */
        snprintf(app->error, sizeof app->error, "%s", loadedData->error);
    }
    if (result != RESULT_OK) {
        appFree(loadedData); /* Giải phóng vùng nhớ tạm nếu nạp thất bại */
    }
    free(loadedData);
    return result;
}

/* Mở tệp tạm .tmp để chuẩn bị ghi an toàn; từ chối ghi nếu tệp .tmp đã tồn tại từ trước */
static FILE *openTemp(AppData *app, const char *fileName, char targetPath[PATH_CAP],
                      char temporaryPath[PATH_CAP]) {
    int pathLength;
    if (!joinPath(targetPath, app->dataDir, fileName)) {
        appError(app, RESULT_INVALID, "Đường dẫn quá dài.");
        return NULL;
    }
    pathLength = snprintf(temporaryPath, PATH_CAP, "%s.tmp", targetPath);
    if (pathLength < 0 || pathLength >= PATH_CAP) {
        appError(app, RESULT_INVALID, "Đường dẫn quá dài.");
        return NULL;
    }
    if (fileExists(temporaryPath)) {
        appError(app, RESULT_IO_ERROR, "Còn tệp tạm %s; không ghi đè.", temporaryPath);
        return NULL;
    }
    FILE *file = fopen(temporaryPath, "wb");
    if (!file) {
        appError(app, RESULT_IO_ERROR, "Không ghi được %s: %s.", temporaryPath, strerror(errno));
    }
    return file;
}

/* Quy trình ghi an toàn 3 bước: Kiểm tra đóng .tmp thành công -> Sao lưu tệp cũ sang .bak -> Đổi tên .tmp thành tệp chính. */
static Result commitTemp(AppData *app, FILE *file, const char *targetPath,
                         const char *temporaryPath, int replace) {
    char backupPath[PATH_CAP];
    int writeFailed = ferror(file); /* Kiểm tra xem quá trình fprintf trước đó có bị lỗi đĩa đầy/I/O không */
    if (fclose(file)) {             /* Đóng tệp để đẩy hết bộ đệm xuống đĩa và kiểm tra lỗi đóng tệp */
        writeFailed = 1;
    }
    if (writeFailed) { /* Nếu ghi lỗi -> xóa ngay .tmp hỏng, giữ nguyên tệp chính an toàn */
        remove(temporaryPath);
        return appError(app, RESULT_IO_ERROR, "Ghi hoặc đóng tệp thất bại: %s.", targetPath);
    }
    if (fileExists(targetPath)) { /* Nếu đã có tệp chính từ trước */
        if (!regularFile(targetPath)) {
            remove(temporaryPath);
            return appError(app, RESULT_INVALID,
                            "Đích không phải tệp thường; không thay thế thư mục: %s.", targetPath);
        }
        if (!replace) { /* Không cho phép ghi đè (ví dụ lệnh --export từ CLI hoặc snapshot đã chốt) */
            remove(temporaryPath);
            return appError(app, RESULT_DUPLICATE, "Tệp đã tồn tại: %s.", targetPath);
        }
        int pathLength = snprintf(backupPath, sizeof backupPath, "%s.bak", targetPath);
        if (pathLength < 0 || pathLength >= (int)sizeof backupPath) {
            remove(temporaryPath);
            return appError(app, RESULT_INVALID, "Đường dẫn sao lưu quá dài.");
        }
        if (fileExists(backupPath) && remove(backupPath)) { /* Xóa bản .bak cũ hơn nếu có (tránh lỗi rename trên Windows) */
            remove(temporaryPath);
            return appError(app, RESULT_IO_ERROR, "Không thay được bản sao lưu %s.", backupPath);
        }
        if (rename(targetPath, backupPath)) { /* Chuyển bản chính hiện tại thành bản sao lưu .bak */
            remove(temporaryPath);
            return appError(app, RESULT_IO_ERROR, "Không tạo được bản sao lưu %s.", backupPath);
        }
        if (rename(temporaryPath, targetPath)) { /* Công bố tệp mới từ .tmp thành tệp chính */
            /* Nếu bước đổi tên cuối lỗi -> cố gắng phục hồi lại tệp cũ từ .bak */
            int restored = rename(backupPath, targetPath) == 0;
            remove(temporaryPath);
            return appError(app, RESULT_IO_ERROR, "Không thay được tệp %s. Khôi phục cũ: %s.",
                            targetPath, restored ? "thành công" : "thất bại; giữ .bak để phục hồi");
        }
    } else if (rename(temporaryPath, targetPath)) { /* Chưa có tệp cũ -> đổi tên thẳng .tmp thành tệp chính */
        remove(temporaryPath);
        return appError(app, RESULT_IO_ERROR, "Không công bố được tệp %s.", targetPath);
    }
    return RESULT_OK;
}

/* Lưu cả 3 tệp dữ liệu nguồn (employees.txt, attendance.txt, payroll_inputs.txt) ra thư mục dữ liệu */
Result saveData(AppData *app) {
    const char *fileNames[] = {"employees.txt", "attendance.txt", "payroll_inputs.txt"};
    char targetPath[PATH_CAP], temporaryPath[PATH_CAP];
    if (!makeDirectory(app->dataDir)) {
        return appError(app, RESULT_IO_ERROR, "Không mở được thư mục dữ liệu.");
    }
    for (int fileKind = 0; fileKind < 3; fileKind++) {
        FILE *file = openTemp(app, fileNames[fileKind], targetPath, temporaryPath);
        if (!file) {
            return RESULT_IO_ERROR;
        }
        /* Ghi dòng header định danh loại tệp ở dòng 1 */
        fprintf(file, "%s|1\n",
                fileKind == 0   ? "EMPLOYEES"
                : fileKind == 1 ? "ATTENDANCE"
                                : "PAYROLL_INPUTS");
        if (fileKind == 0) { /* Xuất danh sách liên kết nhân viên */
            for (size_t index = 0; index < app->employeeCount; index++) {
                Employee *employee = employeeAt(app, index);
                char startDate[11], endDate[11] = "";
                dateIso(employee->startDate, startDate);
                if (employee->hasEndDate) {
                    dateIso(employee->endDate, endDate);
                }
                fprintf(file, "%s|%s|%s|%s|%s|%s|%lld|%lld|%d\n", employee->id, employee->fullName,
                        employee->department, employee->position, startDate, endDate,
                        employee->baseSalary, employee->allowance, employee->status);
            }
        }
        if (fileKind == 1) { /* Xuất mảng chấm công */
            for (size_t index = 0; index < app->attendanceCount; index++) {
                Attendance *record = &app->attendance[index];
                char dateText[11];
                dateIso(record->date, dateText);
                fprintf(file, "%s|%s|%d|%d|%d|%s\n", record->employeeId, dateText,
                        (int)record->type, record->regularHalfHours, record->overtimeHalfHours,
                        record->note);
            }
        }
        if (fileKind == 2) { /* Xuất mảng thông số lương theo tháng */
            for (size_t index = 0; index < app->inputCount; index++) {
                PayrollInput *input = &app->inputs[index];
                fprintf(file, "%s|%d|%d|%lld|%lld|%lld|%lld\n", input->employeeId, input->month,
                        input->year, input->baseSalary, input->allowance, input->bonus,
                        input->deduction);
            }
        }
        Result result = commitTemp(app, file, targetPath, temporaryPath, 1);
        if (result != RESULT_OK) { /* Nếu có lỗi ở bất kỳ tệp nào, giữ nguyên cờ dirty = 1 để cảnh báo chưa lưu hết */
            app->dirty = 1;
            return result;
        }
    }
    app->dirty = 0; /* Cả 3 tệp đã lưu thành công -> tắt cờ chưa lưu */
    return RESULT_OK;
}

/* Ghi tệp Snapshot bảng lương chính thức của một tháng đã chốt (không bao giờ ghi đè bản đã có) */
Result writePayrollSnapshot(AppData *app, int month, int year, const Payroll *rows, size_t count) {
    char fileName[80], targetPath[PATH_CAP], temporaryPath[PATH_CAP];
    snprintf(fileName, sizeof fileName, "payroll_%04d_%02d.txt", year, month);
    if (!joinPath(targetPath, app->dataDir, fileName)) {
        return appError(app, RESULT_INVALID, "Đường dẫn quá dài.");
    }
    if (fileExists(targetPath)) {
        return appError(app, RESULT_DUPLICATE, "Đã tồn tại bản chính thức; không ghi đè.");
    }
    FILE *file = openTemp(app, fileName, targetPath, temporaryPath);
    if (!file) {
        return RESULT_IO_ERROR;
    }
    fprintf(file, "PAYROLL|1|%d|%d|%zu\n", year, month, count);
    for (size_t index = 0; index < count; index++) {
        const Payroll *payroll = &rows[index];
        fprintf(file, "%s|%s|%s|%d|%d|%d|%d|%d|%lld|%lld|%lld|%lld|%lld|%lld|%lld|%lld\n",
                payroll->employeeId, payroll->employeeName, payroll->department, payroll->month,
                payroll->year, payroll->standardDays, payroll->paidHalfHours,
                payroll->overtimeHalfHours, payroll->baseSalary, payroll->allowance, payroll->bonus,
                payroll->deduction, payroll->regularPay, payroll->allowancePay,
                payroll->overtimePay, payroll->netPay);
    }
    return commitTemp(app, file, targetPath, temporaryPath, 0); /* replace = 0: tuyệt đối không ghi đè bản chốt */
}

/* Ghi một trường chuỗi ra định dạng CSV chuẩn: bọc trong ngoặc kép và nhân đôi dấu ngoặc kép bên trong nếu có */
static void csvField(FILE *file, const char *text) {
    fputc('"', file);
    for (; *text; text++) {
        if (*text == '"') {
            fputc('"', file);
        }
        fputc(*text, file);
    }
    fputc('"', file);
}

/* Xuất báo cáo bảng lương tháng ra tệp CSV có chèn UTF-8 BOM ở đầu tệp để Excel hiển thị tiếng Việt chuẩn */
Result exportPayroll(AppData *app, int month, int year, const char *fileName, int overwrite) {
    Payroll rows[MAX_EMPLOYEES];
    size_t rowCount;
    Result result = buildPayroll(app, month, year, rows, &rowCount);
    char temporaryPath[PATH_CAP];
    if (result != RESULT_OK) {
        return result;
    }
    if (fileExists(fileName) && !overwrite) {
        return appError(app, RESULT_DUPLICATE, "Tệp xuất đã tồn tại.");
    }
    int pathLength = snprintf(temporaryPath, sizeof temporaryPath, "%s.tmp", fileName);
    if (pathLength < 0 || pathLength >= (int)sizeof temporaryPath) {
        return appError(app, RESULT_INVALID, "Đường dẫn quá dài.");
    }
    if (fileExists(temporaryPath)) {
        return appError(app, RESULT_IO_ERROR, "Tệp tạm xuất đã tồn tại.");
    }
    FILE *file = fopen(temporaryPath, "wb");
    if (!file) {
        return appError(app, RESULT_IO_ERROR, "Không ghi được tệp xuất.");
    }
    /* Ghi 3 byte \xEF\xBB\xBF (UTF-8 BOM) ở đầu tệp CSV rồi đến dòng tiêu đề các cột */
    fprintf(file, "\xef\xbb\xbfKy,TrangThai,Ma,HoTen,PhongBan,CongChuan,CongHuong,GioTangCa,"
                  "LuongCong,PhuCap,TangCa,Thuong,KhauTru,ThucLinh\n");
    for (size_t index = 0; index < rowCount; index++) {
        Payroll *payroll = &rows[index];
        fprintf(file, "%02d/%d,%s,", month, year, isLocked(app, month, year) ? "DA CHOT" : "TAM");
        csvField(file, payroll->employeeId);
        fputc(',', file);
        csvField(file, payroll->employeeName);
        fputc(',', file);
        csvField(file, payroll->department);
        fprintf(file, ",%d,%.2f,%.2f,%lld,%lld,%lld,%lld,%lld,%lld\n", payroll->standardDays,
                payroll->paidHalfHours / 16.0, payroll->overtimeHalfHours / 2.0,
                payroll->regularPay, payroll->allowancePay, payroll->overtimePay, payroll->bonus,
                payroll->deduction, payroll->netPay);
    }
    return commitTemp(app, file, fileName, temporaryPath, overwrite);
}

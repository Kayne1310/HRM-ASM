#include "storage.h"
#include "input.h"
#include "date_utils.h"
#include "employee.h"
#include "attendance.h"
#include "payroll.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif
int fileExists(const char *filePath) {
    struct stat fileStatus;
    return stat(filePath, &fileStatus) == 0;
}

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

int makeDirectory(const char *directory) {
    struct stat fileStatus;
    if (stat(directory, &fileStatus) == 0) {
#ifdef _WIN32
        return (fileStatus.st_mode & _S_IFDIR) != 0;
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

static int joinPath(char output[PATH_CAP], const char *directory, const char *fileName) {
    int pathLength = snprintf(output, PATH_CAP, "%s/%s", directory, fileName);
    return pathLength >= 0 && pathLength < PATH_CAP;
}

/* Giữ cả trường rỗng: ví dụ ngày nghỉ việc chưa có trong chuỗi A||B. */ static int
splitFields(char *lineBuffer, char **fields, int maxFields) {
    int fieldCount = 1;
    fields[0] = lineBuffer;
    for (char *cursor = lineBuffer; *cursor; cursor++) {
        if (*cursor == '|') {
            *cursor = 0;
            if (fieldCount == maxFields) {
                return -1;
            }
            fields[fieldCount++] = cursor + 1;
        }
    }
    return fieldCount;
}

static int parseNumberField(const char *text, long long minValue, long long maxValue,
                            long long *value) {
    return parseInteger(text, minValue, maxValue, value);
}

static int parseIntField(const char *text, int minValue, int maxValue, int *value) {
    long long parsedValue;
    if (!parseNumberField(text, minValue, maxValue, &parsedValue)) {
        return 0;
    }
    *value = (int)parsedValue;
    return 1;
}

static int copyValidatedText(char *destination, size_t capacity, const char *source,
                             int allowEmpty) {
    if (!validText(source, capacity - 1, allowEmpty)) {
        return 0;
    }
    strcpy(destination, source);
    return 1;
}

static int parseIsoDate(const char *text, Date *date) {
    return strlen(text) == 10 && text[4] == '-' && parseDate(text, date);
}

static Result invalidFileRow(AppData *app, const char *fileName, int lineNumber) {
    return appError(app, RESULT_INVALID, "Tệp %s dòng %d: sai trường, dữ liệu hoặc khóa trùng.",
                    fileName, lineNumber);
}

/* Nạp lần lượt hồ sơ, công hoặc thông số tháng; sai một dòng thì dừng. */
static Result readBase(AppData *app, const char *fileName, int fileKind) {
    char filePath[PATH_CAP], lineBuffer[4096], *fields[20];
    FILE *file;
    int lineNumber = 1, readStatus, fieldCount;
    Result result = RESULT_OK;
    const char *header = fileKind == 0   ? "EMPLOYEES|1"
                         : fileKind == 1 ? "ATTENDANCE|1"
                                         : "PAYROLL_INPUTS|1";
    if (!joinPath(filePath, app->dataDir, fileName)) {
        return appError(app, RESULT_INVALID, "Đường dẫn quá dài.");
    }
    file = fopen(filePath, "rb");
    if (!file) {
        if (errno == ENOENT) {
            char bak[PATH_CAP];
            int k = snprintf(bak, sizeof bak, "%s.bak", filePath);
            if (k > 0 && k < (int)sizeof bak && fileExists(bak)) {
                return appError(app, RESULT_IO_ERROR,
                                "Thiếu tệp chính %s nhưng còn .bak; hãy phục hồi trước khi chạy.",
                                fileName);
            }
            return RESULT_OK;
        }
        return appError(app, RESULT_IO_ERROR, "Không mở được %s.", filePath);
    }
    if (readLine(file, lineBuffer, sizeof lineBuffer) != 1 || strcmp(lineBuffer, header)) {
        result = invalidFileRow(app, fileName, 1);
        goto done;
    }
    while ((readStatus = readLine(file, lineBuffer, sizeof lineBuffer)) != 0) {
        lineNumber++;
        if (readStatus != 1) {
            result = invalidFileRow(app, fileName, lineNumber);
            break;
        }
        fieldCount = splitFields(lineBuffer, fields, 20);
        if (fileKind == 0) {
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
            employee.hasEndDate = *fields[5] != 0;
            result = saveEmployee(app, &employee, 0);
        } else if (fileKind == 1) {
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
            if (result == RESULT_OK) {
                if (app->attendanceCount >= MAX_ATTENDANCE) {
                    result = appError(app, RESULT_FULL, "Đầy danh sách công.");
                } else if (!reserveArray((void **)&app->attendance, &app->attendanceCap,
                                         app->attendanceCount + 1, sizeof(Attendance))) {
                    result = appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ.");
                } else {
                    app->attendance[app->attendanceCount++] = record;
                }
            }
        } else {
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
        if (result != RESULT_OK) {
            char detail[300];
            snprintf(detail, sizeof detail, "%.290s", app->error);
            result = appError(app, result, "Tệp %s dòng %d: %s", fileName, lineNumber, detail);
            break;
        }
    }
    if (result == RESULT_OK && ferror(file)) {
        result = appError(app, RESULT_IO_ERROR, "Lỗi đọc %s.", fileName);
    }
done:
    if (fclose(file) && result == RESULT_OK) {
        result = appError(app, RESULT_IO_ERROR, "Lỗi đóng %s.", fileName);
    }
    return result;
}

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

/* Bản chốt phải đúng công thức và khớp dữ liệu nguồn của chính tháng đó. */
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
    if (readLine(file, lineBuffer, sizeof lineBuffer) != 1 ||
        splitFields(lineBuffer, fields, 20) != 5 || strcmp(fields[0], "PAYROLL") ||
        strcmp(fields[1], "1") || !parseIntField(fields[2], 2000, 2100, &year) ||
        !parseIntField(fields[3], 1, 12, &month) ||
        !parseIntField(fields[4], 0, 200, &expectedCount)) {
        result = invalidFileRow(app, fileName, 1);
        goto done;
    }
    snprintf(expected, sizeof expected, "payroll_%04d_%02d.txt", year, month);
    if (strcmp(fileName, expected) || isLocked(app, month, year)) {
        result = invalidFileRow(app, fileName, 1);
        goto done;
    }
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
        employeeIndex = findEmployeeIndex(app, payroll.employeeId);
        if (employeeIndex < 0 || !employeeInMonth(employeeAt(app, employeeIndex), month, year) ||
            payroll.standardDays != workingDaysInMonth(month, year) ||
            findInputIndex(app, payroll.employeeId, month, year) < 0) {
            result = invalidFileRow(app, fileName, lineNumber);
            goto done;
        }
        for (size_t index = firstPayrollIndex; index < app->payrollCount; index++) {
            if (!strcmp(app->payrolls[index].employeeId, payroll.employeeId)) {
                result = invalidFileRow(app, fileName, lineNumber);
                goto done;
            }
        }
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
    if (app->payrollCount - firstPayrollIndex != (size_t)expectedCount) {
        result = invalidFileRow(app, fileName, lineNumber);
        goto done;
    }
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
            if (!found) {
                result = appError(app, RESULT_INVALID,
                                  "Tệp %s không khớp công hoặc thông số tháng.", fileName);
                goto done;
            }
        }
    }
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
        if (nameLength >= 4 && !strcmp(fileName + nameLength - 4, ".tmp")) {
            result = appError(app, RESULT_IO_ERROR,
                              "Còn tệp tạm %s. Kiểm tra và phục hồi trước khi nạp; xem README.",
                              fileName);
            break;
        }
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

static int attendanceCompare(const void *leftPointer, const void *rightPointer) {
    const Attendance *left = leftPointer, *right = rightPointer;
    int comparison = strcmp(left->employeeId, right->employeeId);
    return comparison ? comparison : compareDate(left->date, right->date);
}

/* Nạp vào vùng nhớ tạm; chỉ thay dữ liệu đang dùng khi toàn bộ tệp hợp lệ. */
Result loadData(AppData *app) {
    AppData *loadedData = calloc(1, sizeof *loadedData);
    Result result;
    if (!loadedData) {
        return appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ nạp dữ liệu.");
    }
    appInit(loadedData, app->dataDir);
    if (!makeDirectory(app->dataDir)) {
        free(loadedData);
        return appError(app, RESULT_IO_ERROR, "Không tạo hoặc mở được thư mục dữ liệu.");
    }
    result = readBase(loadedData, "employees.txt", 0);
    if (result == RESULT_OK) {
        result = readBase(loadedData, "attendance.txt", 1);
    }
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
        result = readBase(loadedData, "payroll_inputs.txt", 2);
    }
    if (result == RESULT_OK) {
        result = scanData(loadedData);
    }
    if (result == RESULT_OK) {
        loadedData->dirty = 0;
        appFree(app);
        *app = *loadedData;
    } else {
        snprintf(app->error, sizeof app->error, "%s", loadedData->error);
    }
    if (result != RESULT_OK) {
        appFree(loadedData);
    }
    free(loadedData);
    return result;
}

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

/* Đóng thành công -> sao lưu tệp cũ -> đổi tên tệp tạm thành tệp chính. */
static Result commitTemp(AppData *app, FILE *file, const char *targetPath,
                         const char *temporaryPath, int replace) {
    char backupPath[PATH_CAP];
    int writeFailed = ferror(file);
    if (fclose(file)) {
        writeFailed = 1;
    }
    if (writeFailed) {
        remove(temporaryPath);
        return appError(app, RESULT_IO_ERROR, "Ghi hoặc đóng tệp thất bại: %s.", targetPath);
    }
    if (fileExists(targetPath)) {
        if (!regularFile(targetPath)) {
            remove(temporaryPath);
            return appError(app, RESULT_INVALID,
                            "Đích không phải tệp thường; không thay thế thư mục: %s.", targetPath);
        }
        if (!replace) {
            remove(temporaryPath);
            return appError(app, RESULT_DUPLICATE, "Tệp đã tồn tại: %s.", targetPath);
        }
        int pathLength = snprintf(backupPath, sizeof backupPath, "%s.bak", targetPath);
        if (pathLength < 0 || pathLength >= (int)sizeof backupPath) {
            remove(temporaryPath);
            return appError(app, RESULT_INVALID, "Đường dẫn sao lưu quá dài.");
        }
        if (fileExists(backupPath) && remove(backupPath)) {
            remove(temporaryPath);
            return appError(app, RESULT_IO_ERROR, "Không thay được bản sao lưu %s.", backupPath);
        }
        if (rename(targetPath, backupPath)) {
            remove(temporaryPath);
            return appError(app, RESULT_IO_ERROR, "Không tạo được bản sao lưu %s.", backupPath);
        }
        if (rename(temporaryPath, targetPath)) {
            int restored = rename(backupPath, targetPath) == 0;
            remove(temporaryPath);
            return appError(app, RESULT_IO_ERROR, "Không thay được tệp %s. Khôi phục cũ: %s.",
                            targetPath, restored ? "thành công" : "thất bại; giữ .bak để phục hồi");
        }
    } else if (rename(temporaryPath, targetPath)) {
        remove(temporaryPath);
        return appError(app, RESULT_IO_ERROR, "Không công bố được tệp %s.", targetPath);
    }
    return RESULT_OK;
}

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
        fprintf(file, "%s|1\n",
                fileKind == 0   ? "EMPLOYEES"
                : fileKind == 1 ? "ATTENDANCE"
                                : "PAYROLL_INPUTS");
        if (fileKind == 0) {
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
        if (fileKind == 1) {
            for (size_t index = 0; index < app->attendanceCount; index++) {
                Attendance *record = &app->attendance[index];
                char dateText[11];
                dateIso(record->date, dateText);
                fprintf(file, "%s|%s|%d|%d|%d|%s\n", record->employeeId, dateText,
                        (int)record->type, record->regularHalfHours, record->overtimeHalfHours,
                        record->note);
            }
        }
        if (fileKind == 2) {
            for (size_t index = 0; index < app->inputCount; index++) {
                PayrollInput *input = &app->inputs[index];
                fprintf(file, "%s|%d|%d|%lld|%lld|%lld|%lld\n", input->employeeId, input->month,
                        input->year, input->baseSalary, input->allowance, input->bonus,
                        input->deduction);
            }
        }
        Result result = commitTemp(app, file, targetPath, temporaryPath, 1);
        if (result != RESULT_OK) {
            app->dirty = 1;
            return result;
        }
    }
    app->dirty = 0;
    return RESULT_OK;
}

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
    return commitTemp(app, file, targetPath, temporaryPath, 0);
}

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

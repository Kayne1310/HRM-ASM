#include "ui.h"
#include "input.h"
#include "date_utils.h"
#include "employee.h"
#include "attendance.h"
#include "payroll.h"
#include "storage.h"
#include <stdlib.h> /* Dùng cho malloc, free */
#include <string.h> /* Dùng cho strlen, strcmp, strstr, memcpy */

/* In thông báo kết quả dựa vào mã trả về Result của các hàm nghiệp vụ */
void reportResult(AppData *app, Result result) {
    if (result == RESULT_OK) {
        puts("Thực hiện thành công.");
    } else {
        printf("Lỗi: %s\n", app->error); /* In chuỗi lỗi chi tiết đã được ghi sẵn vào app->error */
    }
}

/* In tiêu đề/danh sách lựa chọn menu và đọc số lựa chọn trong đoạn [0, maxChoice] */
int askMenuChoice(const char *title, int maxChoice) {
    long long selectedChoice;
    puts(title);
    return askInteger("Lựa chọn: ", 0, maxChoice, &selectedChoice) ? (int)selectedChoice : 0;
}

/* Nhắc người dùng nhập tháng (1..12) và năm (2000..2100) của một kỳ lương */
int askPeriod(int *month, int *year) {
    long long number;
    if (!askInteger("Tháng: ", 1, 12, &number)) {
        return 0;
    }
    *month = (int)number;
    if (!askInteger("Năm: ", 2000, 2100, &number)) {
        return 0;
    }
    *year = (int)number;
    return 1;
}

/* Nhắc nhập mã nhân viên rồi tự động chuẩn hóa viết hoa, bỏ khoảng trắng hai đầu */
int askEmployeeId(char employeeId[16]) {
    if (!askText("Mã nhân viên: ", employeeId, 16, 0)) {
        return 0;
    }
    normalizeId(employeeId);
    return 1;
}

/* Cắt ngắn chuỗi UTF-8 theo tối đa maxBytes mà không cắt đứt giữa chừng một ký tự tiếng Việt nhiều byte */
void utf8Prefix(const char *text, size_t maxBytes, char *output, size_t outputSize) {
    size_t length = strlen(text);
    if (!outputSize) {
        return;
    }
    if (length > maxBytes) {
        length = maxBytes;
    }
    if (length >= outputSize) {
        length = outputSize - 1;
    }
    /* Lùi lại nếu byte tại vị trí cắt là byte nối tiếp UTF-8 (có 2 bit đầu là 10xxxxxx) */
    while (length > 0 && ((unsigned char)text[length] & 0xc0) == 0x80) {
        length--;
    }
    memcpy(output, text, length); /* Sao chép phần chuỗi hợp lệ */
    output[length] = 0;           /* Đặt ký tự kết thúc chuỗi */
}

/* Điều khiển điều hướng phân trang 10 dòng/trang: N (trang sau), P (trang trước), 0 (thoát xem) */
static int pageNav(size_t *page, size_t count) {
    char input[16];
    printf("Trang %zu/%zu | N tiếp P trước 0 quay lại\n", *page + 1, (count + 9) / 10);
    for (;;) {
        if (!askText("> ", input, sizeof input, 0)) {
            return 0;
        }
        if (!strcmp(input, "0")) { /* Thoát phân trang */
            return 0;
        }
        if ((input[0] == 'N' || input[0] == 'n') && !input[1]) { /* Chuyển trang kế tiếp nếu chưa ở trang cuối */
            if ((*page + 1) * 10 < count) {
                (*page)++;
            }
            return 1;
        }
        if ((input[0] == 'P' || input[0] == 'p') && !input[1]) { /* Lùi lại trang trước nếu chưa ở trang đầu */
            if (*page) {
                (*page)--;
            }
            return 1;
        }
        puts("Chọn N P hoặc 0.");
    }
}

/* Lọc và in danh sách nhân viên ra bảng console (hỗ trợ lọc theo mã, tên chứa chuỗi con, phòng ban, trạng thái) */
void printEmployees(const AppData *app, const char *employeeId, const char *employeeName,
                    const char *department, int statusFilter, int paginate) {
    size_t matchedIndices[MAX_EMPLOYEES], matchCount = 0, page = 0;
    /* Lọc qua danh sách liên kết nhân viên, ghi lại chỉ số những người thỏa mãn tất cả điều kiện */
    for (size_t employeeIndex = 0; employeeIndex < app->employeeCount; employeeIndex++) {
        const Employee *employee = employeeAt(app, employeeIndex);
        if (employeeId && *employeeId && strcmp(employee->id, employeeId)) {
            continue;
        }
        if (employeeName && *employeeName && !strstr(employee->fullName, employeeName)) { /* strstr tìm chuỗi con */
            continue;
        }
        if (department && *department && strcmp(employee->department, department)) {
            continue;
        }
        if (statusFilter && employee->status != statusFilter) {
            continue;
        }
        matchedIndices[matchCount++] = employeeIndex;
    }
    if (!matchCount) {
        puts("Không có nhân viên phù hợp.");
        return;
    }
    do {
        size_t begin = paginate ? page * 10 : 0,
               end = paginate && begin + 10 < matchCount ? begin + 10 : matchCount;
        puts("MÃ              | HỌ TÊN                 | PHÒNG BAN        | LƯƠNG       | TRẠNG "
             "THÁI");
        for (size_t rowIndex = begin; rowIndex < end; rowIndex++) {
            const Employee *employee = employeeAt(app, matchedIndices[rowIndex]);
            char nameShort[101], deptShort[61];
            utf8Prefix(employee->fullName, 24, nameShort, sizeof nameShort);
            utf8Prefix(employee->department, 18, deptShort, sizeof deptShort);
            printf("%-15s | %-24s | %-18s | %11lld | %s\n", employee->id, nameShort, deptShort,
                   employee->baseSalary, employee->status == 1 ? "Đang làm" : "Đã nghỉ");
        }
        printf("Tổng phù hợp: %zu\n", matchCount);
    } while (paginate && matchCount > 10 && pageNav(&page, matchCount));
}

/* Lọc và in danh sách bản ghi chấm công (theo mã nhân viên, tháng/năm hoặc ngày cụ thể) */
void printAttendance(const AppData *app, const char *employeeId, int month, int year,
                     const Date *dateFilter, int paginate) {
    /* Cấp phát mảng động lưu các vị trí thỏa mãn vì số bản ghi công có thể rất lớn (tới 150.000) */
    size_t *matchedIndices = malloc((app->attendanceCount + 1) * sizeof(size_t)), matchCount = 0,
           page = 0;
    if (!matchedIndices) {
        puts("Không đủ bộ nhớ hiển thị.");
        return;
    }
    for (size_t recordIndex = 0; recordIndex < app->attendanceCount; recordIndex++) {
        const Attendance *record = &app->attendance[recordIndex];
        if (employeeId && *employeeId && strcmp(record->employeeId, employeeId)) {
            continue;
        }
        if (dateFilter && compareDate(record->date, *dateFilter)) {
            continue;
        }
        if (month && (record->date.month != month || record->date.year != year)) {
            continue;
        }
        matchedIndices[matchCount++] = recordIndex;
    }
    if (!matchCount) {
        puts("Không có bản ghi phù hợp.");
    } else {
        do {
            size_t begin = paginate ? page * 10 : 0,
                   end = paginate && begin + 10 < matchCount ? begin + 10 : matchCount;
            puts("MÃ | NGÀY | LOẠI | GIỜ THƯỜNG | TĂNG CA | GHI CHÚ");
            for (size_t rowIndex = begin; rowIndex < end; rowIndex++) {
                const Attendance *record = &app->attendance[matchedIndices[rowIndex]];
                /* Chia cho 2.0 để quy đổi số đơn vị nửa giờ thành số giờ thực tế khi in ra màn hình */
                printf("%s | %02d/%02d/%d | %s | %.1f | %.1f | %s\n", record->employeeId,
                       record->date.day, record->date.month, record->date.year,
                       record->type == ATT_WORK         ? "Đi làm"
                       : record->type == ATT_PAID_LEAVE ? "Nghỉ có lương"
                                                        : "Nghỉ không lương",
                       record->regularHalfHours / 2.0, record->overtimeHalfHours / 2.0,
                       record->note);
            }
            printf("Tổng bản ghi: %zu\n", matchCount);
        } while (paginate && matchCount > 10 && pageNav(&page, matchCount));
    }
    free(matchedIndices); /* Giải phóng mảng chỉ số tạm sau khi in xong */
}

/* In bảng lương tổng hợp của toàn bộ nhân viên trong kỳ và tổng quỹ lương */
void printPayroll(const Payroll *rows, size_t rowCount, int isFinal, int paginate) {
    size_t page = 0;
    long long total = 0; /* Biến cộng dồn tổng thực lĩnh toàn kỳ */
    for (size_t index = 0; index < rowCount; index++) {
        total += rows[index].netPay;
    }
    printf("BẢNG LƯƠNG %s\n", isFinal ? "ĐÃ CHỐT" : "TẠM - tính từ nguồn hiện tại");
    if (!rowCount) {
        puts("Không có nhân viên thuộc kỳ.");
        return;
    }
    do {
        size_t begin = paginate ? page * 10 : 0,
               end = paginate && begin + 10 < rowCount ? begin + 10 : rowCount;
        puts("MÃ | HỌ TÊN | CÔNG HƯỞNG | GIỜ TĂNG CA | THỰC LĨNH ĐỒNG");
        for (size_t index = begin; index < end; index++) {
            char shortName[101];
            utf8Prefix(rows[index].employeeName, 30, shortName, sizeof shortName);
            /* Chia paidHalfHours cho 16.0 để đổi sang số ngày công hưởng lương */
            printf("%s | %s | %.2f | %.1f | %lld\n", rows[index].employeeId, shortName,
                   rows[index].paidHalfHours / 16.0, rows[index].overtimeHalfHours / 2.0,
                   rows[index].netPay);
        }
        printf("Tổng %zu người, tổng thực lĩnh toàn kỳ: %lld đồng\n", rowCount, total);
    } while (paginate && rowCount > 10 && pageNav(&page, rowCount));
}

/* In chi tiết phiếu lương cá nhân của một nhân viên */
void printPayslip(const Payroll *payroll, int isFinal) {
    printf("\nPHIẾU LƯƠNG %02d/%d - %s\nMã: %s\nHọ tên: %s\nPhòng ban: %s\nCông chuẩn: %d | Công "
           "hưởng: %.2f | Tăng ca: %.1f giờ\nLương cơ bản tháng: %lld\nPhụ cấp tháng: %lld\nLương "
           "theo công: %lld\nPhụ cấp thực hưởng: %lld\nTiền tăng ca: %lld\nThưởng: %lld\nKhấu trừ: "
           "%lld\nTHỰC LĨNH: %lld đồng\n",
           payroll->month, payroll->year, isFinal ? "ĐÃ CHỐT" : "TẠM", payroll->employeeId,
           payroll->employeeName, payroll->department, payroll->standardDays,
           payroll->paidHalfHours / 16.0, payroll->overtimeHalfHours / 2.0, payroll->baseSalary,
           payroll->allowance, payroll->regularPay, payroll->allowancePay, payroll->overtimePay,
           payroll->bonus, payroll->deduction, payroll->netPay);
}

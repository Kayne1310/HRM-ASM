#include "ui.h"
#include "input.h"
#include "date_utils.h"
#include "employee.h"
#include "attendance.h"
#include "payroll.h"
#include "storage.h"
#include <stdlib.h> /* Dùng cho qsort sắp xếp mảng bảng lương */
#include <string.h> /* Dùng cho strcmp, strcpy */

/* Biểu mẫu cập nhật thông số lương tháng (lương CB, phụ cấp, thưởng, khấu trừ); không tự tính lương. */
static void payrollInputForm(AppData *app) {
    int month, year;
    char employeeId[16];
    long long money;
    if (!askPeriod(&month, &year) || !askEmployeeId(employeeId)) {
        return;
    }
    if (isLocked(app, month, year)) { /* Chặn sửa thông số tháng đã chốt */
        puts("Tháng đã chốt, không sửa thông số.");
        return;
    }
    int employeeIndex = findEmployeeIndex(app, employeeId);
    if (employeeIndex < 0 || !employeeInMonth(employeeAt(app, employeeIndex), month, year)) {
        puts("Nhân viên không thuộc kỳ.");
        return;
    }
    PayrollInput payrollInput = {0};
    int inputIndex = findInputIndex(app, employeeId, month, year);
    if (inputIndex >= 0) { /* Nếu tháng này đã có thông số -> nạp ra để sửa tiếp */
        payrollInput = app->inputs[inputIndex];
    } else { /* Chưa có -> lấy bản sao mặc định từ hồ sơ nhân viên */
        strcpy(payrollInput.employeeId, employeeId);
        payrollInput.month = month;
        payrollInput.year = year;
        payrollInput.baseSalary = employeeAt(app, employeeIndex)->baseSalary;
        payrollInput.allowance = employeeAt(app, employeeIndex)->allowance;
    }
    printf("Hiện tại: lương %lld, phụ cấp %lld, thưởng %lld, khấu trừ %lld\n",
           payrollInput.baseSalary, payrollInput.allowance, payrollInput.bonus,
           payrollInput.deduction);
    int menuChoice =
        askMenuChoice("\nTHÔNG SỐ LƯƠNG\n"
                      " 1. Nhập lương, phụ cấp, thưởng và khấu trừ\n"
                      " 2. Lấy lương và phụ cấp từ hồ sơ\n"
                      " 0. Hủy",
                      2);
    if (!menuChoice) {
        return;
    }
    if (menuChoice == 2) { /* Cập nhật lại lương & phụ cấp hiện tại từ hồ sơ (giữ thưởng/khấu trừ) */
        payrollInput.baseSalary = employeeAt(app, employeeIndex)->baseSalary;
        payrollInput.allowance = employeeAt(app, employeeIndex)->allowance;
    } else { /* Nhập thủ công từng khoản cho riêng tháng này */
        if (!askInteger("Lương cơ bản: ", 1, MAX_MONEY, &money)) {
            return;
        }
        payrollInput.baseSalary = money;
        if (!askInteger("Phụ cấp: ", 0, MAX_MONEY, &money)) {
            return;
        }
        payrollInput.allowance = money;
        if (!askInteger("Thưởng: ", 0, MAX_MONEY, &money)) {
            return;
        }
        payrollInput.bonus = money;
        if (!askInteger("Khấu trừ: ", 0, MAX_MONEY, &money)) {
            return;
        }
        payrollInput.deduction = money;
    }
    if (confirm("Y lưu thông số tháng, N hủy: ")) {
        reportResult(app, savePayrollInput(app, &payrollInput));
    }
}

/* Hàm so sánh 2 dòng lương theo mã nhân viên tăng dần (dùng cho qsort) */
static int comparePayrollById(const void *left, const void *right) {
    return strcmp(((const Payroll *)left)->employeeId, ((const Payroll *)right)->employeeId);
}

/* Hàm so sánh thực lĩnh tăng dần (nếu bằng nhau thì xếp theo mã nhân viên) */
static int comparePayrollAscending(const void *left, const void *right) {
    const Payroll *leftPayroll = left;
    const Payroll *rightPayroll = right;
    if (leftPayroll->netPay < rightPayroll->netPay) {
        return -1;
    }
    if (leftPayroll->netPay > rightPayroll->netPay) {
        return 1;
    }
    return comparePayrollById(left, right);
}

/* Hàm so sánh thực lĩnh giảm dần (nếu bằng nhau thì xếp theo mã nhân viên) */
static int comparePayrollDescending(const void *left, const void *right) {
    const Payroll *leftPayroll = left;
    const Payroll *rightPayroll = right;
    if (leftPayroll->netPay > rightPayroll->netPay) {
        return -1;
    }
    if (leftPayroll->netPay < rightPayroll->netPay) {
        return 1;
    }
    return comparePayrollById(left, right);
}

/* Biểu mẫu xuất bảng lương tháng ra tệp CSV (có hỏi xác nhận nếu tệp đã tồn tại) */
static void exportPayrollForm(AppData *app, int month, int year) {
    char filename[PATH_CAP], provided[PATH_CAP];
    snprintf(filename, sizeof filename, "exports/bang_luong_%04d_%02d.csv", year, month);
    printf("Mặc định: %s\n", filename);
    if (!askText("Tệp xuất (trống = mặc định): ", provided, sizeof provided, 1)) {
        return;
    }
    if (*provided) { /* Nếu người dùng nhập đường dẫn riêng */
        strcpy(filename, provided);
    } else if (!makeDirectory("exports")) { /* Nếu dùng mặc định thì đảm bảo thư mục exports tồn tại */
        puts("Không tạo được exports.");
        return;
    }
    int exists = fileExists(filename);
    if (exists && !confirm("Tệp đã có, Y ghi đè N hủy: ")) {
        return;
    }
    reportResult(app, exportPayroll(app, month, year, filename, exists));
}

/* Màn hình xem bảng lương của một kỳ và thực hiện các thao tác (phiếu lương, chốt, xuất CSV, sắp xếp) */
static void payrollView(AppData *app, int month, int year) {
    while (!inputEnded()) {
        Payroll rows[MAX_EMPLOYEES];
        size_t rowCount;
        Result result = buildPayroll(app, month, year, rows, &rowCount); /* Luôn tính lại từ nguồn mới nhất */
        if (result != RESULT_OK) {
            reportResult(app, result);
            return;
        }
        qsort(rows, rowCount, sizeof(Payroll), comparePayrollById); /* Mặc định xếp theo mã nhân viên */
        printf("\nKỳ %02d/%d\n", month, year);
        printPayroll(rows, rowCount, isLocked(app, month, year), 1);
        int menuChoice = askMenuChoice("\nTHAO TÁC BẢNG LƯƠNG\n"
                                       " 1. Xem phiếu lương\n"
                                       " 2. Chốt kỳ lương\n"
                                       " 3. Xuất CSV\n"
                                       " 4. Sắp xếp thực lĩnh tăng dần\n"
                                       " 5. Sắp xếp thực lĩnh giảm dần\n"
                                       " 0. Quay lại",
                                       5);
        if (!menuChoice) {
            return;
        }
        if (menuChoice == 1) { /* Xem phiếu lương chi tiết của 1 nhân viên */
            char employeeId[16];
            int found = 0;
            if (!askEmployeeId(employeeId)) {
                return;
            }
            for (size_t index = 0; index < rowCount; index++) {
                if (!strcmp(rows[index].employeeId, employeeId)) {
                    printPayslip(&rows[index], isLocked(app, month, year));
                    found = 1;
                }
            }
            if (!found) {
                puts("Không có phiếu lương của mã này trong kỳ.");
            }
        }
        if (menuChoice == 2) { /* Chốt kỳ lương (đóng băng dữ liệu bằng Snapshot) */
            if (isLocked(app, month, year)) {
                puts("Tháng đã chốt.");
            } else if (confirm("Sau chốt không sửa dữ liệu kỳ. Y chốt N hủy: ")) {
                reportResult(app, finalizePayroll(app, month, year));
            }
        }
        if (menuChoice == 3) { /* Xuất báo cáo CSV */
            exportPayrollForm(app, month, year);
        }
        if (menuChoice == 4 || menuChoice == 5) { /* Sắp xếp bảng lương theo thực lĩnh tăng/giảm */
            qsort(rows, rowCount, sizeof(Payroll),
                  menuChoice == 4 ? comparePayrollAscending : comparePayrollDescending);
            printPayroll(rows, rowCount, isLocked(app, month, year), 1);
        }
    }
}

/* Vòng lặp menu chức năng Bảng lương */
void payrollMenu(AppData *app) {
    while (!inputEnded()) {
        int menuChoice =
            askMenuChoice("\nBẢNG LƯƠNG\n"
                          " 1. Lập lại và xem bảng lương\n"
                          " 2. Cập nhật thông số lương\n"
                          " 3. Xem bảng lương theo tháng\n"
                          " 4. Xem phiếu lương theo mã nhân viên\n"
                          " 5. Chốt tháng\n"
                          " 6. Xuất CSV\n"
                          " 0. Quay lại",
                          6);
        int month, year;
        if (!menuChoice) {
            return;
        }
        if (menuChoice == 2) { /* Nhập thông số lương tháng (lương, phụ cấp, thưởng, khấu trừ) */
            payrollInputForm(app);
            continue;
        }
        if (!askPeriod(&month, &year)) {
            return;
        }
        if (menuChoice == 1 || menuChoice == 3) { /* Xem và thao tác trên bảng lương kỳ */
            payrollView(app, month, year);
        }
        if (menuChoice == 4) { /* Xem nhanh phiếu lương của 1 nhân viên trong kỳ */
            Payroll rows[MAX_EMPLOYEES];
            size_t rowCount;
            char employeeId[16];
            if (!askEmployeeId(employeeId)) {
                return;
            }
            Result result = buildPayroll(app, month, year, rows, &rowCount);
            if (result != RESULT_OK) {
                reportResult(app, result);
                continue;
            }
            int found = 0;
            for (size_t index = 0; index < rowCount; index++) {
                if (!strcmp(rows[index].employeeId, employeeId)) {
                    printPayslip(&rows[index], isLocked(app, month, year));
                    found = 1;
                }
            }
            if (!found) {
                puts("Không tìm thấy phiếu lương.");
            }
        }
        if (menuChoice == 5) { /* Chốt trực tiếp tháng từ menu */
            Payroll rows[MAX_EMPLOYEES];
            size_t rowCount;
            Result result = buildPayroll(app, month, year, rows, &rowCount);
            if (result != RESULT_OK) {
                reportResult(app, result);
                continue;
            }
            printPayroll(rows, rowCount, isLocked(app, month, year), 0);
            if (confirm("Y chốt tháng, N hủy: ")) {
                reportResult(app, finalizePayroll(app, month, year));
            }
        }
        if (menuChoice == 6) { /* Xuất CSV trực tiếp từ menu */
            exportPayrollForm(app, month, year);
        }
    }
}

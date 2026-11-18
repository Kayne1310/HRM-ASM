#include "ui.h"
#include "input.h"
#include "employee.h"
#include "date_utils.h"

/* Biểu mẫu được nhập vào bản sao; chỉ cập nhật danh sách sau khi xác nhận. */
static void employeeForm(AppData *app, int editing) {
    Employee employee = {0};
    if (!askEmployeeId(employee.id)) {
        return;
    }
    if (editing) {
        Employee *existing = findEmployee(app, employee.id);
        if (existing == NULL) {
            puts("Không tìm thấy nhân viên.");
            return;
        }
        employee = *existing;
        printf("Sửa %s | %s. Mã giữ nguyên.\n", employee.id, employee.fullName);
    }
    if (!askText("Họ tên: ", employee.fullName, sizeof employee.fullName, 0)) {
        return;
    }
    if (!askText("Phòng ban: ", employee.department, sizeof employee.department, 0)) {
        return;
    }
    if (!askText("Chức vụ: ", employee.position, sizeof employee.position, 0)) {
        return;
    }
    if (!askDate("Ngày vào: ", &employee.startDate)) {
        return;
    }
    if (!askInteger("Lương cơ bản: ", 1, MAX_MONEY, &employee.baseSalary)) {
        return;
    }
    if (!askInteger("Phụ cấp: ", 0, MAX_MONEY, &employee.allowance)) {
        return;
    }
    if (!editing) {
        employee.status = 1;
    }
    if (confirm("Y lưu vào danh sách, N hủy: ")) {
        reportResult(app, saveEmployee(app, &employee, editing));
    }
}

static void listEmployeesForm(const AppData *app) {
    char department[61];
    long long status;
    if (!askText("Phòng ban (trống = tất cả): ", department, sizeof department, 1)) {
        return;
    }
    if (!askInteger("Trạng thái 0 tất cả 1 đang làm 2 đã nghỉ: ", 0, 2, &status)) {
        return;
    }
    printEmployees(app, NULL, NULL, department, (int)status, 1);
}

static void recordEmployeeLeave(AppData *app) {
    char employeeId[16];
    if (!askEmployeeId(employeeId)) {
        return;
    }
    Employee *existing = findEmployee(app, employeeId);
    if (existing == NULL) {
        puts("Không tìm thấy.");
        return;
    }
    Employee employee = *existing;
    printf("%s | %s\n", employee.id, employee.fullName);
    if (!askDate("Ngày làm việc cuối: ", &employee.endDate)) {
        return;
    }
    employee.hasEndDate = 1;
    employee.status = 2;
    if (confirm("Y ghi nhận nghỉ việc, N hủy: ")) {
        reportResult(app, saveEmployee(app, &employee, 1));
    }
}

static void deleteEmployeeForm(AppData *app) {
    char employeeId[16];
    if (!askEmployeeId(employeeId)) {
        return;
    }
    if (confirm("Y xác nhận xóa hẳn, N hủy: ")) {
        reportResult(app, deleteEmployee(app, employeeId));
    }
}

static void showEmployeeDetails(const AppData *app) {
    char employeeId[16];
    char startDate[11];
    char endDate[32] = "Chưa nghỉ";
    if (!askEmployeeId(employeeId)) {
        return;
    }
    const Employee *employee = findEmployee(app, employeeId);
    if (employee == NULL) {
        puts("Không tìm thấy nhân viên.");
        return;
    }
    dateIso(employee->startDate, startDate);
    if (employee->hasEndDate) {
        dateIso(employee->endDate, endDate);
    }
    printf("Mã: %s\n", employee->id);
    printf("Họ tên: %s\n", employee->fullName);
    printf("Phòng ban: %s\n", employee->department);
    printf("Chức vụ: %s\n", employee->position);
    printf("Ngày vào: %s\n", startDate);
    printf("Ngày làm cuối: %s\n", endDate);
    printf("Lương cơ bản: %lld\n", employee->baseSalary);
    printf("Phụ cấp: %lld\n", employee->allowance);
    printf("Trạng thái: %s\n", employee->status == 1 ? "Đang làm" : "Đã nghỉ");
}

void employeeMenu(AppData *app) {
    const char *menu = "\nNHÂN VIÊN\n"
                       " 1. Thêm nhân viên\n"
                       " 2. Danh sách và lọc\n"
                       " 3. Sửa hồ sơ\n"
                       " 4. Ghi nhận nghỉ việc\n"
                       " 5. Xóa hồ sơ\n"
                       " 6. Xem chi tiết\n"
                       " 7. Sắp xếp theo mã\n"
                       " 0. Quay lại";
    while (!inputEnded()) {
        int menuChoice = askMenuChoice(menu, 7);
        switch (menuChoice) {
        case 0:
            return;
        case 1:
            employeeForm(app, 0);
            break;
        case 2:
            listEmployeesForm(app);
            break;
        case 3:
            employeeForm(app, 1);
            break;
        case 4:
            recordEmployeeLeave(app);
            break;
        case 5:
            deleteEmployeeForm(app);
            break;
        case 6:
            showEmployeeDetails(app);
            break;
        case 7:
            sortEmployeesById(app);
            puts("Đã sắp xếp danh sách liên kết theo mã nhân viên.");
            printEmployees(app, NULL, NULL, NULL, 0, 1);
            break;
        }
    }
}

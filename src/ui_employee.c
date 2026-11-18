#include "ui.h"
#include "input.h"
#include "employee.h"
#include "date_utils.h"

/* Biểu mẫu thêm mới (editing=0) hoặc sửa (editing=1) hồ sơ nhân viên; nhập vào bản sao rồi mới lưu. */
static void employeeForm(AppData *app, int editing) {
    Employee employee = {0}; /* Khởi tạo bản sao trắng */
    if (!askEmployeeId(employee.id)) {
        return;
    }
    if (editing) { /* Nếu là chế độ sửa -> nạp thông tin hiện có vào bản sao để giữ nguyên mã & ngày nghỉ cũ */
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
        employee.status = 1; /* Nhân viên mới thêm mặc định đang làm việc (status = 1) */
    }
    if (confirm("Y lưu vào danh sách, N hủy: ")) { /* Xác nhận trước khi cập nhật danh sách liên kết */
        reportResult(app, saveEmployee(app, &employee, editing));
    }
}

/* Biểu mẫu nhập điều kiện lọc và hiển thị danh sách nhân viên */
static void listEmployeesForm(const AppData *app) {
    char department[61];
    long long status;
    if (!askText("Phòng ban (trống = tất cả): ", department, sizeof department, 1)) {
        return;
    }
    if (!askInteger("Trạng thái 0 tất cả 1 đang làm 2 đã nghỉ: ", 0, 2, &status)) {
        return;
    }
    printEmployees(app, NULL, NULL, department, (int)status, 1); /* Bật phân trang 10 dòng */
}

/* Biểu mẫu ghi nhận ngày làm việc cuối cùng cho nhân viên nghỉ việc (thay cho xóa cứng) */
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
    employee.hasEndDate = 1; /* Bật cờ đã có ngày nghỉ */
    employee.status = 2;     /* Chuyển trạng thái sang 2 (đã nghỉ) */
    if (confirm("Y ghi nhận nghỉ việc, N hủy: ")) {
        reportResult(app, saveEmployee(app, &employee, 1));
    }
}

/* Biểu mẫu xóa hẳn hồ sơ nhân viên (chỉ thành công nếu chưa có công, thông số hay bảng lương) */
static void deleteEmployeeForm(AppData *app) {
    char employeeId[16];
    if (!askEmployeeId(employeeId)) {
        return;
    }
    if (confirm("Y xác nhận xóa hẳn, N hủy: ")) {
        reportResult(app, deleteEmployee(app, employeeId));
    }
}

/* Xem đầy đủ toàn bộ các trường thông tin của 1 nhân viên */
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
    dateIso(employee->startDate, startDate); /* Đổi ngày vào sang chuỗi yyyy-mm-dd */
    if (employee->hasEndDate) {
        dateIso(employee->endDate, endDate); /* Đổi ngày nghỉ nếu có */
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

/* Vòng lặp điều khiển menu quản lý nhân viên */
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
            employeeForm(app, 0); /* Thêm mới */
            break;
        case 2:
            listEmployeesForm(app); /* Xem & lọc danh sách */
            break;
        case 3:
            employeeForm(app, 1); /* Sửa hồ sơ */
            break;
        case 4:
            recordEmployeeLeave(app); /* Ghi nhận nghỉ việc */
            break;
        case 5:
            deleteEmployeeForm(app); /* Xóa hồ sơ */
            break;
        case 6:
            showEmployeeDetails(app); /* Xem chi tiết 1 hồ sơ */
            break;
        case 7:
            sortEmployeesById(app); /* Chạy Merge Sort đệ quy sắp xếp danh sách liên kết theo mã */
            puts("Đã sắp xếp danh sách liên kết theo mã nhân viên.");
            printEmployees(app, NULL, NULL, NULL, 0, 1);
            break;
        }
    }
}

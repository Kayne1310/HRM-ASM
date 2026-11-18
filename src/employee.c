#include "employee.h"
#include "date_utils.h"

#include <stdlib.h> /* Dùng cho malloc, free */
#include <string.h> /* Dùng cho strcmp */

/*
 * Danh sách liên kết đơn là nơi lưu hồ sơ nhân viên chính của chương trình.
 * Không có một mảng nhân viên thứ hai để đồng bộ với danh sách này.
 */

/* Lấy con trỏ tới hồ sơ nhân viên tại vị trí index (0, 1, 2...) trong danh sách liên kết đơn */
Employee *employeeAt(const AppData *app, size_t index) {
    EmployeeNode *current = app->employeeHead; /* Bắt đầu từ nút đầu danh sách */
    size_t position = 0;                       /* Biến đếm vị trí hiện tại */

    while (current != NULL) {                  /* Duyệt qua từng nút cho tới khi gặp NULL */
        if (position == index) {               /* Đúng vị trí cần tìm -> trả về địa chỉ Employee bên trong nút */
            return &current->employee;
        }
        current = current->next;               /* Chuyển sang nút kế tiếp */
        position++;
    }
    return NULL; /* Index vượt quá số lượng nhân viên hiện có */
}

/* Tìm kiếm nhân viên theo mã employeeId trong danh sách liên kết đơn; trả về con trỏ Employee hoặc NULL */
Employee *findEmployee(const AppData *app, const char *employeeId) {
    EmployeeNode *current = app->employeeHead;

    while (current != NULL) {
        if (strcmp(current->employee.id, employeeId) == 0) { /* So sánh chuỗi mã nhân viên; == 0 nghĩa là giống nhau */
            return &current->employee;                       /* Trả về địa chỉ hồ sơ nằm ngay trong nút tìm thấy */
        }
        current = current->next;
    }
    return NULL; /* Không tìm thấy nhân viên */
}

/* Tìm vị trí thứ tự (0, 1, 2...) dùng để đánh dấu cột nhân viên trong bảng tổng hợp công, không phải chỉ số mảng. */
int findEmployeeIndex(const AppData *app, const char *employeeId) {
    EmployeeNode *current = app->employeeHead;
    int index = 0;

    while (current != NULL) {
        if (strcmp(current->employee.id, employeeId) == 0) {
            return index; /* Trả về thứ tự xuất hiện trong danh sách */
        }
        current = current->next;
        index++;
    }
    return -1; /* Không tìm thấy -> trả về -1 */
}

/* Kiểm tra toàn bộ các trường dữ liệu của một hồ sơ nhân viên xem có hợp lệ không */
static Result validateEmployeeFields(AppData *app, const Employee *employee) {
    if (!validId(employee->id)) { /* Kiểm tra mã: 1-15 ký tự A-Z, 0-9, _ hoặc - */
        return appError(app, RESULT_INVALID, "Mã gồm 1 đến 15 ký tự A-Z, 0-9, _ hoặc -.");
    }
    if (!validText(employee->fullName, 100, 0) || !validText(employee->department, 60, 0) ||
        !validText(employee->position, 60, 0)) { /* Họ tên, phòng ban, chức vụ không được rỗng và đúng UTF-8 */
        return appError(app, RESULT_INVALID, "Họ tên, phòng ban hoặc chức vụ không hợp lệ.");
    }
    if (!isValidDate(employee->startDate)) { /* Kiểm tra ngày vào làm có thực tế trong lịch không */
        return appError(app, RESULT_INVALID, "Ngày vào làm không hợp lệ.");
    }
    if (employee->baseSalary <= 0 || employee->baseSalary > MAX_MONEY || employee->allowance < 0 ||
        employee->allowance > MAX_MONEY) { /* Lương cơ bản > 0, phụ cấp >= 0, tối đa 1 tỷ đồng */
        return appError(app, RESULT_INVALID,
                        "Lương phải dương; phụ cấp không âm; mỗi khoản tối đa 1 tỷ đồng.");
    }
    if (employee->hasEndDate != 0 && employee->hasEndDate != 1) { /* Cờ đánh dấu nghỉ việc chỉ được là 0 hoặc 1 */
        return appError(app, RESULT_INVALID, "Cờ ngày nghỉ chỉ nhận 0 hoặc 1.");
    }
    if (employee->status != (employee->hasEndDate ? 2 : 1)) { /* Trạng thái phải khớp: có ngày nghỉ -> status 2, chưa -> status 1 */
        return appError(app, RESULT_INVALID, "Trạng thái nhân viên không khớp ngày nghỉ việc.");
    }
    if (employee->hasEndDate && (!isValidDate(employee->endDate) ||
                                 compareDate(employee->endDate, employee->startDate) < 0)) { /* Ngày nghỉ phải hợp lệ và >= ngày vào */
        return appError(app, RESULT_INVALID, "Ngày làm cuối không hợp lệ hoặc trước ngày vào làm.");
    }
    return RESULT_OK;
}

/* Khi sửa ngày vào/ngày nghỉ của nhân viên, kiểm tra không được làm mất hiệu lực các công/thông số đã lưu trước đó */
static Result validateExistingRecords(AppData *app, const Employee *employee) {
    for (size_t index = 0; index < app->attendanceCount; index++) { /* Duyệt toàn bộ dữ liệu chấm công hiện có */
        const Attendance *record = &app->attendance[index];
        if (strcmp(record->employeeId, employee->id) == 0 &&
            !employeeOnDate(employee, record->date)) { /* Nếu ngày đã chấm công lại nằm ngoài khoảng ngày làm việc mới -> báo lỗi */
            return appError(app, RESULT_INVALID,
                            "Thay đổi làm chấm công hiện có nằm ngoài thời gian làm việc.");
        }
    }
    for (size_t index = 0; index < app->inputCount; index++) { /* Duyệt toàn bộ thông số lương tháng hiện có */
        const PayrollInput *input = &app->inputs[index];
        if (strcmp(input->employeeId, employee->id) == 0 &&
            !employeeInMonth(employee, input->month, input->year)) { /* Nếu tháng đã có thông số lại không thuộc khoảng làm việc mới -> lỗi */
            return appError(app, RESULT_INVALID,
                            "Thay đổi làm thông số lương tháng hiện có không còn hợp lệ.");
        }
    }
    return RESULT_OK;
}

/* Chặn việc sửa đổi ngày vào/ngày nghỉ nếu làm thay đổi tập ngày làm việc hoặc danh sách nhân sự của các tháng ĐÃ CHỐT */
static Result validateLockedMonths(AppData *app, const Employee *before, const Employee *after) {
    for (size_t index = 0; index < app->lockCount; index++) { /* Duyệt qua từng kỳ tháng đã chốt khóa */
        MonthLock lockedMonth = app->locks[index];
        int lastDay = daysInMonth(lockedMonth.month, lockedMonth.year);

        for (int day = 1; day <= lastDay; day++) { /* Kiểm tra từng ngày làm việc trong tháng đã chốt */
            Date date = {day, lockedMonth.month, lockedMonth.year};
            int workedBefore = before != NULL && employeeOnDate(before, date); /* Trước khi sửa có làm ngày này không */
            int worksAfter = employeeOnDate(after, date);                      /* Sau khi sửa có làm ngày này không */

            if (isWorkingDay(date) && workedBefore != worksAfter) { /* Bị sai khác -> ảnh hưởng phạm vi công tháng đã chốt -> chặn */
                return appError(app, RESULT_LOCKED,
                                "Thay đổi ảnh hưởng phạm vi công của tháng %02d/%d đã chốt.",
                                lockedMonth.month, lockedMonth.year);
            }
        }
        int belongedBefore =
            before != NULL && employeeInMonth(before, lockedMonth.month, lockedMonth.year);
        int belongsAfter = employeeInMonth(after, lockedMonth.month, lockedMonth.year);
        if (belongedBefore != belongsAfter) { /* Làm tăng hoặc giảm số người thuộc tháng đã chốt -> chặn */
            return appError(app, RESULT_LOCKED,
                            "Thay đổi danh sách nhân viên trong tháng đã chốt.");
        }
    }
    return RESULT_OK;
}

/* Cấp phát nút mới và nối vào CUỐI danh sách liên kết đơn */
static Result appendEmployee(AppData *app, const Employee *employee) {
    EmployeeNode *newNode = malloc(sizeof *newNode); /* Xin cấp phát 1 nút EmployeeNode trên bộ nhớ Heap */
    if (newNode == NULL) {                           /* Kiểm tra hết RAM */
        return appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ thêm nhân viên.");
    }
    newNode->employee = *employee; /* Chép toàn bộ dữ liệu hồ sơ vào nút mới */
    newNode->next = NULL;          /* Vì nối vào cuối nên con trỏ next của nút mới phải là NULL */

    if (app->employeeHead == NULL) { /* Nếu danh sách đang rỗng -> nút mới chính là nút đầu tiên */
        app->employeeHead = newNode;
    } else { /* Danh sách đã có phần tử -> duyệt tới nút cuối cùng rồi nối vào */
        EmployeeNode *last = app->employeeHead;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = newNode;
    }
    app->employeeCount++; /* Tăng biến đếm số nhân viên */
    return RESULT_OK;
}

/* Kiểm tra mọi quy tắc nghiệp vụ rồi lưu hồ sơ nhân viên (thêm mới hoặc sửa) */
Result saveEmployee(AppData *app, const Employee *employee, int editing) {
    Employee *existing = findEmployee(app, employee->id); /* Tìm xem mã nhân viên đã tồn tại chưa */
    Result result = validateEmployeeFields(app, employee);
    if (result != RESULT_OK) {
        return result;
    }
    if (editing && existing == NULL) { /* Chế độ sửa nhưng không tìm thấy mã */
        return appError(app, RESULT_NOT_FOUND, "Không tìm thấy mã %s.", employee->id);
    }
    if (!editing && existing != NULL) { /* Chế độ thêm mới nhưng mã đã bị trùng */
        return appError(app, RESULT_DUPLICATE, "Mã %s đã tồn tại.", employee->id);
    }
    if (!editing && app->employeeCount >= MAX_EMPLOYEES) { /* Vượt quá giới hạn tối đa 200 nhân viên */
        return appError(app, RESULT_FULL, "Đã đủ 200 nhân viên.");
    }
    result = validateExistingRecords(app, employee);
    if (result != RESULT_OK) {
        return result;
    }
    result = validateLockedMonths(app, editing ? existing : NULL, employee);
    if (result != RESULT_OK) {
        return result;
    }

    if (editing) {
        /* Sửa dữ liệu ngay trong nút tìm được, giữ nguyên con trỏ liên kết next. */
        *existing = *employee;
    } else {
        result = appendEmployee(app, employee); /* Thêm mới vào cuối danh sách */
        if (result != RESULT_OK) {
            return result;
        }
    }
    app->dirty = 1; /* Đánh dấu dữ liệu trong RAM đã thay đổi, cần lưu ra file trước khi thoát */
    return RESULT_OK;
}

/* Kiểm tra ràng buộc toàn vẹn trước khi xóa: không cho xóa nhân viên đã phát sinh công, thông số hay bảng lương */
static Result validateEmployeeDeletion(AppData *app, const char *employeeId) {
    for (size_t index = 0; index < app->attendanceCount; index++) {
        if (strcmp(app->attendance[index].employeeId, employeeId) == 0) {
            return appError(app, RESULT_INVALID, "Đã có công; hãy ghi nhận nghỉ việc.");
        }
    }
    for (size_t index = 0; index < app->inputCount; index++) {
        if (strcmp(app->inputs[index].employeeId, employeeId) == 0) {
            return appError(app, RESULT_INVALID, "Đã có thông số lương; không xóa hồ sơ.");
        }
    }
    for (size_t index = 0; index < app->payrollCount; index++) {
        if (strcmp(app->payrolls[index].employeeId, employeeId) == 0) {
            return appError(app, RESULT_INVALID, "Đã có bảng lương; không xóa hồ sơ.");
        }
    }
    return RESULT_OK;
}

/* Xóa một nút nhân viên khỏi danh sách liên kết đơn (xử lý đủ 3 trường hợp: đầu, giữa, cuối) */
Result deleteEmployee(AppData *app, const char *employeeId) {
    EmployeeNode *previous = NULL;             /* Con trỏ lưu nút đứng liền trước nút cần xóa */
    EmployeeNode *current = app->employeeHead; /* Con trỏ duyệt tìm nút cần xóa */

    while (current != NULL && strcmp(current->employee.id, employeeId) != 0) {
        previous = current;
        current = current->next;
    }
    if (current == NULL) { /* Duyệt hết danh sách không thấy mã cần xóa */
        return appError(app, RESULT_NOT_FOUND, "Không tìm thấy nhân viên.");
    }
    Result result = validateEmployeeDeletion(app, employeeId); /* Kiểm tra đã phát sinh dữ liệu liên quan chưa */
    if (result != RESULT_OK) {
        return result;
    }

    if (previous == NULL) {
        app->employeeHead = current->next; /* Trường hợp xóa nút đầu: cập nhật employeeHead trỏ sang nút thứ 2 */
    } else {
        previous->next = current->next; /* Trường hợp xóa nút giữa hoặc cuối: nối thẳng nút trước qua nút sau */
    }
    free(current);        /* Giải phóng vùng nhớ của nút bị xóa để tránh rò rỉ bộ nhớ (memory leak) */
    app->employeeCount--; /* Giảm số lượng nhân viên */
    app->dirty = 1;       /* Đánh dấu có thay đổi chưa lưu */
    return RESULT_OK;
}

/* Tách danh sách liên kết thành 2 nửa bằng kỹ thuật 2 con trỏ: slow đi 1 bước, fast đi 2 bước. */
static EmployeeNode *splitEmployeeList(EmployeeNode *head) {
    EmployeeNode *slow = head;
    EmployeeNode *fast = head->next;
    while (fast != NULL && fast->next != NULL) { /* Khi fast chạm cuối danh sách thì slow đứng đúng ở điểm giữa */
        slow = slow->next;
        fast = fast->next->next;
    }
    EmployeeNode *rightHalf = slow->next; /* Nửa bên phải bắt đầu từ sau slow */
    slow->next = NULL;                    /* Cắt đứt liên kết giữa nửa trái và nửa phải */
    return rightHalf;
}

/* Trộn hai danh sách đã sắp xếp thành một danh sách tăng dần theo mã nhân viên (dùng vòng lặp để tránh tràn stack). */
static EmployeeNode *mergeEmployeeLists(EmployeeNode *left, EmployeeNode *right) {
    EmployeeNode temporaryHead = {0}; /* Nút giả (dummy node) làm điểm tựa nối đầu danh sách kết quả */
    EmployeeNode *tail = &temporaryHead;

    while (left != NULL && right != NULL) { /* So sánh phần tử đầu của 2 danh sách, chọn mã nhỏ hơn nối vào tail */
        if (strcmp(left->employee.id, right->employee.id) <= 0) {
            tail->next = left;
            left = left->next;
        } else {
            tail->next = right;
            right = right->next;
        }
        tail = tail->next;
    }
    tail->next = left != NULL ? left : right; /* Nối nốt phần còn thừa của danh sách chưa rỗng vào đuôi */
    return temporaryHead.next;                /* Trả về nút đầu tiên thực sự (sau nút giả) */
}

/*
 * Đệ quy có ý nghĩa thực tế: Merge Sort sắp xếp danh sách nhân viên.
 * Điểm dừng: danh sách rỗng hoặc chỉ có một nút.
 * Mỗi lần gọi chia đôi số nút, độ sâu O(log n), thời gian O(n log n).
 * Chỉ nối lại các nút, không cấp phát, sao chép hay làm mất hồ sơ.
 */
static EmployeeNode *mergeSortEmployees(EmployeeNode *head) {
    if (head == NULL || head->next == NULL) { /* Điều kiện dừng đệ quy: 0 hoặc 1 nút đã tự có thứ tự */
        return head;
    }
    EmployeeNode *rightHalf = splitEmployeeList(head);          /* Bước chia (Divide): tách đôi danh sách */
    EmployeeNode *sortedLeft = mergeSortEmployees(head);        /* Đệ quy sắp xếp nửa trái */
    EmployeeNode *sortedRight = mergeSortEmployees(rightHalf);  /* Đệ quy sắp xếp nửa phải */
    return mergeEmployeeLists(sortedLeft, sortedRight);         /* Bước trị (Conquer/Combine): trộn 2 nửa đã xếp */
}

/* Hàm gọi công khai để sắp xếp danh sách nhân viên theo mã nhân viên tăng dần */
void sortEmployeesById(AppData *app) {
    if (app->employeeCount < 2) { /* Ít hơn 2 người thì không cần sắp xếp */
        return;
    }
    app->employeeHead = mergeSortEmployees(app->employeeHead); /* Cập nhật lại con trỏ đầu danh sách sau khi sắp xếp */
    app->dirty = 1; /* Lưu lại thứ tự mới khi người dùng chọn lưu. */
}

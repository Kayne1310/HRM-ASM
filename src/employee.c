#include "employee.h"
#include "date_utils.h"

#include <stdlib.h>
#include <string.h>

/*
 * Danh sách liên kết đơn là nơi lưu hồ sơ nhân viên chính của chương trình.
 * Không có một mảng nhân viên thứ hai để đồng bộ với danh sách này.
 */
Employee *employeeAt(const AppData *app, size_t index) {
    EmployeeNode *current = app->employeeHead;
    size_t position = 0;

    while (current != NULL) {
        if (position == index) {
            return &current->employee;
        }
        current = current->next;
        position++;
    }
    return NULL;
}

Employee *findEmployee(const AppData *app, const char *employeeId) {
    EmployeeNode *current = app->employeeHead;

    while (current != NULL) {
        if (strcmp(current->employee.id, employeeId) == 0) {
            return &current->employee;
        }
        current = current->next;
    }
    return NULL;
}

/* Chỉ số dùng cho bảng tổng hợp công theo tháng, không phải vị trí trong mảng. */
int findEmployeeIndex(const AppData *app, const char *employeeId) {
    EmployeeNode *current = app->employeeHead;
    int index = 0;

    while (current != NULL) {
        if (strcmp(current->employee.id, employeeId) == 0) {
            return index;
        }
        current = current->next;
        index++;
    }
    return -1;
}

static Result validateEmployeeFields(AppData *app, const Employee *employee) {
    if (!validId(employee->id)) {
        return appError(app, RESULT_INVALID, "Mã gồm 1 đến 15 ký tự A-Z, 0-9, _ hoặc -.");
    }
    if (!validText(employee->fullName, 100, 0) || !validText(employee->department, 60, 0) ||
        !validText(employee->position, 60, 0)) {
        return appError(app, RESULT_INVALID, "Họ tên, phòng ban hoặc chức vụ không hợp lệ.");
    }
    if (!isValidDate(employee->startDate)) {
        return appError(app, RESULT_INVALID, "Ngày vào làm không hợp lệ.");
    }
    if (employee->baseSalary <= 0 || employee->baseSalary > MAX_MONEY || employee->allowance < 0 ||
        employee->allowance > MAX_MONEY) {
        return appError(app, RESULT_INVALID,
                        "Lương phải dương; phụ cấp không âm; mỗi khoản tối đa 1 tỷ đồng.");
    }
    if (employee->hasEndDate != 0 && employee->hasEndDate != 1) {
        return appError(app, RESULT_INVALID, "Cờ ngày nghỉ chỉ nhận 0 hoặc 1.");
    }
    if (employee->status != (employee->hasEndDate ? 2 : 1)) {
        return appError(app, RESULT_INVALID, "Trạng thái nhân viên không khớp ngày nghỉ việc.");
    }
    if (employee->hasEndDate && (!isValidDate(employee->endDate) ||
                                 compareDate(employee->endDate, employee->startDate) < 0)) {
        return appError(app, RESULT_INVALID, "Ngày làm cuối không hợp lệ hoặc trước ngày vào làm.");
    }
    return RESULT_OK;
}

static Result validateExistingRecords(AppData *app, const Employee *employee) {
    for (size_t index = 0; index < app->attendanceCount; index++) {
        const Attendance *record = &app->attendance[index];
        if (strcmp(record->employeeId, employee->id) == 0 &&
            !employeeOnDate(employee, record->date)) {
            return appError(app, RESULT_INVALID,
                            "Thay đổi làm chấm công hiện có nằm ngoài thời gian làm việc.");
        }
    }
    for (size_t index = 0; index < app->inputCount; index++) {
        const PayrollInput *input = &app->inputs[index];
        if (strcmp(input->employeeId, employee->id) == 0 &&
            !employeeInMonth(employee, input->month, input->year)) {
            return appError(app, RESULT_INVALID,
                            "Thay đổi làm thông số lương tháng hiện có không còn hợp lệ.");
        }
    }
    return RESULT_OK;
}

static Result validateLockedMonths(AppData *app, const Employee *before, const Employee *after) {
    for (size_t index = 0; index < app->lockCount; index++) {
        MonthLock lockedMonth = app->locks[index];
        int lastDay = daysInMonth(lockedMonth.month, lockedMonth.year);

        for (int day = 1; day <= lastDay; day++) {
            Date date = {day, lockedMonth.month, lockedMonth.year};
            int workedBefore = before != NULL && employeeOnDate(before, date);
            int worksAfter = employeeOnDate(after, date);

            if (isWorkingDay(date) && workedBefore != worksAfter) {
                return appError(app, RESULT_LOCKED,
                                "Thay đổi ảnh hưởng phạm vi công của tháng %02d/%d đã chốt.",
                                lockedMonth.month, lockedMonth.year);
            }
        }
        int belongedBefore =
            before != NULL && employeeInMonth(before, lockedMonth.month, lockedMonth.year);
        int belongsAfter = employeeInMonth(after, lockedMonth.month, lockedMonth.year);
        if (belongedBefore != belongsAfter) {
            return appError(app, RESULT_LOCKED,
                            "Thay đổi danh sách nhân viên trong tháng đã chốt.");
        }
    }
    return RESULT_OK;
}

static Result appendEmployee(AppData *app, const Employee *employee) {
    EmployeeNode *newNode = malloc(sizeof *newNode);
    if (newNode == NULL) {
        return appError(app, RESULT_NO_MEMORY, "Không đủ bộ nhớ thêm nhân viên.");
    }
    newNode->employee = *employee;
    newNode->next = NULL;

    if (app->employeeHead == NULL) {
        app->employeeHead = newNode;
    } else {
        EmployeeNode *last = app->employeeHead;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = newNode;
    }
    app->employeeCount++;
    return RESULT_OK;
}

Result saveEmployee(AppData *app, const Employee *employee, int editing) {
    Employee *existing = findEmployee(app, employee->id);
    Result result = validateEmployeeFields(app, employee);
    if (result != RESULT_OK) {
        return result;
    }
    if (editing && existing == NULL) {
        return appError(app, RESULT_NOT_FOUND, "Không tìm thấy mã %s.", employee->id);
    }
    if (!editing && existing != NULL) {
        return appError(app, RESULT_DUPLICATE, "Mã %s đã tồn tại.", employee->id);
    }
    if (!editing && app->employeeCount >= MAX_EMPLOYEES) {
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
        /* Sửa dữ liệu ngay trong nút tìm được, giữ nguyên liên kết next. */
        *existing = *employee;
    } else {
        result = appendEmployee(app, employee);
        if (result != RESULT_OK) {
            return result;
        }
    }
    app->dirty = 1;
    return RESULT_OK;
}

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

Result deleteEmployee(AppData *app, const char *employeeId) {
    EmployeeNode *previous = NULL;
    EmployeeNode *current = app->employeeHead;

    while (current != NULL && strcmp(current->employee.id, employeeId) != 0) {
        previous = current;
        current = current->next;
    }
    if (current == NULL) {
        return appError(app, RESULT_NOT_FOUND, "Không tìm thấy nhân viên.");
    }
    Result result = validateEmployeeDeletion(app, employeeId);
    if (result != RESULT_OK) {
        return result;
    }

    if (previous == NULL) {
        app->employeeHead = current->next; /* Xóa nút đầu. */
    } else {
        previous->next = current->next; /* Bỏ qua nút giữa hoặc cuối. */
    }
    free(current);
    app->employeeCount--;
    app->dirty = 1;
    return RESULT_OK;
}

/* Tách danh sách bằng hai con trỏ: slow đi 1 bước, fast đi 2 bước. */
static EmployeeNode *splitEmployeeList(EmployeeNode *head) {
    EmployeeNode *slow = head;
    EmployeeNode *fast = head->next;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    EmployeeNode *rightHalf = slow->next;
    slow->next = NULL;
    return rightHalf;
}

/* Trộn bằng vòng lặp để chỉ phần chia đôi sử dụng đệ quy. */
static EmployeeNode *mergeEmployeeLists(EmployeeNode *left, EmployeeNode *right) {
    EmployeeNode temporaryHead = {0};
    EmployeeNode *tail = &temporaryHead;

    while (left != NULL && right != NULL) {
        if (strcmp(left->employee.id, right->employee.id) <= 0) {
            tail->next = left;
            left = left->next;
        } else {
            tail->next = right;
            right = right->next;
        }
        tail = tail->next;
    }
    tail->next = left != NULL ? left : right;
    return temporaryHead.next;
}

/*
 * Đệ quy có ý nghĩa thực tế: Merge Sort sắp xếp danh sách nhân viên.
 * Điểm dừng: danh sách rỗng hoặc chỉ có một nút.
 * Mỗi lần gọi chia đôi số nút, độ sâu O(log n), thời gian O(n log n).
 * Chỉ nối lại các nút, không cấp phát, sao chép hay làm mất hồ sơ.
 */
static EmployeeNode *mergeSortEmployees(EmployeeNode *head) {
    if (head == NULL || head->next == NULL) {
        return head;
    }
    EmployeeNode *rightHalf = splitEmployeeList(head);
    EmployeeNode *sortedLeft = mergeSortEmployees(head);
    EmployeeNode *sortedRight = mergeSortEmployees(rightHalf);
    return mergeEmployeeLists(sortedLeft, sortedRight);
}

void sortEmployeesById(AppData *app) {
    if (app->employeeCount < 2) {
        return;
    }
    app->employeeHead = mergeSortEmployees(app->employeeHead);
    app->dirty = 1; /* Lưu lại thứ tự mới khi người dùng chọn lưu. */
}

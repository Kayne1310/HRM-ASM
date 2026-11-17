#include "models.h"
#include "employee.h"
#include "storage.h"
#include <stdlib.h>
#include <string.h>

static int checks = 0;
static int failures = 0;
#define CHECK(condition)                                                                           \
    do {                                                                                           \
        checks++;                                                                                  \
        if (!(condition)) {                                                                        \
            failures++;                                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                   \
        }                                                                                          \
    } while (0)

static Employee makeEmployee(const char *id, long long salary) {
    Employee employee = {0};
    strcpy(employee.id, id);
    strcpy(employee.fullName, id);
    strcpy(employee.department, "Nhân sự");
    strcpy(employee.position, "Nhân viên");
    employee.startDate = (Date){1, 1, 2026};
    employee.baseSalary = salary;
    employee.status = 1;
    return employee;
}

static void testEmptyAndSingleNode(void) {
    AppData app;
    appInit(&app, "unused");
    CHECK(app.employeeHead == NULL);
    CHECK(employeeAt(&app, 0) == NULL);
    CHECK(findEmployee(&app, "NV001") == NULL);
    CHECK(findEmployeeIndex(&app, "NV001") == -1);
    sortEmployeesById(&app);
    CHECK(app.employeeHead == NULL && app.dirty == 0);
    CHECK(deleteEmployee(&app, "NV001") == RESULT_NOT_FOUND);

    Employee employee = makeEmployee("NV001", 1000000);
    CHECK(saveEmployee(&app, &employee, 0) == RESULT_OK);
    EmployeeNode *originalHead = app.employeeHead;
    app.dirty = 0;
    sortEmployeesById(&app);
    CHECK(app.employeeHead == originalHead);
    CHECK(app.employeeHead->next == NULL && app.employeeCount == 1);
    CHECK(app.dirty == 0);
    CHECK(deleteEmployee(&app, "NV001") == RESULT_OK);
    CHECK(app.employeeHead == NULL && app.employeeCount == 0);
    appFree(&app);
    CHECK(app.employeeHead == NULL && app.employeeCount == 0);
}

static void testEditSortAndDelete(void) {
    AppData app;
    appInit(&app, "unused");
    const char *ids[] = {"NV003", "NV001", "NV004", "NV002"};
    Employee *originalAddresses[4];
    for (int index = 0; index < 4; index++) {
        Employee employee = makeEmployee(ids[index], 1000000 + index);
        CHECK(saveEmployee(&app, &employee, 0) == RESULT_OK);
        originalAddresses[index] = findEmployee(&app, ids[index]);
    }
    CHECK(app.employeeCount == 4);
    CHECK(employeeAt(&app, 4) == NULL);
    CHECK(findEmployeeIndex(&app, "NV004") == 2);
    Employee changed = *findEmployee(&app, "NV003");
    strcpy(changed.fullName, "Đã sửa");
    CHECK(saveEmployee(&app, &changed, 1) == RESULT_OK);
    CHECK(strcmp(findEmployee(&app, "NV003")->fullName, "Đã sửa") == 0);
    CHECK(saveEmployee(&app, &changed, 0) == RESULT_DUPLICATE);
    CHECK(app.employeeCount == 4);

    sortEmployeesById(&app);
    CHECK(strcmp(employeeAt(&app, 0)->id, "NV001") == 0);
    CHECK(strcmp(employeeAt(&app, 1)->id, "NV002") == 0);
    CHECK(strcmp(employeeAt(&app, 2)->id, "NV003") == 0);
    CHECK(strcmp(employeeAt(&app, 3)->id, "NV004") == 0);
    CHECK(app.employeeCount == 4 && app.dirty == 1);
    for (int index = 0; index < 4; index++) {
        /* Merge Sort chỉ nối lại nút: địa chỉ hồ sơ không được đổi. */
        CHECK(findEmployee(&app, ids[index]) == originalAddresses[index]);
    }
    CHECK(strcmp(findEmployee(&app, "NV003")->fullName, "Đã sửa") == 0);
    sortEmployeesById(&app);
    CHECK(strcmp(employeeAt(&app, 0)->id, "NV001") == 0);
    CHECK(deleteEmployee(&app, "NV002") == RESULT_OK); /* Nút giữa. */
    CHECK(strcmp(app.employeeHead->next->employee.id, "NV003") == 0);
    CHECK(deleteEmployee(&app, "NV001") == RESULT_OK); /* Nút đầu. */
    CHECK(strcmp(app.employeeHead->employee.id, "NV003") == 0);
    CHECK(deleteEmployee(&app, "NV004") == RESULT_OK); /* Nút cuối. */
    CHECK(app.employeeHead->next == NULL);
    CHECK(deleteEmployee(&app, "NV003") == RESULT_OK); /* Nút duy nhất. */
    CHECK(app.employeeHead == NULL && app.employeeCount == 0);
    appFree(&app);
}

static void testLimitAndPersistence(const char *directory) {
    AppData app;
    AppData restored;
    appInit(&app, directory);
    appInit(&restored, directory);
    for (int index = MAX_EMPLOYEES; index >= 1; index--) {
        char id[16];
        snprintf(id, sizeof id, "NV%03d", index);
        Employee employee = makeEmployee(id, 1000000 + index);
        CHECK(saveEmployee(&app, &employee, 0) == RESULT_OK);
    }
    Employee extra = makeEmployee("EXTRA", 1000000);
    CHECK(saveEmployee(&app, &extra, 0) == RESULT_FULL);
    CHECK(app.employeeCount == MAX_EMPLOYEES);
    sortEmployeesById(&app);
    EmployeeNode *current = app.employeeHead;
    for (int index = 1; index <= MAX_EMPLOYEES; index++) {
        char expectedId[16];
        snprintf(expectedId, sizeof expectedId, "NV%03d", index);
        CHECK(current != NULL);
        if (current == NULL) {
            break;
        }
        CHECK(strcmp(current->employee.id, expectedId) == 0);
        CHECK(current->employee.baseSalary == 1000000 + index);
        current = current->next;
    }
    CHECK(current == NULL); /* Không mất nút, không tạo vòng liên kết. */
    CHECK(saveData(&app) == RESULT_OK);
    CHECK(loadData(&restored) == RESULT_OK);
    CHECK(restored.employeeCount == MAX_EMPLOYEES);
    CHECK(strcmp(employeeAt(&restored, 0)->id, "NV001") == 0);
    CHECK(strcmp(employeeAt(&restored, 199)->id, "NV200") == 0);
    CHECK(restored.dirty == 0);
    appFree(&app);
    appFree(&restored);
    CHECK(app.employeeHead == NULL && restored.employeeHead == NULL);
}

int main(int argumentCount, char **arguments) {
    if (argumentCount != 2) {
        fprintf(stderr, "test_linked_list requires a fresh temporary directory\n");
        return 2;
    }
    testEmptyAndSingleNode();
    testEditSortAndDelete();
    testLimitAndPersistence(arguments[1]);
    printf("LIST_CHECKS=%d FAILURES=%d\n", checks, failures);
    return failures != 0;
}

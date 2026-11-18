#include "ui.h"
#include "input.h"
#include "storage.h"

/* Trả về 1 khi có thể thoát, 0 khi người dùng muốn tiếp tục. */
static int requestExit(AppData *app) {
    if (!app->dirty) {
        return 1;
    }
    int menuChoice = askMenuChoice("Có thay đổi chưa lưu.\n"
                                   "1 Lưu rồi thoát | 2 Bỏ thay đổi | 0 Quay lại",
                                   2);
    if (menuChoice == 1) {
        Result result = saveData(app);
        reportResult(app, result);
        return result == RESULT_OK;
    }
    if (menuChoice == 2 && confirm("Y bỏ thay đổi trong bộ nhớ, N quay lại: ")) {
        app->dirty = 0;
        return 1;
    }
    return 0;
}

void runConsole(AppData *app) {
    puts("QUẢN LÝ CHẤM CÔNG VÀ TÍNH LƯƠNG NHÂN VIÊN\n"
         "Giả định học tập: thứ Hai đến thứ Bảy, 8 giờ/ngày, tăng ca 150%, "
         "chưa tính thuế bảo hiểm.");
    while (!inputEnded()) {
        printf("\n------------------------------------------------------------\n"
               "QUẢN LÝ CHẤM CÔNG VÀ TÍNH LƯƠNG\n"
               "Nhân viên: %zu   |   Bản ghi công: %zu   |   Kỳ đã chốt: %zu\n"
               "Trạng thái dữ liệu: %s\n"
               "------------------------------------------------------------\n",
               app->employeeCount, app->attendanceCount, app->lockCount,
               app->dirty ? "Chưa lưu" : "Đã lưu");
        int menuChoice = askMenuChoice(" 1. Nhân viên\n"
                                       " 2. Chấm công\n"
                                       " 3. Bảng lương\n"
                                       " 4. Tìm kiếm\n"
                                       " 5. Thống kê\n"
                                       " 6. Lưu dữ liệu\n"
                                       " 0. Thoát",
                                       6);
        if (inputEnded()) {
            break;
        }
        switch (menuChoice) {
        case 0:
            if (requestExit(app)) {
                return;
            }
            break;
        case 1:
            employeeMenu(app);
            break;
        case 2:
            attendanceMenu(app);
            break;
        case 3:
            payrollMenu(app);
            break;
        case 4:
            searchMenu(app);
            break;
        case 5:
            statisticsMenu(app);
            break;
        case 6:
            reportResult(app, saveData(app));
            break;
        }
    }
    if (app->dirty) {
        puts("Đã hết dữ liệu nhập. Có thay đổi CHƯA LƯU; chương trình không tự ghi tệp.");
    }
}

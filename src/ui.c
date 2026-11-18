#include "ui.h"
#include "input.h"
#include "storage.h"

/* Hỏi người dùng xử lý các thay đổi chưa lưu khi thoát chương trình. Trả về 1 nếu cho phép thoát, 0 nếu quay lại. */
static int requestExit(AppData *app) {
    if (!app->dirty) { /* Không có thay đổi nào chưa lưu -> thoát ngay */
        return 1;
    }
    int menuChoice = askMenuChoice("Có thay đổi chưa lưu.\n"
                                   "1 Lưu rồi thoát | 2 Bỏ thay đổi | 0 Quay lại",
                                   2);
    if (menuChoice == 1) { /* Chọn lưu dữ liệu ra tệp rồi mới thoát */
        Result result = saveData(app);
        reportResult(app, result);
        return result == RESULT_OK; /* Chỉ cho thoát nếu lưu thành công */
    }
    if (menuChoice == 2 && confirm("Y bỏ thay đổi trong bộ nhớ, N quay lại: ")) { /* Xác nhận hủy bỏ thay đổi */
        app->dirty = 0; /* Đưa cờ về 0 vì người dùng chủ động chấp nhận bỏ thay đổi */
        return 1;
    }
    return 0; /* Quay lại menu chính */
}

/* Vòng lặp hiển thị menu chính và điều hướng vào các phân hệ chức năng */
void runConsole(AppData *app) {
    puts("QUẢN LÝ CHẤM CÔNG VÀ TÍNH LƯƠNG NHÂN VIÊN\n"
         "Giả định học tập: thứ Hai đến thứ Bảy, 8 giờ/ngày, tăng ca 150%, "
         "chưa tính thuế bảo hiểm.");
    while (!inputEnded()) { /* Tiếp tục lặp khi luồng nhập stdin chưa kết thúc */
        /* In thông tin tổng quan trạng thái dữ liệu hiện tại trong RAM */
        printf("\n------------------------------------------------------------\n"
               "QUẢN LÝ CHẤM CÔNG VÀ TÍNH LƯƠNG\n"
               "Nhân viên: %zu   |   Bản ghi công: %zu   |   Kỳ đã chốt: %zu\n"
               "Trạng thái dữ liệu: %s\n"
               "------------------------------------------------------------\n",
               app->employeeCount, app->attendanceCount, app->lockCount,
               app->dirty ? "Chưa lưu" : "Đã lưu");
        /* Hiển thị menu và yêu cầu nhập lựa chọn từ 0 đến 6 */
        int menuChoice = askMenuChoice(" 1. Nhân viên\n"
                                       " 2. Chấm công\n"
                                       " 3. Bảng lương\n"
                                       " 4. Tìm kiếm\n"
                                       " 5. Thống kê\n"
                                       " 6. Lưu dữ liệu\n"
                                       " 0. Thoát",
                                       6);
        if (inputEnded()) { /* Kiểm tra trường hợp nhấn Ctrl+Z/hết input ngay lúc chọn menu */
            break;
        }
        switch (menuChoice) { /* Rẽ nhánh sang từng chức năng theo phím chọn */
        case 0:
            if (requestExit(app)) { /* Kiểm tra lưu dữ liệu trước khi kết thúc */
                return;
            }
            break;
        case 1:
            employeeMenu(app); /* Chuyển sang menu quản lý nhân viên */
            break;
        case 2:
            attendanceMenu(app); /* Chuyển sang menu chấm công */
            break;
        case 3:
            payrollMenu(app); /* Chuyển sang menu tính & chốt lương */
            break;
        case 4:
            searchMenu(app); /* Chuyển sang menu tìm kiếm */
            break;
        case 5:
            statisticsMenu(app); /* Chuyển sang menu thống kê */
            break;
        case 6:
            reportResult(app, saveData(app)); /* Lưu toàn bộ dữ liệu nguồn ra file ngay lập tức */
            break;
        }
    }
    if (app->dirty) { /* Nếu hết luồng nhập (EOF) mà dữ liệu vẫn chưa lưu, cảnh báo không tự ghi đè file */
        puts("Đã hết dữ liệu nhập. Có thay đổi CHƯA LƯU; chương trình không tự ghi tệp.");
    }
}

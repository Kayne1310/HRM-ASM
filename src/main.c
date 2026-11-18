#include "models.h"
#include "storage.h"
#include "payroll.h"
#include "input.h"
#include "ui.h"
#include <stdlib.h> /* Dùng cho calloc, free */
#include <string.h> /* Dùng cho strcmp, strlen */
#ifdef _WIN32
#include <windows.h> /* Dùng SetConsoleOutputCP / SetConsoleCP để hiển thị tiếng Việt UTF-8 trên Windows */
#endif

/* Cấu trúc lưu các tùy chọn đọc được từ tham số dòng lệnh (CLI) */
typedef struct {
    const char *dataDirectory; /* Thư mục chứa dữ liệu (mặc định là "data") */
    const char *command;       /* Lệnh muốn thực thi (--check, --payroll, --finalize, --export hoặc NULL nếu chạy menu) */
    const char *exportFile;    /* Đường dẫn tệp CSV đích nếu dùng lệnh --export */
    int month;                 /* Tháng truyền từ tham số */
    int year;                  /* Năm truyền từ tham số */
    int showHelp;              /* Cờ bằng 1 nếu người dùng truyền --help */
} CommandOptions;

/* In hướng dẫn các câu lệnh dòng lệnh (CLI) hỗ trợ */
static void printUsage(void) {
    puts("Cách dùng:\n"
         "  chamcong [--data THU_MUC]\n"
         "  chamcong [--data THU_MUC] --check\n"
         "  chamcong [--data THU_MUC] --payroll THANG NAM\n"
         "  chamcong [--data THU_MUC] --finalize THANG NAM\n"
         "  chamcong [--data THU_MUC] --export THANG NAM TEP_CSV\n"
         "Không ghi đè CSV từ dòng lệnh. Dùng menu để xác nhận ghi đè.");
}

/* Phân tích các tham số truyền vào hàm main(argc, argv) và kiểm tra tính hợp lệ */
static int readCommandOptions(int argumentCount, char **arguments, CommandOptions *options) {
    *options = (CommandOptions){.dataDirectory = "data"}; /* Khởi tạo mặc định thư mục dữ liệu là "data" */
    for (int index = 1; index < argumentCount; index++) { /* Duyệt từ tham số thứ 1 (bỏ qua tên file exe ở vị trí 0) */
        const char *argument = arguments[index];
        if (strcmp(argument, "--help") == 0) { /* Yêu cầu xem hướng dẫn */
            options->showHelp = 1;
            return 1;
        }
        if (strcmp(argument, "--data") == 0 && index + 1 < argumentCount) { /* Tham số chỉ định thư mục dữ liệu riêng */
            options->dataDirectory = arguments[++index]; /* Lấy tham số ngay phía sau --data */
            continue;
        }
        if (options->command != NULL) { /* Không cho phép truyền nhiều lệnh cùng lúc */
            return 0;
        }
        options->command = argument;
        if (strcmp(argument, "--check") == 0) { /* Lệnh --check không cần tham số tháng/năm phía sau */
            continue;
        }
        int supportedCommand = strcmp(argument, "--payroll") == 0 ||
                               strcmp(argument, "--finalize") == 0 ||
                               strcmp(argument, "--export") == 0;
        if (!supportedCommand || index + 2 >= argumentCount) { /* Các lệnh này bắt buộc phải có đủ THANG và NAM phía sau */
            return 0;
        }
        long long parsedNumber;
        if (!parseInteger(arguments[++index], 1, 12, &parsedNumber)) { /* Đọc và kiểm tra THANG trong [1, 12] */
            return 0;
        }
        options->month = (int)parsedNumber;
        if (!parseInteger(arguments[++index], 2000, 2100, &parsedNumber)) { /* Đọc và kiểm tra NAM trong [2000, 2100] */
            return 0;
        }
        options->year = (int)parsedNumber;
        if (strcmp(argument, "--export") == 0) { /* Lệnh --export cần thêm tên tệp CSV ở tham số kế tiếp */
            if (index + 1 >= argumentCount) {
                return 0;
            }
            options->exportFile = arguments[++index];
        }
    }
    /* Đảm bảo đường dẫn thư mục không quá dài để còn ghép tên tệp vào sau */
    return strlen(options->dataDirectory) <= PATH_CAP - 100;
}

/* Điều phối thực thi chương trình theo chế độ Console tương tác hoặc lệnh CLI */
static int executeCommand(AppData *app, const CommandOptions *options) {
    Result result = RESULT_OK;
    if (options->command == NULL) { /* Không truyền lệnh CLI -> Chạy giao diện menu console tương tác */
        runConsole(app);
        return inputEnded() && app->dirty ? 2 : 0; /* Nếu hết luồng nhập mà còn dữ liệu chưa lưu -> mã thoát 2 */
    }
    if (strcmp(options->command, "--check") == 0) { /* Chế độ kiểm tra tính hợp lệ dữ liệu */
        printf("Dữ liệu hợp lệ: %zu nhân viên, %zu công, %zu thông số tháng, %zu kỳ đã chốt.\n",
               app->employeeCount, app->attendanceCount, app->inputCount, app->lockCount);
    } else if (strcmp(options->command, "--export") == 0) { /* Chế độ xuất CSV nhanh (không ghi đè file có sẵn) */
        result = exportPayroll(app, options->month, options->year, options->exportFile, 0);
    } else if (strcmp(options->command, "--finalize") == 0) { /* Chế độ chốt kỳ lương từ CLI */
        result = finalizePayroll(app, options->month, options->year);
        if (result == RESULT_OK) {
            printf("Đã chốt %02d/%d.\n", options->month, options->year);
        }
    } else { /* Chế độ --payroll: Tính và in bảng lương tháng ra màn hình */
        Payroll rows[MAX_EMPLOYEES];
        size_t rowCount;
        result = buildPayroll(app, options->month, options->year, rows, &rowCount);
        if (result == RESULT_OK) {
            printf("Kỳ %02d/%d\n", options->month, options->year);
            printPayroll(rows, rowCount, isLocked(app, options->month, options->year), 0);
        }
    }
    if (result != RESULT_OK) { /* Nếu có lỗi nghiệp vụ/tệp thì in ra stderr và trả về mã thoát 1 */
        fprintf(stderr, "Lỗi: %s\n", app->error);
        return 1;
    }
    return 0; /* Thành công -> mã thoát 0 */
}

/* Hàm main làm nhiệm vụ khởi tạo, nạp dữ liệu, điều phối chạy và giải phóng bộ nhớ trước khi thoát. */
int main(int argumentCount, char **arguments) {
#ifdef _WIN32
    /* Thiết lập bảng mã UTF-8 (65001) cho cả đầu ra và đầu vào của Console Windows để không lỗi font tiếng Việt */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    CommandOptions options;
    if (!readCommandOptions(argumentCount, arguments, &options)) { /* Sai cú pháp tham số dòng lệnh */
        printUsage();
        return 2; /* Mã thoát 2: sai tham số */
    }
    if (options.showHelp) { /* Hiển thị trợ giúp rồi thoát với mã 0 */
        printUsage();
        return 0;
    }
    AppData *app = calloc(1, sizeof *app); /* Cấp phát AppData trên Heap thay vì Stack do cấu trúc lớn */
    if (app == NULL) {
        fputs("Không đủ bộ nhớ.\n", stderr);
        return 1;
    }
    appInit(app, options.dataDirectory); /* Khởi tạo các giá trị ban đầu cho app */
    int exitCode;
    if (loadData(app) != RESULT_OK) { /* Nạp toàn bộ tệp dữ liệu; nếu lỗi thì dừng để tránh ghi đè làm hỏng dữ liệu */
        fprintf(stderr,
                "Không nạp được dữ liệu: %s\n"
                "Không ghi đè. Xem hướng dẫn phục hồi trong README.\n",
                app->error);
        exitCode = 1;
    } else {
        exitCode = executeCommand(app, &options); /* Dữ liệu sạch -> thực thi menu hoặc lệnh CLI */
    }
    appFree(app); /* Giải phóng toàn bộ danh sách liên kết và các mảng động bên trong app */
    free(app);    /* Giải phóng chính khối nhớ của struct AppData */
    return exitCode;
}

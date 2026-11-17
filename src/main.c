#include "models.h"
#include "storage.h"
#include "payroll.h"
#include "input.h"
#include "ui.h"
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

typedef struct {
    const char *dataDirectory;
    const char *command;
    const char *exportFile;
    int month;
    int year;
    int showHelp;
} CommandOptions;

static void printUsage(void) {
    puts("Cách dùng:\n"
         "  chamcong [--data THU_MUC]\n"
         "  chamcong [--data THU_MUC] --check\n"
         "  chamcong [--data THU_MUC] --payroll THANG NAM\n"
         "  chamcong [--data THU_MUC] --finalize THANG NAM\n"
         "  chamcong [--data THU_MUC] --export THANG NAM TEP_CSV\n"
         "Không ghi đè CSV từ dòng lệnh. Dùng menu để xác nhận ghi đè.");
}

static int readCommandOptions(int argumentCount, char **arguments, CommandOptions *options) {
    *options = (CommandOptions){.dataDirectory = "data"};
    for (int index = 1; index < argumentCount; index++) {
        const char *argument = arguments[index];
        if (strcmp(argument, "--help") == 0) {
            options->showHelp = 1;
            return 1;
        }
        if (strcmp(argument, "--data") == 0 && index + 1 < argumentCount) {
            options->dataDirectory = arguments[++index];
            continue;
        }
        if (options->command != NULL) {
            return 0;
        }
        options->command = argument;
        if (strcmp(argument, "--check") == 0) {
            continue;
        }
        int supportedCommand = strcmp(argument, "--payroll") == 0 ||
                               strcmp(argument, "--finalize") == 0 ||
                               strcmp(argument, "--export") == 0;
        if (!supportedCommand || index + 2 >= argumentCount) {
            return 0;
        }
        long long parsedNumber;
        if (!parseInteger(arguments[++index], 1, 12, &parsedNumber)) {
            return 0;
        }
        options->month = (int)parsedNumber;
        if (!parseInteger(arguments[++index], 2000, 2100, &parsedNumber)) {
            return 0;
        }
        options->year = (int)parsedNumber;
        if (strcmp(argument, "--export") == 0) {
            if (index + 1 >= argumentCount) {
                return 0;
            }
            options->exportFile = arguments[++index];
        }
    }
    return strlen(options->dataDirectory) <= PATH_CAP - 100;
}

static int executeCommand(AppData *app, const CommandOptions *options) {
    Result result = RESULT_OK;
    if (options->command == NULL) {
        runConsole(app);
        return inputEnded() && app->dirty ? 2 : 0;
    }
    if (strcmp(options->command, "--check") == 0) {
        printf("Dữ liệu hợp lệ: %zu nhân viên, %zu công, %zu thông số tháng, %zu kỳ đã chốt.\n",
               app->employeeCount, app->attendanceCount, app->inputCount, app->lockCount);
    } else if (strcmp(options->command, "--export") == 0) {
        result = exportPayroll(app, options->month, options->year, options->exportFile, 0);
    } else if (strcmp(options->command, "--finalize") == 0) {
        result = finalizePayroll(app, options->month, options->year);
        if (result == RESULT_OK) {
            printf("Đã chốt %02d/%d.\n", options->month, options->year);
        }
    } else {
        Payroll rows[MAX_EMPLOYEES];
        size_t rowCount;
        result = buildPayroll(app, options->month, options->year, rows, &rowCount);
        if (result == RESULT_OK) {
            printf("Kỳ %02d/%d\n", options->month, options->year);
            printPayroll(rows, rowCount, isLocked(app, options->month, options->year), 0);
        }
    }
    if (result != RESULT_OK) {
        fprintf(stderr, "Lỗi: %s\n", app->error);
        return 1;
    }
    return 0;
}

/* main chỉ khởi tạo, nạp dữ liệu, điều phối và giải phóng bộ nhớ. */
int main(int argumentCount, char **arguments) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    CommandOptions options;
    if (!readCommandOptions(argumentCount, arguments, &options)) {
        printUsage();
        return 2;
    }
    if (options.showHelp) {
        printUsage();
        return 0;
    }
    AppData *app = calloc(1, sizeof *app);
    if (app == NULL) {
        fputs("Không đủ bộ nhớ.\n", stderr);
        return 1;
    }
    appInit(app, options.dataDirectory);
    int exitCode;
    if (loadData(app) != RESULT_OK) {
        fprintf(stderr,
                "Không nạp được dữ liệu: %s\n"
                "Không ghi đè. Xem hướng dẫn phục hồi trong README.\n",
                app->error);
        exitCode = 1;
    } else {
        exitCode = executeCommand(app, &options);
    }
    appFree(app);
    free(app);
    return exitCode;
}

# QUẢN LÝ CHẤM CÔNG VÀ TÍNH LƯƠNG NHÂN VIÊN

## Tài liệu kỹ thuật và kế hoạch phát triển

Mã tài liệu KT 01   |   Phiên bản 1.2   |   Ngày 05/11/2026

Tài liệu hướng dẫn triển khai hệ thống bằng C, quản lý các thành phần phát triển, kiểm thử và bàn giao. Kế hoạch bảo đảm cả SV1, SV2 và SV3 cùng tham gia từng giai đoạn với số giờ dự kiến bằng nhau.

### Thông tin đề tài

| Thông tin | Nội dung |
| --- | --- |
| Môn học | Kỹ thuật lập trình |
| Ngôn ngữ và giao diện | Ngôn ngữ C và giao diện console |
| Trường và lớp | [Điền tên trường]  \|  [Điền lớp] |
| Giảng viên hướng dẫn | [Điền họ tên giảng viên] |
| Nhóm thực hiện | [Điền tên hoặc số nhóm] |

### Nhóm sinh viên thực hiện

| Mã | Họ và tên | Mã sinh viên | Lớp và liên hệ |
| --- | --- | --- | --- |
| SV1 | [Họ tên sinh viên 1] | [MSSV 1] | [Lớp / email] |
| SV2 | [Họ tên sinh viên 2] | [MSSV 2] | [Lớp / email] |
| SV3 | [Họ tên sinh viên 3] | [MSSV 3] | [Lớp / email] |

### Lịch sử chỉnh sửa tài liệu

| Phiên bản | Ngày | Nội dung thay đổi | Người thực hiện |
| --- | --- | --- | --- |
| 1.0 | 05/11/2026 | Khởi tạo bộ tài liệu theo phạm vi đề tài | Nhóm SV1 SV2 SV3 |
| 1.1 | 05/11/2026 | Chuyển sang Markdown và liên kết bản triển khai thực tế tại tài liệu 04 và 05 | Công cụ hỗ trợ Codex |
| 1.2 | 05/11/2026 | Cập nhật danh sách liên kết, đệ quy và mã dễ đọc theo tiêu chí đánh giá; xem tài liệu 06 | Công cụ hỗ trợ Codex |
| [Điền] | [Điền] | [Ghi nội dung chỉnh sửa] | [Điền] |

## 1 Nền tảng và tổ chức mã nguồn

Chuẩn đề xuất C11, thư viện chuẩn stdio.h, stdlib.h, string.h, ctype.h, errno.h, limits.h. Không dùng cơ sở dữ liệu hoặc thư viện giao diện. Chọn một môi trường GCC hoặc Clang để cả nhóm dùng cùng cấu hình; ghi phiên bản thực tế trong biên bản bàn giao.

```text
project/
  include/ models.h input.h date_utils.h
           employee.h attendance.h payroll.h
           storage.h search.h statistics.h
  src/     main.c input.c date_utils.c employee.c
           attendance.c payroll.c storage.c
           search.c statistics.c
  tests/   test_date.c test_payroll.c test_storage.c
  data/    employees.txt attendance.txt
           payroll_inputs.txt payroll_YYYY_MM.txt
  exports/
  docs/    URD PT_TK KT test_report change_log
  README.md
```

### 1.1 Biên dịch và chạy

```text
gcc -std=c11 -Wall -Wextra -Wpedantic -Iinclude
    src/*.c -o chamcong
```

Lệnh trên trình bày thành hai dòng để dễ đọc; khi chạy nhập thành một dòng. Biên dịch bản kiểm thử riêng với các tệp nghiệp vụ cần thiết và tránh ghép hai hàm main. Chạy chương trình từ thư mục project để đường dẫn data và exports được xác định thống nhất.

### 1.2 Quy ước kỹ thuật

Tên kiểu PascalCase; hàm camelCase; hằng UPPER_SNAKE_CASE. Mọi hàm có mô tả đầu vào, đầu ra và mã lỗi. Dùng const cho dữ liệu chỉ đọc, kiểm tra mọi thao tác tệp và cấp phát. Không để hàm tính lương trực tiếp nhập bàn phím hoặc ghi tệp.

## 2 Danh mục thành phần phát triển

| Thành phần | Trách nhiệm và đầu ra | Phụ thuộc |
| --- | --- | --- |
| models | Date, Employee, Attendance, PayrollInput, Payroll, AppData; hằng sức chứa và enum. | Không |
| input | readLine, parseInt, parseMoney, parseHalfHours; xử lý quá dài và EOF. | models |
| date_utils | Ngày nhuận, số ngày tháng, so sánh ngày, thứ, ngày làm việc. | models |
| employee | Tìm mã, thêm sửa, nghỉ việc, xóa có kiểm tra liên kết. | input date_utils |
| attendance | Khóa mã ngày, thêm sửa xóa, tổng hợp và kiểm tra thiếu. | employee date_utils |
| payroll | Thông số tháng, tổng hợp, tính khoản, kiểm tra hợp lệ, chốt. | attendance date_utils storage |
| storage | Đọc kiểm tra, tách trường giữ rỗng, lưu tạm, sao lưu, xuất CSV. | models |
| search statistics | Bộ lọc AND, tổng hợp nhóm, quỹ lương và sắp xếp. | employee payroll |
| main | Khởi tạo, menu, quản lý cờ thay đổi, điều phối và giải phóng. | Các mô đun |

### 2.1 Nguyên tắc tích hợp

Thống nhất models.h và giao diện hàm trước khi tách việc. Mô đun nghiệp vụ trả mã kết quả; lớp giao diện chuyển mã thành thông báo. Chỉ lớp storage tạo và thay tệp chính thức; chỉ payroll quyết định đủ điều kiện chốt. Tránh mỗi màn hình tự cài một công thức tính lương khác nhau.

## 3 Cấu trúc dữ liệu và hợp đồng hàm

```text
typedef struct { int day, month, year; } Date;

typedef enum {
    RESULT_OK, RESULT_INVALID, RESULT_NOT_FOUND,
    RESULT_DUPLICATE, RESULT_FULL, RESULT_LOCKED,
    RESULT_IO_ERROR, RESULT_NO_MEMORY
} Result;

Result calculatePayroll(
    const PayrollInput *input, int standardDays,
    int paidHalfHours, int overtimeHalfHours,
    Payroll *result);

Result validateAttendance(
    const Attendance *record,
    const Employee *employee);

int findEmployeeIndex(const Employee *items,
                      int count, const char *id);
```

findEmployeeIndex trả -1 nếu không tồn tại. Hàm trả Result không được báo thành công khi đầu ra chưa hoàn chỉnh. Hàm calculatePayroll chỉ tính một nhân viên; việc tổng hợp nguồn và kiểm tra thiếu thuộc các hàm khác.

### 3.1 Bộ nhớ và sức chứa

AppData giữ con trỏ employeeHead của danh sách liên kết nhân viên, các con trỏ mảng công/lương, số phần tử, sức chứa và cờ đã thay đổi theo nhóm tệp. Giới hạn 200 nhân viên và 150.000 công. Mảng thông số tháng cấp phát động và mở rộng theo nhu cầu, kiểm tra tràn kích thước và kết quả realloc trước khi thay con trỏ cũ. Bảng lương đang xử lý tối đa 200 dòng cho một kỳ; lịch sử chốt đọc theo tệp tháng khi cần. Không ghi vượt sức chứa đã cấp.

Cấp phát mảng công bằng malloc hoặc calloc, kiểm tra NULL, giải phóng khi kết thúc. Duyệt mảng bằng chỉ số trong [0, count). Mọi sao chép chuỗi bảo đảm ký tự kết thúc. Không ghi trực tiếp struct nhị phân ra tệp vì phụ thuộc padding và nền tảng.

## 4 Thuật toán ngày công và tính tiền

### 4.1 Ngày tháng và kiểm tra thiếu

Năm nhuận nếu chia hết cho 400 hoặc chia hết cho 4 và không chia hết cho 100. daysInMonth dùng bảng ngày và hiệu chỉnh tháng 2. Xác định thứ bằng một thuật toán lịch đã thống nhất, kiểm thử bằng ngày biết trước. workingDaysInMonth duyệt mọi ngày, loại Chủ nhật.

Để kiểm tra một kỳ, tạo bảng tạm theo nhân viên và ngày, tối đa 200 × 31 ô. Duyệt mảng công một lần để điền ô; phát hiện trùng ngay. Sau đó duyệt các ngày làm việc trong khoảng nhân viên có hiệu lực để tìm ô rỗng. Độ phức tạp bản triển khai dùng tìm mã tuyến tính là O(A × E + E² + E × 31), với A là tổng công và E là số nhân viên.

### 4.2 Tính bằng số nguyên

```text
Q = giờ thường quy đổi nửa giờ
    + 16 × số ngày nghỉ có lương
T = tổng đơn vị nửa giờ tăng ca

regularPay   = roundDiv(L × Q, D × 16)
allowancePay = roundDiv(A × Q, D × 16)
overtimePay  = roundDiv(L × T × 3, D × 32)
netPay = regularPay + allowancePay
         + overtimePay + bonus - deduction
```

roundDiv nhận tử không âm, mẫu dương. Tính thương và dư, cộng 1 khi dư từ nửa mẫu trở lên. Giới hạn mỗi đầu vào tiền không quá 1 tỷ, Q và T phải khớp trần theo số ngày; kiểm tra phép nhân trước khi thực hiện bằng giới hạn LLONG_MAX. Kiểm tra cả tổng các khoản và tổng quỹ lương.

### 4.3 Tính lại và khóa kỳ

Mọi thay đổi công hoặc thông số tháng làm bảng tạm mất hiệu lực. Trước chốt, tổng hợp lại và kiểm tra thiếu, số âm, khóa kỳ; sau đó ghi snapshot. Tên và phòng ban trong bản chốt không được lấy động từ hồ sơ khi tra cứu.

## 5 Thiết kế tệp và quy trình lưu

| Tệp | Định dạng dòng dữ liệu |
| --- | --- |
| employees.txt | id\|name\|department\|position\|startDate\|endDate\|baseSalary\|allowance\|status |
| attendance.txt | employeeId\|date\|type\|regularHalfHours\|overtimeHalfHours\|note |
| payroll_inputs.txt | employeeId\|month\|year\|baseSalary\|allowance\|bonus\|deduction |
| payroll_YYYY_MM.txt | employeeId\|name\|department\|month\|year\|standardDays\|paidHalfHours\|overtimeHalfHours\|baseSalary\|allowance\|bonus\|deduction\|regularPay\|allowancePay\|overtimePay\|netPay |

Tệp UTF-8, dòng đầu loại và phiên bản: EMPLOYEES|1, ATTENDANCE|1, PAYROLL_INPUTS|1. Tệp chính thức dùng PAYROLL|1|year|month|recordCount. Ngày yyyy-mm-dd; ngày nghỉ rỗng nếu chưa nghỉ. Một tệp bảng lương chính thức hợp lệ xác định tháng đã chốt; tên tệp, kỳ trong header và từng dòng phải khớp.

### 5.1 Nạp dữ liệu

Đọc vào bộ nhớ tạm, kiểm tra đủ trường, chuyển số, miền, khóa trùng, FK và tổng dòng. Giữ được trường rỗng bằng hàm tách tự viết, không dùng strtok cho dòng có ||. Chỉ thay dữ liệu đang dùng sau khi toàn bộ tập nạp hợp lệ. Lần đầu không có tệp được phép rỗng; tệp đang có nhưng hỏng phải dừng cập nhật hoặc chọn khôi phục từ bản sao lưu hợp lệ.

### 5.2 Ghi và phục hồi

Ghi tên.tmp cùng thư mục, kiểm tra fprintf và fclose. Nếu có bản chính, tạo bản .bak rồi thay bằng tệp mới. Nếu thao tác thay lỗi, giữ hoặc khôi phục bản cũ và báo lỗi. Trên Windows không giả định rename tự ghi đè tệp tồn tại. Sau sự cố, chỉ khôi phục theo lựa chọn có kiểm tra tính hợp lệ; không tự ưu tiên tệp có thời gian mới hơn.

Nhiều tệp chưa có giao dịch đồng bộ. Khi khởi động phải kiểm tra tham chiếu và phát hiện tệp tạm còn lại. Không tuyên bố bảo đảm mất điện an toàn tuyệt đối. Tệp xuất CSV là bản báo cáo, không dùng làm nguồn khôi phục.

## 6 Nhập liệu và quản lý lỗi

### 6.1 Dòng nhập

Chỉ dùng fgets cho đầu vào console. readLine trả trạng thái thành công, quá dài hoặc EOF. Nếu quá dài, đọc bỏ phần dư đến newline hoặc EOF rồi yêu cầu nhập lại. Không dùng fflush(stdin). Có thể fflush(stdout) để hiển thị lời nhắc.

Chuyển số bằng strtol hoặc strtoll với errno, endptr, kiểm tra có chữ số và phần dư chỉ có khoảng trắng. Tiền nhập chữ số nguyên không dấu phân cách. Giờ nhập 0, 0.5, 1, 1.5 và tương tự; tách phần nguyên phần thập phân để đổi trực tiếp sang nửa giờ, không dùng float.

### 6.2 Chuỗi và CSV

Chuỗi UTF-8 giới hạn theo byte; không nhận |, CR, LF và ký tự điều khiển. Tránh cắt ký tự UTF-8 giữa byte khi trình bày cột. Khi xuất CSV, bọc trường chứa dấu phẩy hoặc dấu nháy bằng dấu nháy kép, nhân đôi dấu nháy bên trong. Đầu tệp ghi kỳ và trạng thái tạm hoặc đã chốt theo cấu trúc báo cáo đã thống nhất.

| Tình huống | Xử lý bắt buộc |
| --- | --- |
| Mã trùng hoặc không tìm thấy | Trả mã lỗi nghiệp vụ; không cập nhật nửa chừng. |
| Hết bộ nhớ hoặc đầy mảng | Giữ dữ liệu cũ; báo giới hạn hoặc lỗi cấp phát. |
| Tệp sai hoặc tham chiếu thiếu | Nêu tên tệp và số dòng; chặn ghi đè dữ liệu lỗi. |
| Lỗi lưu một nhóm tệp | Giữ cờ thay đổi của nhóm lỗi; báo rõ nhóm đã lưu và chưa lưu. |
| EOF khi nhập | Dừng vòng lặp nhập; đi qua luồng thoát có kiểm soát. |

Thông báo không in dữ liệu nhạy cảm không cần thiết. Phiên bản học tập lưu tệp văn bản, cần quản lý thư mục và bản sao lưu trên máy dùng để thực hành.

## 7 Kế hoạch giai đoạn và cân bằng khối lượng

Kế hoạch dự kiến 7 tuần, mỗi giai đoạn một tuần. Giờ công dưới đây là ước lượng, chưa phải công việc đã thực hiện. Mỗi giai đoạn cả ba thành viên có cùng số giờ; chia theo độ phức tạp và sản phẩm, không chỉ theo số màn hình.

| Giai đoạn | Nội dung | SV1 giờ | SV2 giờ | SV3 giờ | Tổng giờ |
| --- | --- | --- | --- | --- | --- |
| GĐ1 | Phân tích và chốt yêu cầu | 8 | 8 | 8 | 24 |
| GĐ2 | Thiết kế mô hình và màn hình | 10 | 10 | 10 | 30 |
| GĐ3 | Nền tảng nhập ngày và lưu tệp | 12 | 12 | 12 | 36 |
| GĐ4 | Nhân viên và chấm công | 14 | 14 | 14 | 42 |
| GĐ5 | Lương tìm kiếm và thống kê | 14 | 14 | 14 | 42 |
| GĐ6 | Tích hợp và kiểm thử hệ thống | 12 | 12 | 12 | 36 |
| GĐ7 | Hoàn thiện và bàn giao | 8 | 8 | 8 | 24 |
| Tổng | Toàn bộ dự án | 78 | 78 | 78 | 234 |

### 7.1 Cơ cấu giờ trong mỗi giai đoạn

Mỗi thành viên dành khoảng 60 phần trăm thời gian cho sản phẩm chính, 25 phần trăm cho kiểm thử và rà chéo, 15 phần trăm cho tích hợp và tài liệu. Cập nhật giờ thực tế cuối mỗi giai đoạn; nếu một người vượt quá 20 phần trăm so với hai người còn lại, chia lại công việc chưa bắt đầu ở giai đoạn kế tiếp.

### 7.2 Trách nhiệm phối hợp

Đầu mối giai đoạn luân phiên: GĐ1 SV1, GĐ2 SV2, GĐ3 SV3, GĐ4 SV1, GĐ5 SV2, GĐ6 SV3; GĐ7 cả nhóm thống nhất bàn giao. Đầu mối theo dõi tiến độ và tích hợp, không nhận toàn bộ việc của giai đoạn. Không chuyển toàn bộ kiểm thử hoặc tài liệu cho riêng một người.

## 8 Phân công chi tiết từng giai đoạn

## 8.1 GĐ1 Phân tích và chốt yêu cầu

Thời lượng dự kiến: tuần 1. Mỗi thành viên 8 giờ; tổng 24 giờ.

| Thành viên | Đầu việc và kiểm tra chéo |
| --- | --- |
| SV1 | Viết hồ sơ nhân viên, UC01; rà quy tắc công của SV2. |
| SV2 | Viết chấm công, UC02; rà công thức của SV3. |
| SV3 | Viết lương, UC03 và nghiệm thu; rà hồ sơ của SV1. |

### Sản phẩm và điều kiện kết thúc

URD thống nhất, mọi FR có AC.

Đầu vào là phạm vi đề tài và tài liệu nghiệp vụ đã trao đổi. Đầu ra được cả ba người rà soát, ghi nhận câu hỏi và sửa quy tắc mâu thuẫn trước khi thiết kế.

### Bằng chứng tiến độ

Ghi đường dẫn sản phẩm, mã thay đổi, ca kiểm thử đã chạy, người rà soát và giờ thực tế trong sổ theo dõi. Mỗi thành viên phải có ít nhất một sản phẩm chính và một nội dung rà chéo ở giai đoạn này. Công việc chưa xong được chuyển tiếp có lý do, không đánh dấu hoàn tất chỉ vì hết tuần.

## 8.2 GĐ2 Thiết kế mô hình và màn hình

Thời lượng dự kiến: tuần 2. Mỗi thành viên 10 giờ; tổng 30 giờ.

| Thành viên | Đầu việc và kiểm tra chéo |
| --- | --- |
| SV1 | Mô hình dữ liệu; SCR01 đến SCR04; rà màn hình công. |
| SV2 | Luồng công; SCR05 đến SCR08; rà màn hình lương. |
| SV3 | Kiến trúc và lương; SCR09 đến SCR16; rà mô hình dữ liệu. |

### Sản phẩm và điều kiện kết thúc

PT TK, models.h và hợp đồng hàm thống nhất.

Phụ thuộc: hoàn tất các đầu ra của GĐ1. Mọi lỗi chặn tích hợp phải được ghi người xử lý và kiểm thử lại trước khi chuyển bước.

### Bằng chứng tiến độ

Ghi đường dẫn sản phẩm, mã thay đổi, ca kiểm thử đã chạy, người rà soát và giờ thực tế trong sổ theo dõi. Mỗi thành viên phải có ít nhất một sản phẩm chính và một nội dung rà chéo ở giai đoạn này. Công việc chưa xong được chuyển tiếp có lý do, không đánh dấu hoàn tất chỉ vì hết tuần.

## 8.3 GĐ3 Nền tảng nhập ngày và lưu tệp

Thời lượng dự kiến: tuần 3. Mỗi thành viên 12 giờ; tổng 36 giờ.

| Thành viên | Đầu việc và kiểm tra chéo |
| --- | --- |
| SV1 | Phát triển input, khung menu, kiểm thử input; rà date_utils. |
| SV2 | Phát triển date_utils, bộ kiểm thử lịch; rà storage. |
| SV3 | Phát triển storage chung, parser và kiểm thử tệp; rà input. |

### Sản phẩm và điều kiện kết thúc

Khung chạy, nạp lưu mẫu, kiểm tra nhập và lịch đạt.

Phụ thuộc: hoàn tất các đầu ra của GĐ2. Mọi lỗi chặn tích hợp phải được ghi người xử lý và kiểm thử lại trước khi chuyển bước.

### Bằng chứng tiến độ

Ghi đường dẫn sản phẩm, mã thay đổi, ca kiểm thử đã chạy, người rà soát và giờ thực tế trong sổ theo dõi. Mỗi thành viên phải có ít nhất một sản phẩm chính và một nội dung rà chéo ở giai đoạn này. Công việc chưa xong được chuyển tiếp có lý do, không đánh dấu hoàn tất chỉ vì hết tuần.

## 8.4 GĐ4 Nhân viên và chấm công

Thời lượng dự kiến: tuần 4. Mỗi thành viên 14 giờ; tổng 42 giờ.

| Thành viên | Đầu việc và kiểm tra chéo |
| --- | --- |
| SV1 | employee thêm sửa nghỉ việc; kiểm thử quy tắc hồ sơ. |
| SV2 | attendance thêm sửa xóa xem; kiểm thử giờ và trùng. |
| SV3 | Kiểm tra thiếu, tích hợp FK, lưu nhân viên và công; kiểm thử tích hợp. |

### Sản phẩm và điều kiện kết thúc

FR01 đến FR04 hoạt động và lưu mở lại đúng.

Phụ thuộc: hoàn tất các đầu ra của GĐ3. Mọi lỗi chặn tích hợp phải được ghi người xử lý và kiểm thử lại trước khi chuyển bước.

### Bằng chứng tiến độ

Ghi đường dẫn sản phẩm, mã thay đổi, ca kiểm thử đã chạy, người rà soát và giờ thực tế trong sổ theo dõi. Mỗi thành viên phải có ít nhất một sản phẩm chính và một nội dung rà chéo ở giai đoạn này. Công việc chưa xong được chuyển tiếp có lý do, không đánh dấu hoàn tất chỉ vì hết tuần.

## 8.5 GĐ5 Lương tìm kiếm và thống kê

Thời lượng dự kiến: tuần 5. Mỗi thành viên 14 giờ; tổng 42 giờ.

| Thành viên | Đầu việc và kiểm tra chéo |
| --- | --- |
| SV1 | Thông số tháng, search, đầu vào lương; kiểm thử thay đổi nguồn. |
| SV2 | Tổng hợp Q T, statistics công quỹ lương; kiểm thử tổng hợp. |
| SV3 | Công thức, snapshot, chốt và phiếu lương; kiểm thử tiền và khóa. |

### Sản phẩm và điều kiện kết thúc

FR05 đến FR09, ví dụ lương đúng và chốt an toàn.

Phụ thuộc: hoàn tất các đầu ra của GĐ4. Mọi lỗi chặn tích hợp phải được ghi người xử lý và kiểm thử lại trước khi chuyển bước.

### Bằng chứng tiến độ

Ghi đường dẫn sản phẩm, mã thay đổi, ca kiểm thử đã chạy, người rà soát và giờ thực tế trong sổ theo dõi. Mỗi thành viên phải có ít nhất một sản phẩm chính và một nội dung rà chéo ở giai đoạn này. Công việc chưa xong được chuyển tiếp có lý do, không đánh dấu hoàn tất chỉ vì hết tuần.

## 8.6 GĐ6 Tích hợp và kiểm thử hệ thống

Thời lượng dự kiến: tuần 6. Mỗi thành viên 12 giờ; tổng 36 giờ.

| Thành viên | Đầu việc và kiểm tra chéo |
| --- | --- |
| SV1 | Kiểm thử nhân viên nhập liệu, sửa lỗi phần phụ trách; rà lương. |
| SV2 | Kiểm thử công lịch thống kê, sửa lỗi; rà lưu tệp. |
| SV3 | Kiểm thử lương tệp sự cố, sửa lỗi; rà hồ sơ và nhập liệu. |

### Sản phẩm và điều kiện kết thúc

Tất cả AC đạt; không lỗi sai tiền mất dữ liệu khóa tháng.

Phụ thuộc: hoàn tất các đầu ra của GĐ5. Mọi lỗi chặn tích hợp phải được ghi người xử lý và kiểm thử lại trước khi chuyển bước.

### Bằng chứng tiến độ

Ghi đường dẫn sản phẩm, mã thay đổi, ca kiểm thử đã chạy, người rà soát và giờ thực tế trong sổ theo dõi. Mỗi thành viên phải có ít nhất một sản phẩm chính và một nội dung rà chéo ở giai đoạn này. Công việc chưa xong được chuyển tiếp có lý do, không đánh dấu hoàn tất chỉ vì hết tuần.

## 8.7 GĐ7 Hoàn thiện và bàn giao

Thời lượng dự kiến: tuần 7. Mỗi thành viên 8 giờ; tổng 24 giờ.

| Thành viên | Đầu việc và kiểm tra chéo |
| --- | --- |
| SV1 | Hoàn thiện URD, hướng dẫn người dùng; trình diễn hồ sơ. |
| SV2 | Hoàn thiện thiết kế, dữ liệu mẫu và báo cáo test; trình diễn công. |
| SV3 | Hoàn thiện kỹ thuật, bản chạy và README; trình diễn lương. |

### Sản phẩm và điều kiện kết thúc

Bộ tài liệu mã nguồn dữ liệu và biên bản bàn giao.

Phụ thuộc: hoàn tất các đầu ra của GĐ6. Mọi lỗi chặn tích hợp phải được ghi người xử lý và kiểm thử lại trước khi chuyển bước.

### Bằng chứng tiến độ

Ghi đường dẫn sản phẩm, mã thay đổi, ca kiểm thử đã chạy, người rà soát và giờ thực tế trong sổ theo dõi. Mỗi thành viên phải có ít nhất một sản phẩm chính và một nội dung rà chéo ở giai đoạn này. Công việc chưa xong được chuyển tiếp có lý do, không đánh dấu hoàn tất chỉ vì hết tuần.

## 9 Kế hoạch kiểm thử và quản lý chất lượng

| Nhóm | Ca kiểm thử trọng yếu | Người viết và rà |
| --- | --- | --- |
| Ngày và đầu vào | 29/02/2028 đúng; 29/02/2027 sai; chuỗi quá dài; EOF; số tràn. | SV2 và SV1 |
| Nhân viên | Trùng mã; ngày nghỉ sai; xóa có liên kết; sửa ngày gây mâu thuẫn. | SV1 và SV3 |
| Chấm công | Trùng khóa; Chủ nhật; 4 giờ; nghỉ có tăng ca; thiếu ngày. | SV2 và SV3 |
| Lương | Đủ công; nghỉ có lương; giữa tháng; ví dụ 14.437.500; làm tròn; thực lĩnh âm. | SV3 và SV1 |
| Lưu tệp | Trường rỗng; dòng hỏng; FK thiếu; ghi lỗi; tmp còn lại; lưu và nạp. | SV3 và SV2 |
| Hệ thống | Nhập → công → kiểm tra → tính → chốt → xuất → mở lại; thử sửa tháng chốt. | Cả ba thành viên |

### 9.1 Mức độ lỗi

Nghiêm trọng: sai tiền, mất dữ liệu, vượt bộ nhớ hoặc sửa được tháng chốt. Cao: không thực hiện được yêu cầu bắt buộc. Thấp: lỗi trình bày không làm sai dữ liệu. Điều kiện bàn giao: tất cả AC đạt, không còn lỗi nghiêm trọng và cao; lỗi thấp còn lại được ghi rõ và nhóm thống nhất xử lý.

### 9.2 Biểu mẫu kết quả kiểm thử

| Mã ca | Dữ liệu và bước chạy | Mong đợi | Thực tế và kết luận |
| --- | --- | --- | --- |
| [TC...] | [Điền dữ liệu và thao tác] | [Điền kết quả đúng] | [Điền đạt hoặc lỗi; người ngày] |

Ghi kết quả thực tế sau khi chạy, không điền sẵn trạng thái đạt trong kế hoạch. Sau sửa lỗi, chạy lại ca lỗi và các ca liên quan đến cùng quy tắc.

## 10 Theo dõi thành phần và thay đổi

Bảng dưới đây là sổ theo dõi để cập nhật trong quá trình thực hiện, không xác nhận các thành phần đã hoàn thành. Mỗi mô đun có người làm chính và người rà; phân công cụ thể ở giai đoạn vẫn ưu tiên cân bằng giờ công.

| Thành phần | Giai đoạn | Chính và rà | Trạng thái và minh chứng |
| --- | --- | --- | --- |
| input và main | GĐ3 | SV1 / SV2 | [Chưa bắt đầu; commit; test] |
| date_utils | GĐ3 | SV2 / SV1 | [Chưa bắt đầu; commit; test] |
| storage và parser | GĐ3 GĐ4 | SV3 / SV2 | [Chưa bắt đầu; commit; test] |
| employee | GĐ4 | SV1 / SV3 | [Chưa bắt đầu; commit; test] |
| attendance | GĐ4 | SV2 / SV1 | [Chưa bắt đầu; commit; test] |
| Kiểm tra thiếu và tích hợp | GĐ4 | SV3 / SV2 | [Chưa bắt đầu; commit; test] |
| Thông số tháng và search | GĐ5 | SV1 / SV3 | [Chưa bắt đầu; commit; test] |
| Tổng hợp và statistics | GĐ5 | SV2 / SV1 | [Chưa bắt đầu; commit; test] |
| Tính lương snapshot chốt | GĐ5 | SV3 / SV2 | [Chưa bắt đầu; commit; test] |

### 10.1 Luồng quản lý mã nguồn

Mỗi đầu việc phát triển trên nhánh riêng hoặc bản làm việc riêng, đưa vào nhánh chung sau khi biên dịch, kiểm thử và được một thành viên khác rà. Tránh cùng sửa models.h mà chưa thống nhất. Thông điệp thay đổi gắn FR, SCR hoặc mã lỗi. Chốt phiên bản bàn giao và giữ bản dữ liệu mẫu riêng với dữ liệu thử nghiệm.

### 10.2 Sổ thay đổi

Mỗi dòng ghi mã thay đổi, lý do, FR và SCR bị ảnh hưởng, mô đun, người làm, người rà, ngày và ca kiểm thử lại. Thay công thức phải sửa đồng bộ URD, thiết kế, calculatePayroll và dữ liệu kỳ vọng.

## 11 Rủi ro và bàn giao

| Rủi ro | Cách xử lý |
| --- | --- |
| Lệch công thức giữa các mô đun | Một hàm tính chung; ví dụ tiền bắt buộc; rà chéo công thức. |
| Không khớp dữ liệu sau sự cố lưu | Tệp tạm và sao lưu; kiểm tra tham chiếu khi nạp; không ghi đè tệp lỗi. |
| Lệch khối lượng hoặc chậm phụ thuộc | Cập nhật giờ từng giai đoạn; chia lại phần chưa bắt đầu; tích hợp sớm. |
| Lỗi tiếng Việt hoặc lệch cột console | Kiểm tra trên máy trình diễn; danh sách rút gọn, phiếu hiển thị đủ. |
| Tràn số hoặc vượt mảng | Giới hạn tiền giờ sức chứa; kiểm tra phép nhân và cấp phát. |

### 11.1 Danh sách bàn giao

Ba tài liệu Word thống nhất phiên bản; mã nguồn và header; bản chạy cho môi trường đã chọn; README biên dịch; bộ dữ liệu mẫu; kết quả kiểm thử và lỗi còn lại; hướng dẫn sao lưu và phục hồi; kịch bản trình diễn ba phần cho ba sinh viên.

### 11.2 Kịch bản nghiệm thu

SV1 trình diễn hồ sơ, kiểm tra nhập sai và tìm kiếm. SV2 trình diễn công, kiểm tra thiếu và thống kê công. SV3 trình diễn bảng lương, chốt, lưu mở lại và khóa sửa. Mỗi thành viên giải thích một phần mình rà chéo để thể hiện hiểu quy trình chung.

### 11.3 Thông tin môi trường thực tế

| Thông tin cần ghi khi bàn giao | Giá trị |
| --- | --- |
| Hệ điều hành và console | [Điền phiên bản thực tế] |
| Trình biên dịch và phiên bản | [Điền sau khi cài đặt thống nhất] |
| Mã phiên bản nguồn và ngày kiểm thử | [Điền commit hoặc mã bản chạy] |
| Người đóng gói và hai người rà | [Điền SV thực hiện] |

## Liên kết bản triển khai

Xem [thiết kế kỹ thuật thực tế](04_Trien_khai_thuc_te.md) và [báo cáo kiểm thử](05_Ket_qua_kiem_thu.md). Các bảng phân công SV1 SV2 SV3 là kế hoạch để nhóm sử dụng, không phải xác nhận giờ công hoặc đóng góp thực tế.

## Bổ sung kỹ thuật theo bảng chấm điểm

EmployeeNode lưu Employee và con trỏ next. Thêm, sửa, xóa, tìm dùng trực tiếp danh sách; Merge Sort đệ quy sắp theo mã qua menu nhân viên mục 7. Giao diện tách thành ui_display.c, ui_employee.c, ui_attendance.c, ui_payroll.c và ui.c điều phối. Phân công bổ sung vẫn giữ giờ từng giai đoạn bằng nhau cho ba thành viên; xem bảng tại tài liệu 06.

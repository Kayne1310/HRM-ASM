# QUẢN LÝ CHẤM CÔNG VÀ TÍNH LƯƠNG NHÂN VIÊN

## Tài liệu phân tích thiết kế hệ thống

Mã tài liệu PT TK 01   |   Phiên bản 1.2   |   Ngày 05/11/2026

Tài liệu chuyển các yêu cầu URD 01 thành mô hình xử lý, cấu trúc dữ liệu và đặc tả màn hình console. Các mã màn hình được dùng trong kế hoạch triển khai và kiểm thử.

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

## 1 Mô hình tổng quan hệ thống

```text
              NGƯỜI QUẢN LÝ
       Nhập hồ sơ công thông số lương
                       |
                       v
       +-------------------------------+
       | GIAO DIỆN CONSOLE              |
       | Menu - biểu mẫu - báo cáo      |
       +-------------------------------+
                       |
                       v
       +-------------------------------+
       | XỬ LÝ NGHIỆP VỤ               |
       | Nhân viên | Chấm công | Lương  |
       | Tìm kiếm  | Thống kê           |
       +-------------------------------+
                       |
                       v
       +-------------------------------+
       | KIỂM TRA VÀ LƯU TRỮ TỆP       |
       | Đọc - kiểm tra - ghi - sao lưu |
       +-------------------------------+
                       |
                       v
           Tệp dữ liệu trên máy
```

Ranh giới hệ thống bao gồm giao diện, xử lý nghiệp vụ và lớp lưu tệp. Không có dịch vụ ngoài hay máy chủ. Người quản lý nhận danh sách, phiếu lương, thông báo lỗi và báo cáo tổng hợp từ console hoặc tệp xuất.

### 1.1 Phân rã chức năng

| Nhóm | Chức năng con và yêu cầu |
| --- | --- |
| Nhân viên | Thêm, sửa, nghỉ việc, xóa có điều kiện, tìm — FR01 FR02 |
| Chấm công | Ghi, sửa, xóa, xem, kiểm tra thiếu — FR03 FR04 |
| Lương | Thông số tháng, tổng hợp, xem, chốt — FR05 FR06 FR07 |
| Báo cáo và dữ liệu | Tìm, thống kê, xuất, lưu, thoát — FR08 FR09 FR10 |

## 2 Mô hình dữ liệu logic

```text
EMPLOYEE 1 --------------------- 0..N ATTENDANCE
 id PK                                  employeeId FK
                                        date PK cùng FK

EMPLOYEE 1 --------------------- 0..N PAYROLL_INPUT
                                        employeeId FK
                                        month year PK cùng FK

EMPLOYEE 1 --------------------- 0..N PAYROLL
                                        employeeId FK
                                        month year PK cùng FK

ATTENDANCE + PAYROLL_INPUT -----> PAYROLL
                   Tổng hợp theo nhân viên và tháng
```

PK là khóa định danh; FK là mã tham chiếu. Trong chương trình C, các ràng buộc được kiểm tra bằng hàm tìm kiếm trước khi cập nhật danh sách liên kết nhân viên, mảng công/lương hoặc đọc tệp. Bảng lương chốt giữ bản sao tên, phòng ban và mức lương, không đọc ngược giá trị hiện tại để tính lại lịch sử.

| Thực thể | Các thuộc tính chính |
| --- | --- |
| Employee | id, fullName, department, position, startDate, hasEndDate, endDate, baseSalary, allowance, status |
| Attendance | employeeId, date, type, regularHalfHours, overtimeHalfHours, note |
| PayrollInput | employeeId, month, year, baseSalary, allowance, bonus, deduction |
| Payroll | employeeId, employeeName, department, month, year, standardDays, paidHalfHours, overtimeHalfHours, các khoản tiền, trạng thái |

Ngày dùng Date gồm day month year; tiền dùng long long nguyên đồng; giờ dùng int đếm nửa giờ. Không lưu số công dạng float. Danh sách nhân viên tối đa 200, danh sách công tối đa 150.000 bản ghi.

## 3 Luồng tính lương và trạng thái

```text
Chọn kỳ
   |
   +--> Đã chốt? -- Có --> Xem bản chính thức
   | Không
   v
Kiểm tra thiếu công -- Có thiếu --> Danh sách bổ sung
   | Đủ
   v
Lấy thông số tháng --> Tính bảng tạm --> Đối chiếu
                                          |
                                  Yêu cầu chốt
                                          |
                                          v
                              Kiểm tra và tính lại
                                          |
                                   Xác nhận Y N
                                          | Y
                                          v
                                 Ghi tệp chính thức
                                  |              |
                                 Lỗi           Thành công
                                  |              |
                             Chưa chốt        Đã chốt
```

### 3.1 Kiểm soát dữ liệu thay đổi

Sửa công hoặc thông số của tháng chưa chốt làm mất hiệu lực bảng tạm trong bộ nhớ. Mỗi lần chốt phải tổng hợp lại từ dữ liệu nguồn hiện tại. Tháng không được coi là đã chốt khi mới bấm xác nhận hoặc khi tệp tạm mới được tạo.

### 3.2 Luồng danh sách và biểu mẫu

Từ danh sách chọn mã để xem hoặc sửa; thao tác cập nhật phải qua kiểm tra và xác nhận. Hủy không làm thay đổi dữ liệu. Trở về giữ bộ lọc trong phiên. Danh sách dài phân trang 10 dòng, dùng N để trang sau, P để trang trước và 0 để quay lại.

## 4 SCR01 Menu chính

Yêu cầu liên quan: FR01 đến FR10. Điểm điều hướng toàn bộ chương trình. Hiển thị số dữ liệu đã nạp và cờ chưa lưu.

### Bố cục console dự kiến

```text
1 Nhân viên | 2 Chấm công | 3 Bảng lương
4 Tìm kiếm  | 5 Thống kê  | 6 Lưu dữ liệu
0 Thoát
Lựa chọn: [ ]
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Lựa chọn | Số nguyên từ 0 đến 6; không nhận ký tự thừa. |
| Trạng thái dữ liệu | Hiển thị số hồ sơ, số công và có thay đổi chưa lưu hay không. |

### Thao tác và điều hướng

Chọn 1 đến 5 mở menu tương ứng; 6 chuyển SCR15; 0 chuyển SCR16.

### Ngoại lệ và thông báo

Nhập sai giữ nguyên màn hình và báo lựa chọn không hợp lệ. Tệp nạp lỗi phải xử lý tại khởi động trước khi cho cập nhật.

### Điều kiện kiểm thử màn hình

Nhập 2 mở chấm công; nhập abc yêu cầu nhập lại.

## 5 SCR02 Danh sách nhân viên

Yêu cầu liên quan: FR01 FR02 FR08. Xem, lọc và chọn nhân viên để thao tác.

### Bố cục console dự kiến

```text
Bộ lọc phòng ban: [ ]  Trạng thái: [ ]
Mã     Họ tên        Phòng ban     Trạng thái
NV001  Nguyễn Văn An Nhân sự       Đang làm
A Thêm | E Sửa | R Nghỉ | D Xóa | 0 Quay lại
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Phòng ban | Để trống xem tất cả; so khớp tên đã lưu. |
| Trạng thái | Tất cả, đang làm hoặc đã nghỉ. |
| Mã thao tác | Mã tồn tại trong danh sách; hiển thị hồ sơ trước xác nhận. |

### Thao tác và điều hướng

A mở SCR03 tạo mới; E mở SCR03 sửa; R mở SCR04; D chỉ xóa khi không có mọi dữ liệu liên quan.

### Ngoại lệ và thông báo

Không có kết quả hiển thị danh sách rỗng. Xóa có dữ liệu báo cần ghi nhận nghỉ việc.

### Điều kiện kiểm thử màn hình

Lọc nhân sự đúng số người; xóa người đã có công bị chặn.

## 6 SCR03 Thêm và sửa hồ sơ nhân viên

Yêu cầu liên quan: FR01. Biểu mẫu dùng hai chế độ tạo mới và chỉnh sửa. Mã bị khóa ở chế độ sửa.

### Bố cục console dự kiến

```text
Mã nhân viên: [NV001]    Chế độ: [THÊM / SỬA]
Họ tên: [                         ]
Phòng ban: [          ] Chức vụ: [          ]
Ngày vào: [dd/mm/yyyy]
Lương: [            ] Phụ cấp: [            ]
Y Xác nhận | N Hủy
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Mã | 1 đến 15 byte ASCII chữ số gạch dưới hoặc gạch ngang; chuẩn hóa viết hoa; duy nhất. |
| Tên phòng ban chức vụ | Bắt buộc; tối đa lần lượt 100, 60, 60 byte UTF-8; cấm dấu \| và xuống dòng. |
| Ngày vào | Ngày hợp lệ; thay đổi không được loại các bản ghi công hiện có. |
| Lương và phụ cấp | Lương lớn hơn 0; phụ cấp không âm; mỗi khoản không quá 1 tỷ đồng. |

### Thao tác và điều hướng

Kiểm tra từng trường, cho sửa trước khi Y. Lưu vào bộ nhớ và đánh dấu chưa lưu; thông số tháng đã tạo không tự cập nhật.

### Ngoại lệ và thông báo

Trùng mã hoặc đầy 200 người thì từ chối tạo. Dòng quá dài phải xả phần dư rồi nhập lại.

### Điều kiện kiểm thử màn hình

Tạo mã nv001 được chuẩn hóa; sửa mã bị khóa; nhập lương âm bị chặn.

## 7 SCR04 Ghi nhận nghỉ việc

Yêu cầu liên quan: FR02. Lưu ngày làm việc cuối cùng và chuyển trạng thái đã nghỉ.

### Bố cục console dự kiến

```text
Mã nhân viên: [NV001]
Họ tên: Nguyễn Văn An
Ngày làm cuối: [dd/mm/yyyy]
Y Ghi nhận | N Hủy
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Mã | Tồn tại; hiển thị đúng hồ sơ. |
| Ngày làm cuối | Không trước ngày vào; không làm công hiện có nằm sau ngày nghỉ. |

### Thao tác và điều hướng

Kiểm tra ảnh hưởng các tháng đã chốt; nếu thay đổi làm phạm vi lịch sử không nhất quán thì từ chối. Với tháng tạm bị ảnh hưởng, đánh dấu cần tính lại.

### Ngoại lệ và thông báo

Đã nghỉ thì hiển thị ngày hiện tại; sửa ngày vẫn áp dụng toàn bộ kiểm tra. Không xóa hồ sơ.

### Điều kiện kiểm thử màn hình

Ngày nghỉ trước ngày vào bị chặn; nhân viên nghỉ trong tháng vẫn có trong lương tháng đó.

## 8 SCR05 Menu và danh sách chấm công

Yêu cầu liên quan: FR03. Điều hướng nhập và xem dữ liệu công.

### Bố cục console dự kiến

```text
1 Chấm công một người | 2 Chấm lần lượt theo ngày
3 Xem theo ngày       | 4 Xem theo người và tháng
5 Sửa | 6 Xóa | 7 Kiểm tra thiếu | 0 Quay lại
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Ngày xem | Ngày hợp lệ; danh sách hiện các bản ghi đã có. |
| Mã và kỳ | Mã tồn tại; tháng 1 đến 12; năm 2000 đến 2100. |

### Thao tác và điều hướng

1 hoặc 2 mở SCR06; 5 mở bản ghi SCR06; 6 xác nhận xóa; 7 mở SCR07. Xem hiển thị loại công, giờ và ghi chú.

### Ngoại lệ và thông báo

Tháng đã chốt chỉ được xem. Xóa không tồn tại báo không tìm thấy. Xóa thành công làm bảng tạm cũ.

### Điều kiện kiểm thử màn hình

Xem theo ngày đúng bản ghi; xóa công chốt bị chặn.

## 9 SCR06 Biểu mẫu chấm công

Yêu cầu liên quan: FR03. Tạo hoặc sửa một bản ghi; chế độ lần lượt tái sử dụng biểu mẫu.

### Bố cục console dự kiến

```text
Mã: [NV001]  Ngày: [dd/mm/yyyy]
Loại: [1 Đi làm 2 Nghỉ có lương 3 Nghỉ không lương]
Giờ thường: [ ]  Tăng ca: [ ]
Ghi chú: [                              ]
Y Ghi nhận | N Bỏ qua hủy
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Mã và ngày | Nhân viên tồn tại; đúng lịch làm và khoảng ngày vào đến ngày nghỉ; tháng chưa chốt. |
| Loại công | Giá trị 1 đến 3. |
| Giờ | Đi làm 0,5 đến 8 giờ thường; tăng ca 0 đến 4; bước 0,5. Nghỉ có hai trường giờ bằng 0. |
| Ghi chú | Tối đa 200 byte; không chứa ký tự phân cách hoặc xuống dòng. |

### Thao tác và điều hướng

Tạo không ghi đè bản trùng. Sửa giữ nguyên khóa mã và ngày. Nhập lần lượt hiển thị tiến độ, bỏ qua không được tính thành nghỉ không lương.

### Ngoại lệ và thông báo

Nếu bản ghi tồn tại, gợi ý chọn sửa. Ngày Chủ nhật hoặc ngoài thời gian làm bị từ chối.

### Điều kiện kiểm thử màn hình

4 giờ chuyển thành 8 đơn vị nửa giờ; nghỉ có tăng ca bị chặn.

## 10 SCR07 Kiểm tra thiếu chấm công

Yêu cầu liên quan: FR04. Liệt kê ngày bắt buộc nhưng chưa có bản ghi.

### Bố cục console dự kiến

```text
Kỳ: [mm/yyyy]  Mã: [để trống nếu tất cả]
Mã      Họ tên            Ngày còn thiếu
NV001   Nguyễn Văn An     02/03, 03/03
Tổng số ngày thiếu: [ ]
1 Bổ sung công | 0 Quay lại
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Kỳ | Tháng 1 đến 12; năm 2000 đến 2100. |
| Mã tùy chọn | Rỗng là toàn bộ nhân viên có thời gian làm việc giao tháng. |

### Thao tác và điều hướng

Duyệt ngày làm việc trong phạm vi từng người, đối chiếu khóa công. Chọn bổ sung chuyển SCR06 theo mã và ngày.

### Ngoại lệ và thông báo

Không thiếu hiển thị dữ liệu đầy đủ. Người không thuộc tháng không xuất hiện. Không tự tạo dữ liệu nghỉ.

### Điều kiện kiểm thử màn hình

Người vào ngày 15 chỉ kiểm tra từ ngày 15; thiếu một ngày phải liệt kê đúng.

## 11 SCR08 Thông số lương tháng

Yêu cầu liên quan: FR05. Lưu bản sao lương tháng và nhập các khoản điều chỉnh.

### Bố cục console dự kiến

```text
Kỳ: [mm/yyyy]  Mã: [NV001]
Lương cơ bản: [ ]  Phụ cấp: [ ]
Thưởng: [ ]        Khấu trừ: [ ]
1 Lưu | 2 Lấy lại mức từ hồ sơ | 0 Hủy
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Kỳ và nhân viên | Nhân viên thuộc tháng; tháng chưa chốt. |
| Lương phụ cấp | Lần đầu sao chép từ hồ sơ; điều chỉnh có xác nhận; lương dương, phụ cấp không âm. |
| Thưởng khấu trừ | Không âm; mỗi khoản không quá 1 tỷ đồng. |

### Thao tác và điều hướng

Khởi tạo thưởng khấu trừ bằng 0. Lấy lại từ hồ sơ phải xác nhận và chỉ thay lương phụ cấp, giữ thưởng khấu trừ. Lưu làm bảng tạm cũ.

### Ngoại lệ và thông báo

Tháng đã chốt chỉ xem thông số sao chép. Hủy không thay dữ liệu.

### Điều kiện kiểm thử màn hình

Sửa hồ sơ không tự đổi tháng; lấy lại mức mới có thông báo xác nhận.

## 12 SCR09 Lập bảng lương tạm

Yêu cầu liên quan: FR06. Tính lại bảng tạm sau kiểm tra nguồn.

### Bố cục console dự kiến

```text
Kỳ cần lập: [mm/yyyy]
Công chuẩn: [ ]  Nhân viên thuộc kỳ: [ ]
1 Kiểm tra và tính | 0 Quay lại
Kết quả: [ĐỦ DỮ LIỆU / CÒN THIẾU / CÓ LỖI]
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Kỳ | Hợp lệ và chưa chốt; nếu đã chốt chuyển xem SCR10. |
| Thông số tháng | Phải có cho từng người; khởi tạo từ hồ sơ khi chưa có. |

### Thao tác và điều hướng

Kiểm tra thiếu qua SCR07, kiểm tra thông số, tổng hợp theo mã và kỳ rồi tính các khoản. Thành công chuyển SCR10 với nhãn TẠM.

### Ngoại lệ và thông báo

Còn thiếu hoặc dữ liệu sai liệt kê nguyên nhân; không hiển thị là bảng hoàn chỉnh. Thực lĩnh âm đánh dấu không hợp lệ, hướng về SCR08.

### Điều kiện kiểm thử màn hình

Ví dụ URD cho đúng 14.437.500; tính lại sau sửa thưởng cho kết quả mới.

## 13 SCR10 Bảng lương tháng

Yêu cầu liên quan: FR06 FR07 FR08. Xem danh sách kết quả và tổng cộng theo kỳ.

### Bố cục console dự kiến

```text
BẢNG LƯƠNG [mm/yyyy]   [TẠM / ĐÃ CHỐT]
Mã     Tên          Công     Tăng ca    Thực lĩnh
NV001  Nguyễn An    24,00    10,00      14.437.500
Tổng thực lĩnh: [ ]
1 Phiếu lương | 2 Chốt | 3 Xuất | 0 Quay lại
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Kỳ | Lấy kỳ đang chọn; có thể đổi để tra cứu. |
| Mã chi tiết | Tồn tại trong bảng đang xem. |
| Sắp xếp | Theo mã hoặc thực lĩnh tăng giảm; không thay dữ liệu gốc. |

### Thao tác và điều hướng

1 mở SCR11; 2 mở SCR12 khi bảng hợp lệ; 3 mở SCR15 xuất. Phân trang 10 dòng, tên dài rút gọn để xem chi tiết ở phiếu.

### Ngoại lệ và thông báo

Bảng tạm đã cũ phải tính lại trước khi xem kết quả cập nhật, chốt hoặc xuất. Bảng chốt đọc từ bản chính thức.

### Điều kiện kiểm thử màn hình

Tổng toàn bảng khớp tổng từng người, không chỉ trang đang hiển thị.

## 14 SCR11 Phiếu lương nhân viên

Yêu cầu liên quan: FR08. Trình bày đầy đủ đầu vào và từng khoản để đối chiếu.

### Bố cục console dự kiến

```text
PHIẾU LƯƠNG [mm/yyyy] [TẠM / ĐÃ CHỐT]
Mã tên phòng ban: [...]
Công chuẩn: [...]  Công hưởng: [...]
Giờ tăng ca: [...]
Lương cơ bản: [...] Phụ cấp tháng: [...]
Lương công: [...] Phụ cấp hưởng: [...]
Tăng ca: [...] Thưởng: [...] Khấu trừ: [...]
THỰC LĨNH: [...]
0 Quay lại
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Mã và kỳ | Lấy từ SCR10 hoặc tra cứu; phải có kết quả hợp lệ. |
| Dữ liệu hiển thị | Bản chốt dùng thông tin đã sao chép; bản tạm dùng đầu vào tháng và tổng hợp hiện tại. |

### Thao tác và điều hướng

Chỉ đọc. Hiển thị đơn vị đồng, giờ và công rõ ràng. Phiếu giữ tên đầy đủ, không cắt như danh sách.

### Ngoại lệ và thông báo

Không có lương thì báo chưa lập; không tự tạo phiếu có giá trị 0.

### Điều kiện kiểm thử màn hình

Đổi tên nhân viên sau chốt không làm đổi tên trên phiếu lịch sử.

## 15 SCR12 Xác nhận chốt bảng lương

Yêu cầu liên quan: FR07. Kiểm soát thao tác hoàn tất kỳ và khóa dữ liệu.

### Bố cục console dự kiến

```text
CHỐT LƯƠNG [mm/yyyy]
Số nhân viên: [...] Công còn thiếu: 0
Tổng thực lĩnh: [...]
Sau chốt không sửa công hoặc thông số tháng.
Y Xác nhận chốt | N Quay lại
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Xác nhận | Chỉ Y mới ghi; N hoặc 0 quay lại, không thay trạng thái. |
| Điều kiện | Đủ công, dữ liệu hợp lệ, thực lĩnh không âm, kỳ chưa chốt. |

### Thao tác và điều hướng

Tính lại trước khi hiển thị số cuối, xác nhận, ghi bản chính thức bằng tệp tạm và thay tệp an toàn. Chỉ sau thành công mới khóa tháng.

### Ngoại lệ và thông báo

Lỗi ghi giữ chưa chốt, thông báo đường dẫn và cho thử lại. Không chốt lặp hoặc ghi đè bản chính thức.

### Điều kiện kiểm thử màn hình

Giả lập thư mục không ghi được: trạng thái vẫn chưa chốt.

## 16 SCR13 Tìm kiếm tổng hợp

Yêu cầu liên quan: FR08. Tra cứu theo nhu cầu và chuyển đến màn hình chi tiết.

### Bố cục console dự kiến

```text
Đối tượng: [1 Nhân viên 2 Công 3 Lương]
Mã: [...] Tên: [...] Phòng ban: [...]
Kỳ: [mm/yyyy nếu tìm công hoặc lương]
1 Tìm | 0 Quay lại
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Điều kiện | Nhân viên hỗ trợ mã chính xác, tên chuỗi con và phòng ban. Công lương cần kỳ; mã tùy chọn. |
| Kết hợp | Các điều kiện không rỗng kết hợp AND; mã chuẩn hóa viết hoa. |

### Thao tác và điều hướng

Kết quả nhân viên chuyển SCR02; công chuyển SCR05 dạng xem; lương chuyển SCR10 hoặc SCR11.

### Ngoại lệ và thông báo

Không kết quả báo rõ; giữ bộ lọc để sửa. Tìm tên phân biệt dấu và hoa thường trong bản cơ bản.

### Điều kiện kiểm thử màn hình

Mã và phòng ban mâu thuẫn trả không kết quả, không bỏ qua một điều kiện.

## 17 SCR14 Thống kê

Yêu cầu liên quan: FR09. Tổng hợp nhân sự và số liệu tháng.

### Bố cục console dự kiến

```text
1 Nhân viên theo phòng ban trạng thái
2 Công nghỉ tăng ca theo tháng
3 Quỹ lương và xếp hạng thực lĩnh
Kỳ: [mm/yyyy]   Nguồn: [TẠM / ĐÃ CHỐT]
0 Quay lại
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Kỳ | Bắt buộc với công và lương; nhân sự hiện tại không bắt buộc. |
| Nguồn lương | Mặc định chính thức; nếu chọn tạm phải có bảng tạm còn hiệu lực. |

### Thao tác và điều hướng

Nhân sự đếm từ hồ sơ hiện tại; công từ các bản ghi kỳ; quỹ lương bằng tổng netPay của nguồn đã chọn. Người bằng tiền được sắp tiếp theo mã.

### Ngoại lệ và thông báo

Không có bản chốt thì thông báo chưa chốt, không trộn bảng tạm vào quỹ chính thức.

### Điều kiện kiểm thử màn hình

Tổng nhóm phòng ban bằng tổng nhân viên cùng bộ lọc.

## 18 SCR15 Lưu dữ liệu và xuất báo cáo

Yêu cầu liên quan: FR08 FR10. Tách lưu dữ liệu nghiệp vụ và xuất bản sao báo cáo.

### Bố cục console dự kiến

```text
1 Lưu dữ liệu đang thay đổi
2 Xuất bảng lương CSV
Tệp đích: [exports/bang_luong_YYYY_MM.csv]
Y Thực hiện | N Hủy
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Loại thao tác | Lưu ghi các tập dữ liệu nghiệp vụ; xuất không đổi trạng thái tháng. |
| Đường dẫn | Mặc định thư mục data hoặc exports; tệp xuất có sẵn phải xác nhận trước ghi đè. |

### Thao tác và điều hướng

Lưu kiểm tra từng tệp và báo tệp thành công thất bại. Xuất UTF-8 có tiêu đề kỳ và trạng thái; escape đúng trường CSV.

### Ngoại lệ và thông báo

Hết dung lượng hoặc không có quyền ghi: báo thất bại, giữ cờ chưa lưu tương ứng. Xuất thất bại không ảnh hưởng bản chốt.

### Điều kiện kiểm thử màn hình

Lưu rồi mở lại phải khớp; xuất không làm tháng chuyển sang đã chốt.

## 19 SCR16 Thoát và xử lý thay đổi

Yêu cầu liên quan: FR10. Kết thúc có kiểm soát dữ liệu chưa lưu.

### Bố cục console dự kiến

```text
Có dữ liệu chưa lưu.
1 Lưu rồi thoát
2 Bỏ thay đổi và thoát
0 Quay lại
```

### Trường dữ liệu và kiểm tra

| Trường hoặc điều kiện | Đặc tả |
| --- | --- |
| Lựa chọn | Chỉ nhận 0, 1, 2; bỏ thay đổi cần xác nhận Y N. |

### Thao tác và điều hướng

Không có thay đổi thì thoát. Chọn 1 lưu thành công tất cả mới kết thúc; chọn 2 chỉ bỏ thay đổi trong bộ nhớ, không hoàn tác tệp đã ghi.

### Ngoại lệ và thông báo

Lưu lỗi quay lại, giữ dữ liệu trong bộ nhớ. EOF khi nhập không được lặp vô hạn; chuyển xử lý thoát an toàn và báo nếu chưa lưu.

### Điều kiện kiểm thử màn hình

Mô phỏng lỗi lưu: chương trình còn mở và cho người dùng xử lý.

## 20 Ma trận truy vết và quy ước giao diện

| Yêu cầu | Màn hình | Mô đun |
| --- | --- | --- |
| FR01 FR02 | SCR02 SCR03 SCR04 | employee |
| FR03 FR04 | SCR05 SCR06 SCR07 | attendance date_utils |
| FR05 | SCR08 | payroll |
| FR06 FR07 | SCR09 SCR10 SCR11 SCR12 | payroll storage |
| FR08 | SCR10 SCR11 SCR13 SCR15 | search payroll storage |
| FR09 | SCR14 | statistics |
| FR10 | SCR01 SCR15 SCR16 | main storage input |

### Quy ước chung

Mỗi biểu mẫu ghi rõ chế độ thêm sửa hoặc chỉ xem. Enter kết thúc một trường; 0 quay lại tại menu; Y N dùng ở bước xác nhận riêng. Không dùng 0 để hủy một trường tiền vì 0 có thể là giá trị hợp lệ. Hủy nhập biểu mẫu được thực hiện tại bước xác nhận hoặc bằng lựa chọn hủy được ghi rõ.

Ngày hiển thị dd/mm/yyyy, tệp dùng yyyy-mm-dd. Tiền hiển thị phân cách hàng nghìn và đơn vị đồng; tệp lưu chữ số nguyên. Với console hẹp, bảng danh sách chỉ hiển thị cột chính; phiếu chi tiết hiển thị đầy đủ. Không cắt chuỗi UTF-8 giữa các byte của một ký tự.

### Thông báo chuẩn

Mỗi lỗi nêu trường hoặc đối tượng và cách xử lý: Mã NV001 đã tồn tại; Ngày 31/04 không hợp lệ; Tháng đã chốt chỉ được xem; Còn thiếu công ngày 02/03 của NV001; Không ghi được tệp, dữ liệu chưa được lưu.

### Kiểm tra thiết kế trước lập trình

Đối chiếu đủ FR, không có đường đi sửa tháng đã chốt; kiểm tra nút quay lại và hủy; kiểm tra khóa mã ngày; thống nhất công thức với URD và kiểm tra các bước ghi tệp với KT 01.

## Liên kết bản triển khai

Xem [thiết kế kỹ thuật thực tế](04_Trien_khai_thuc_te.md) và [báo cáo kiểm thử](05_Ket_qua_kiem_thu.md). Các bảng phân công SV1 SV2 SV3 là kế hoạch để nhóm sử dụng, không phải xác nhận giờ công hoặc đóng góp thực tế.

## Bổ sung màn hình sắp xếp nhân viên

SCR02 có thêm mục 7 Sắp xếp theo mã bằng Merge Sort. Chức năng sắp xếp trực tiếp các nút danh sách liên kết bằng đệ quy; không sửa khóa hồ sơ, dữ liệu công hoặc snapshot. Cho phép sắp sau chốt, lưu thứ tự mới và mở lại.

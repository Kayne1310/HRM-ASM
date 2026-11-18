# Quản lý chấm công và tính lương nhân viên

Phiên bản 1.2. Dự án C11 chạy hoàn toàn trên console, được triển khai từ bộ tài liệu URD, phân tích thiết kế và kế hoạch kỹ thuật của đề tài.

## Chạy thử nhanh trên Windows

1. Giải nén toàn bộ dự án vào thư mục ngắn, ví dụ `C:\Projects\ChamCongTinhLuong`. Giữ cấu trúc thư mục sau khi giải nén.
2. Mở terminal trong thư mục dự án.
3. Chạy bản Windows x64 đã biên dịch:

```text
build\chamcong.exe --data examples/demo --payroll 3 2026
```

Kết quả mẫu cần có:

| Nhân viên | Thực lĩnh đồng |
| --- | ---: |
| NV001 Nguyễn Văn An | 14.437.500 |
| NV002 Trần Thị Bình | 660.000 |
| NV003 Lê Minh Cường | 11.440.000 |
| Tổng quỹ lương | 26.537.500 |

Để tương tác bằng menu, dùng một bản sao dữ liệu mẫu:

```text
mkdir demo-run
copy examples\demo\*.txt demo-run\
build\chamcong.exe --data demo-run
```

Chỉ sao chép mẫu khi tạo bộ demo lần đầu. Lần sau chạy tiếp với `--data demo-run` để giữ thay đổi. Hoặc mở `Chay_demo.cmd`; script tự khởi tạo thư mục demo khi chưa có hồ sơ.

Khởi tạo dự án trống để tự nhập toàn bộ:

```text
build\chamcong.exe
```

Mặc định dữ liệu được lưu ở `data/`, báo cáo ở `exports/`. Chương trình C không yêu cầu Node.js hoặc trình biên dịch khi chạy bản `.exe` có sẵn.

**Đường dẫn Windows:** vị trí OneDrive hiện tại rất dài. Trong môi trường thực hiện, Windows không chạy trực tiếp được tệp vừa biên dịch từ đường dẫn đó; bản chạy và kiểm thử đã hoạt động từ thư mục ngắn. Nên giải nén gói vào đường dẫn ngắn như ví dụ trên. Tên và dữ liệu tiếng Việt vẫn dùng UTF-8.

## Cấu trúc dành cho sinh viên

Nhân viên được lưu bằng **danh sách liên kết đơn** với đủ thêm, xóa, sửa, tìm. Menu nhân viên mục **7** chạy **Merge Sort đệ quy** để sắp theo mã. Mã được định dạng nhất quán, có chú thích tiếng Việt, tên biến rõ nghĩa và tách giao diện theo chức năng.

Đọc [hướng dẫn đọc mã và chuẩn bị bảo vệ](docs/06_Huong_dan_doc_code_va_bao_ve.md) để đối chiếu đầy đủ bảng tiêu chí đánh giá và luyện giải thích con trỏ, đệ quy, lưu tệp.

## Chức năng đã triển khai

- Thêm, sửa, xem chi tiết, lọc, tìm và ghi nhận nghỉ việc của nhân viên.
- Chỉ xóa hồ sơ chưa có công, thông số lương hoặc bảng lương liên quan.
- Chấm công nhanh theo tháng cho một nhân viên, tự tạo ngày làm thứ Hai đến thứ Bảy và nhập riêng ngày nghỉ; vẫn có thể chấm công từng ngày.
- Sửa, xóa, xem theo ngày hoặc theo nhân viên/tháng.
- Phát hiện công còn thiếu; không tự biến ngày thiếu thành nghỉ không lương.
- Lưu thông số lương riêng từng tháng; nhập thưởng, khấu trừ; lấy lại mức từ hồ sơ có xác nhận.
- Tính lại bảng tạm mỗi lần xem; phiếu lương chi tiết; sắp xếp tăng/giảm thực lĩnh.
- Chốt tháng bằng bản dữ liệu cố định, chặn sửa công/thông số và thay đổi thời gian làm việc ảnh hưởng kỳ đã chốt.
- Tra cứu kết hợp điều kiện; thống kê nhân sự, công, tăng ca, quỹ lương và nhóm thực lĩnh cao nhất.
- Đọc/ghi tệp có kiểm tra; tệp tạm và bản sao lưu; xuất CSV có phân biệt tạm và đã chốt.
- Các danh sách dài được phân trang 10 dòng bằng `N`, `P`, `0`.

## Tài liệu Markdown

Ba tài liệu gốc đã được chuyển đầy đủ từ DOCX, giữ nội dung, bảng, sơ đồ chữ, bảng thông tin ba sinh viên và lịch sử chỉnh sửa:

1. [URD nghiệp vụ](docs/01_URD.md).
2. [Phân tích thiết kế và 16 màn hình](docs/02_Phan_tich_thiet_ke.md).
3. [Kỹ thuật và phân công 7 giai đoạn](docs/03_Ky_thuat_va_ke_hoach.md).

Tài liệu bổ sung phản ánh bản triển khai:

- [Thiết kế kỹ thuật thực tế và ma trận truy vết](docs/04_Trien_khai_thuc_te.md).
- [Kết quả kiểm thử thực tế](docs/05_Ket_qua_kiem_thu.md).
- [Hướng dẫn đọc mã và bảo vệ theo tiêu chí đánh giá](docs/06_Huong_dan_doc_code_va_bao_ve.md).
- [Lịch sử triển khai](CHANGELOG.md).

SV1, SV2, SV3 vẫn là ô thông tin và kế hoạch phân công để nhóm điền. Những bảng này không xác nhận ba sinh viên đã thực hiện các thay đổi trong bản triển khai này.

## Giả định tính lương giản lược

Đây là mô hình học tập, chưa phải hệ thống trả lương thực tế:

- Làm từ thứ Hai đến thứ Bảy; Chủ nhật nghỉ; chưa có ngày lễ và ca đêm.
- Công chuẩn tính từ lịch từng tháng, không cố định 26 cho mọi tháng.
- Một ngày 8 giờ; giờ nhập theo bước 0,5; thường tối đa 8 giờ, tăng ca tối đa 4 giờ.
- Nghỉ có lương được 1 công; nghỉ không lương được 0 công; ngày nghỉ không có giờ thường hoặc tăng ca.
- Lương và phụ cấp phân bổ theo công hưởng lương. Tăng ca bằng 150% đơn giá giờ thường.
- Người vào/nghỉ giữa tháng chỉ hưởng công trong thời gian làm việc, vẫn chia công chuẩn toàn tháng.
- Thưởng/khấu trừ nhập theo tháng; chưa tính thuế và bảo hiểm; không chốt thực lĩnh âm.
- Tiền lưu nguyên đồng bằng `long long`; giờ lưu số đơn vị nửa giờ bằng `int`.
- Làm tròn từng khoản đến đồng gần nhất, đúng nửa đồng làm tròn lên với khoản không âm.

Gọi `Q` là đơn vị nửa giờ hưởng lương, `T` là đơn vị nửa giờ tăng ca, `D` là công chuẩn:

```text
Q = tổng giờ thường quy đổi nửa giờ + 16 × số ngày nghỉ có lương
Lương công    = round(lương cơ bản × Q / (D × 16))
Phụ cấp hưởng = round(phụ cấp tháng × Q / (D × 16))
Tăng ca       = round(lương cơ bản × T × 3 / (D × 32))
Thực lĩnh     = lương công + phụ cấp hưởng + tăng ca + thưởng - khấu trừ
```

## Biên dịch từ mã nguồn

Dùng GCC, Clang hoặc `zig cc` có hỗ trợ C11. Thư viện của chương trình chỉ gồm thư viện C chuẩn và một lớp liệt kê thư mục nhỏ cho Windows/POSIX.

Với Node.js, công cụ dựng không cần cài thêm gói:

```text
node tools/build.mjs
node tools/build.mjs --cc "C:\duong-dan\clang.exe"
node tools/build.mjs --cc "C:\duong-dan\zig.exe"
```

Mặc định `gcc` được tìm trong PATH, hoặc đặt biến môi trường `CC`. Công cụ dựng tạo ứng dụng và hai bộ kiểm thử C, bật `-std=c11 -Wall -Wextra -Wpedantic -Werror`.

Biên dịch thủ công với GCC từ thư mục dự án:

```text
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude src/models.c src/date_utils.c src/input.c src/employee.c src/attendance.c src/payroll.c src/storage.c src/ui.c src/ui_display.c src/ui_employee.c src/ui_attendance.c src/ui_payroll.c src/search_statistics.c src/main.c -o build/chamcong.exe
```

Trên Visual Studio 2026, mở thư mục gốc dự án bằng **File → Open → Folder**, chọn cấu hình `x64-Debug` và target khởi chạy `chamcong`, rồi nhấn **F5**. Project CMake đặt UTF-8 cho cả thư viện nghiệp vụ và menu tiếng Việt.

Cũng có thể dựng Release bằng CMake trong Developer PowerShell:

```text
cmake -S . -B build/vs2026-release -G "Visual Studio 18 2026"
cmake --build build/vs2026-release --config Release --target chamcong
```

Trên Linux thay tên đầu ra GCC bằng `build/chamcong`.

## Kiểm thử

Sau khi dựng cả ba chương trình:

```text
node tools/test.mjs
node tools/stress.mjs
```

`tools/test.mjs` tạo dữ liệu cô lập trong thư mục tạm; chạy kiểm tra nghiệp vụ C và các luồng console bằng đầu vào tự động. Không thay đổi `data/` hoặc mẫu `examples/demo/`. Trên Windows, bản chạy được sao chép tới thư mục tạm ngắn để tránh giới hạn đường dẫn.

`tools/stress.mjs` kiểm tra giới hạn 200 nhân viên, 150.000 công và trường hợp vượt giới hạn. Kết quả máy đọc được lưu ở `build/test_report.json` và `build/stress_report.json`; bản trình bày ở tài liệu kết quả kiểm thử.

## Các lệnh tiện ích

```text
build\chamcong.exe --help
build\chamcong.exe --data demo-run --check
build\chamcong.exe --data demo-run --payroll 3 2026
build\chamcong.exe --data demo-run --export 3 2026 exports/bang_luong_2026_03.csv
build\chamcong.exe --data demo-run --finalize 3 2026
```

`--finalize` thực sự chốt và lưu tháng; không dùng trên bản mẫu gốc nếu muốn giữ mẫu chưa chốt. `--export` không ghi đè tệp đã có; menu cho phép xác nhận ghi đè. `--payroll` chỉ in kết quả, không tự chốt hoặc lưu thông số mới khởi tạo trong bộ nhớ.

Mã thoát: `0` thành công; `1` lỗi nghiệp vụ/tệp; `2` sai tham số hoặc kết thúc đầu vào khi còn dữ liệu chưa lưu. Trong menu, khi còn thay đổi và thoát có thể lưu, bỏ thay đổi hoặc quay lại. EOF không tự ghi tệp.

## Lưu trữ và phục hồi

Tệp dữ liệu văn bản UTF-8 **không BOM**, phân cách `|`. Không nhập `|`, xuống dòng hoặc ký tự điều khiển trong trường chuỗi. CSV xuất báo cáo có BOM UTF-8 để thuận tiện mở bằng Excel.

```text
employees.txt              hồ sơ
attendance.txt             công theo ngày
payroll_inputs.txt         thông số theo tháng
payroll_YYYY_MM.txt         kết quả chính thức và dấu chốt kỳ
```

Khi lưu dữ liệu nguồn, chương trình ghi `.tmp`, kiểm tra ghi/đóng, chuyển bản cũ sang `.bak`, rồi công bố bản mới. Bản chốt không bị ghi đè. Trước chốt phải lưu dữ liệu nguồn thành công. Khi mở lại, kết quả chốt được đối chiếu với công và thông số tháng; đổi mức lương/tên trên hồ sơ không làm thay đổi lịch sử.

Nếu phát hiện tệp lỗi, tệp tạm còn lại hoặc thiếu tệp chính nhưng có `.bak`, chương trình dừng cập nhật. Cách phục hồi:

1. Thoát chương trình và sao chép toàn bộ thư mục dữ liệu sang nơi khác để giữ bằng chứng.
2. Đọc thông báo xác định tệp và dòng lỗi.
3. Chọn bộ dữ liệu sao lưu nhất quán. Khôi phục tệp chính từ `.bak` phù hợp hoặc bản sao lưu toàn thư mục; không trộn các kỳ không khớp nhau.
4. Sau khi kiểm tra, chuyển tệp `.tmp` còn lại ra ngoài thư mục dữ liệu để tránh bị nạp nhầm. Không tự coi tệp tạm là bản chính.
5. Chạy `--check` trước khi mở menu cập nhật. Nếu có bảng chốt, phải khớp đầy đủ dữ liệu nguồn.

Giới hạn: đây là ứng dụng một người sử dụng, không có khóa giữa nhiều tiến trình và không có giao dịch đồng bộ nhiều tệp. Không chạy hai phiên cùng ghi một bộ dữ liệu. Tệp `.bak` theo từng tệp không thay thế việc sao lưu nhất quán toàn thư mục; chưa bảo đảm an toàn tuyệt đối khi mất điện.

## Quy ước giao diện thực tế

Menu dùng số thay cho phím chữ trong một số bản phác thảo; `Y/N` dùng khi xác nhận. Chuỗi giới hạn theo byte UTF-8. Số tiền trên console hiện ở dạng nguyên đồng không có dấu phân cách; bảng báo cáo tài liệu dùng dấu chấm để dễ đọc. Tìm tên phân biệt dấu và hoa thường. Không mở lại tháng đã chốt trong phiên bản này.

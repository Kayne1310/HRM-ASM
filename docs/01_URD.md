# QUẢN LÝ CHẤM CÔNG VÀ TÍNH LƯƠNG NHÂN VIÊN

## Tài liệu yêu cầu người sử dụng URD

Mã tài liệu URD 01   |   Phiên bản 1.2   |   Ngày 05/11/2026

Tài liệu xác định nhu cầu nghiệp vụ, phạm vi, quy tắc xử lý và điều kiện nghiệm thu hệ thống. Đây là căn cứ thống nhất giữa nhóm thực hiện và người đánh giá trước khi thiết kế, lập trình.

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

## 1 Mục đích và phạm vi nghiệp vụ

Hệ thống phục vụ người phụ trách nhân sự ghi nhận hồ sơ, chấm công hằng ngày, lập lương hằng tháng và tra cứu kết quả. Chương trình hoạt động trên console, một người dùng tại một thời điểm, dữ liệu lưu bằng tệp trên máy.

### 1.1 Các bên liên quan

| Đối tượng | Nhu cầu và trách nhiệm |
| --- | --- |
| Người quản lý nhân sự | Nhập hồ sơ, ghi nhận biến động nhân viên và chấm công. |
| Người lập lương | Kiểm tra công, nhập thưởng khấu trừ, xem và chốt bảng lương. Cùng người sử dụng console trong phiên bản cơ bản. |
| Giảng viên | Đánh giá nghiệp vụ, thiết kế, chất lượng và kết quả kiểm thử. |
| Nhóm SV1 SV2 SV3 | Phân tích, thiết kế, phát triển, kiểm thử và bàn giao. |

### 1.2 Phạm vi thực hiện

Bao gồm quản lý nhân viên; chấm công theo ngày; tính lương theo tháng; tìm kiếm; thống kê; lưu và xuất tệp. Người sử dụng có thể tra cứu dữ liệu lịch sử của nhân viên đã nghỉ.

Chưa bao gồm đăng nhập phân quyền, nhiều người dùng đồng thời, ca đêm, lịch nghỉ lễ, làm Chủ nhật, thuế, bảo hiểm, thiết bị chấm công, chuyển khoản và đồng bộ hệ thống bên ngoài.

### 1.3 Thuật ngữ

| Thuật ngữ | Định nghĩa |
| --- | --- |
| Công chuẩn | Số ngày từ thứ Hai đến thứ Bảy của toàn tháng. |
| Công hưởng lương | Giờ làm thường chia 8 cộng ngày nghỉ có lương. |
| Bảng tạm | Kết quả tính chưa chốt, phải tính lại khi dữ liệu nguồn thay đổi. |
| Bảng đã chốt | Kết quả tháng được lưu chính thức, khóa chỉnh sửa dữ liệu liên quan. |

## 2 Quy trình nghiệp vụ tổng quan

```text
Hồ sơ nhân viên
        |
        v
Chấm công theo ngày --> Kiểm tra thiếu và sai
        |                        |
        +<--- Bổ sung sửa đổi ----+
        |
        v
Thông số tháng --> Tính lương tạm
                         |
                         v
                 Đối chiếu và xác nhận
                         |
                         v
              Chốt và lưu bảng lương
                         |
                         v
               Tra cứu thống kê xuất tệp
```

### 2.1 Quy trình thực hiện

Đầu kỳ, người sử dụng thêm hoặc cập nhật hồ sơ nhân viên. Trong kỳ, mỗi ngày làm việc được ghi nhận một trạng thái chấm công cho từng nhân viên thuộc thời gian làm việc. Cuối kỳ, người lập lương kiểm tra các ngày còn thiếu, bổ sung dữ liệu và xác nhận thông số lương tháng.

Hệ thống tổng hợp bảng tạm. Người sử dụng đối chiếu từng khoản và xác nhận chốt. Chỉ khi ghi tệp thành công, tháng mới chuyển sang đã chốt. Sau đó chỉ được xem, thống kê và xuất kết quả; phiên bản cơ bản không mở lại tháng đã chốt.

### 2.2 Trạng thái nghiệp vụ

| Đối tượng | Trạng thái và chuyển đổi |
| --- | --- |
| Nhân viên | Đang làm → đã nghỉ khi nhập ngày làm việc cuối cùng. |
| Tháng lương | Chưa chốt → bảng tạm → bảng tạm đã cũ nếu nguồn đổi → tính lại → đã chốt. |
| Dữ liệu tệp | Chưa lưu → lưu thành công; lỗi ghi giữ trạng thái chưa lưu. |

## 3 Danh mục yêu cầu chức năng

Tất cả yêu cầu dưới đây thuộc mức bắt buộc của phiên bản cơ bản. Mã FR được dùng xuyên suốt bộ tài liệu để đối chiếu với màn hình, mô đun và ca kiểm thử.

| Mã | Chức năng | Kết quả người dùng cần |
| --- | --- | --- |
| FR01 | Quản lý hồ sơ | Thêm, xem, sửa hồ sơ; mã duy nhất; không xóa nhân viên có phát sinh. |
| FR02 | Ghi nhận nghỉ việc | Lưu ngày làm cuối cùng; bảo toàn lịch sử và kiểm tra các bản ghi hiện có. |
| FR03 | Chấm công | Thêm, sửa, xóa và xem theo ngày hoặc nhân viên; chống trùng nhân viên và ngày. |
| FR04 | Kiểm tra đầy đủ công | Liệt kê từng nhân viên và ngày thiếu; thiếu dữ liệu không tự coi là nghỉ không lương. |
| FR05 | Thông số lương tháng | Khởi tạo bản sao lương và phụ cấp; nhập thưởng, khấu trừ; cập nhật có chủ đích. |
| FR06 | Tính lương tạm | Tổng hợp công và tăng ca; áp dụng công thức; chỉ ra dữ liệu không hợp lệ. |
| FR07 | Chốt bảng lương | Tính lại, xác nhận, lưu thành công rồi khóa tháng; không tạo trùng. |
| FR08 | Tra cứu và xuất | Tìm mã, tên, phòng ban; xem bảng lương, phiếu lương và xuất CSV. |
| FR09 | Thống kê | Đếm nhân viên, tổng công, tăng ca, quỹ lương, xếp hạng thực lĩnh. |
| FR10 | Lưu và khôi phục phiên | Đọc khi khởi động; lưu theo yêu cầu; hỏi xử lý thay đổi khi thoát. |

### 3.1 Tiêu chí tìm kiếm

Mã nhân viên được chuẩn hóa về chữ ASCII viết hoa trước khi tìm chính xác. Tìm tên hỗ trợ chuỗi con có dấu và phân biệt hoa thường trong phiên bản cơ bản. Khi không có kết quả, hiển thị thông báo và giữ điều kiện tìm để nhập lại.

## 4 Quy tắc nghiệp vụ và giả định giản lược

Toàn bộ quy tắc lương dưới đây là mô hình học tập giản lược, không phải quy trình trả lương thực tế hoặc hướng dẫn áp dụng pháp luật.

| Mã | Quy tắc |
| --- | --- |
| BR01 | Làm từ thứ Hai đến thứ Bảy, 8 giờ/ngày; Chủ nhật nghỉ; chưa xử lý ngày lễ. Công chuẩn tính theo lịch thực tế từng tháng. |
| BR02 | Một nhân viên có tối đa một bản ghi/ngày. Chỉ chấm ngày làm việc tiêu chuẩn nằm trong thời gian làm việc. |
| BR03 | Đi làm từ 0,5 đến 8 giờ thường và 0 đến 4 giờ tăng ca, theo bước 0,5 giờ. |
| BR04 | Nghỉ có lương hưởng một công; nghỉ không lương hưởng không công. Hai loại nghỉ có giờ thường và tăng ca bằng 0. |
| BR05 | Thiếu bản ghi là dữ liệu chưa hoàn tất; không cho chốt tháng. |
| BR06 | Ngày nghỉ việc là ngày làm cuối. Người vào hoặc nghỉ giữa tháng chỉ có công trong khoảng hợp lệ; mẫu số vẫn là công chuẩn toàn tháng. |
| BR07 | Tăng ca hưởng 150 phần trăm đơn giá giờ; phụ cấp phân bổ theo công hưởng lương. |
| BR08 | Thưởng và khấu trừ không âm, nhập theo tháng. Không tự tính thuế hoặc bảo hiểm. |
| BR09 | Làm tròn từng khoản đến đồng rồi cộng trừ; thực lĩnh âm phải sửa trước khi chốt. |
| BR10 | Sau chốt, giữ nguyên tên, phòng ban và các thông số lương đã sao chép; không sửa công, thông số hoặc kết quả tháng đó. |
| BR11 | Xóa hẳn nhân viên chỉ khi chưa có chấm công, thông số tháng hoặc bảng lương. Các trường hợp khác ghi nhận nghỉ việc. |
| BR12 | Sửa ngày vào hoặc nghỉ không được làm dữ liệu hiện có nằm ngoài thời gian làm việc; thay đổi ảnh hưởng bảng tạm phải tính lại. |

## 5 Quy tắc tính lương và ví dụ đối chiếu

Gọi D là số công chuẩn toàn tháng; H là tổng giờ làm thường cộng 8 giờ cho mỗi ngày nghỉ có lương; O là giờ tăng ca; L là lương cơ bản; A là phụ cấp; B là thưởng; K là khấu trừ.

```text
Lương theo công = round(L × H / (D × 8))
Phụ cấp hưởng   = round(A × H / (D × 8))
Tiền tăng ca    = round(L × O × 1,5 / (D × 8))
Thực lĩnh      = Lương theo công + Phụ cấp hưởng
                 + Tiền tăng ca + B - K
```

round là làm tròn đến đồng gần nhất; đúng nửa đồng làm tròn lên đối với khoản không âm. D phải lớn hơn 0.

| Dữ liệu ví dụ | Giá trị |
| --- | --- |
| Công chuẩn; lương; phụ cấp | 26 ngày; 13.000.000 đồng; 1.300.000 đồng |
| Chấm công | 23 ngày đủ 8 giờ; 1 ngày nghỉ có lương; 2 ngày nghỉ không lương |
| Tăng ca; thưởng; khấu trừ | 10 giờ; 500.000 đồng; 200.000 đồng |
| Lương theo công | 12.000.000 đồng |
| Phụ cấp thực hưởng | 1.200.000 đồng |
| Tiền tăng ca | 937.500 đồng |
| Thực lĩnh | 14.437.500 đồng |

### 5.1 Trường hợp biên

Người làm 4 giờ được 0,5 công. Người nghỉ không lương cả kỳ được 0 lương theo công và 0 phụ cấp, nhưng thưởng riêng vẫn được xét nếu đã nhập. Nhân viên đã nghỉ trước tháng hoặc vào làm sau tháng không nằm trong danh sách tính. Nhân viên nghỉ trong tháng vẫn phải có dữ liệu công đến ngày làm cuối.

## 6 Đặc tả các tình huống sử dụng

### UC01 Quản lý nhân viên

Tác nhân: người quản lý. Tiền điều kiện: dữ liệu hệ thống đã nạp hợp lệ. Luồng chính: nhập mã → nhập hồ sơ → kiểm tra → xác nhận → lưu vào danh sách. Kết quả: hồ sơ mới xuất hiện và dữ liệu được đánh dấu chưa lưu. Ngoại lệ: trùng mã, ngày sai, lương không dương hoặc danh sách đầy thì không thêm.

### UC02 Ghi nhận chấm công

Tiền điều kiện: nhân viên tồn tại và tháng chưa chốt. Luồng chính: chọn nhân viên, ngày → kiểm tra lịch và thời gian làm việc → chọn loại công → nhập giờ phù hợp → xác nhận. Ngoại lệ: trùng bản ghi chuyển hướng sang thao tác sửa; ngày không hợp lệ hoặc đã chốt thì từ chối. Kết quả: công được ghi nhận và bảng tạm liên quan bị đánh dấu đã cũ.

### UC03 Lập và chốt lương

Tiền điều kiện: tháng hợp lệ, chưa chốt. Luồng chính: kiểm tra thiếu công → tạo hoặc xác nhận thông số tháng → nhập thưởng khấu trừ → tính và xem bảng tạm → yêu cầu chốt → tính lại và xác nhận → ghi tệp tháng → công bố trạng thái đã chốt. Ngoại lệ: thiếu công, thực lĩnh âm hoặc ghi tệp thất bại thì giữ chưa chốt.

### UC04 Tra cứu thống kê và xuất

Chọn điều kiện → kiểm tra → truy vấn → hiển thị kết quả và trạng thái tạm hoặc chính thức → tùy chọn xuất. Không có kết quả thì thông báo rõ, không tạo báo cáo giả. Tổng quỹ lương chính thức chỉ lấy bảng đã chốt; số liệu tạm được ghi nhãn riêng.

### UC05 Lưu và kết thúc

Lưu các thay đổi vào tệp, kiểm tra lỗi. Khi thoát và còn dữ liệu chưa lưu, cho chọn lưu, bỏ thay đổi hoặc quay lại. Nếu lưu thất bại, không tự đóng chương trình.

## 7 Yêu cầu dữ liệu và chất lượng

| Nhóm dữ liệu | Nội dung và ràng buộc |
| --- | --- |
| Nhân viên | Mã, họ tên, phòng ban, chức vụ, ngày vào, ngày nghỉ tùy chọn, lương, phụ cấp, trạng thái. Mã duy nhất và không sửa sau tạo. |
| Chấm công | Mã nhân viên, ngày, loại, giờ thường, tăng ca, ghi chú. Khóa ghép mã và ngày. |
| Thông số tháng | Mã, tháng năm, lương, phụ cấp, thưởng, khấu trừ. Khóa ghép mã và kỳ. |
| Bảng lương chính thức | Thông tin nhân viên được sao chép, công chuẩn, công hưởng, tăng ca, các khoản và thực lĩnh. Một nhân viên một kỳ. |

### 7.1 Yêu cầu phi chức năng

| Mã | Yêu cầu kiểm chứng |
| --- | --- |
| NFR01 | Chạy console; menu đánh số; nhập sai không làm chương trình treo hoặc thoát bất ngờ. |
| NFR02 | Lưu được 200 nhân viên và tối đa 150.000 bản ghi công; báo đầy trước khi ghi vượt giới hạn. |
| NFR03 | Tệp UTF-8; kiểm tra hiển thị tiếng Việt trên máy trình diễn; nhập chuỗi theo giới hạn byte. |
| NFR04 | Đọc kiểm tra phiên bản, từng dòng và quan hệ mã; tệp lỗi không bị âm thầm thay bằng dữ liệu rỗng. |
| NFR05 | Lưu qua tệp tạm và bản sao lưu; báo thành công chỉ sau khi đóng và thay tệp thành công. |
| NFR06 | Tiền lưu nguyên đồng, giờ theo nửa giờ; mọi phép tính có kiểm soát miền và tràn số. |

Giới hạn triển khai: một phiên chương trình ghi dữ liệu tại một thời điểm. Sao lưu nhiều tệp chưa bảo đảm giao dịch đồng bộ; phải kiểm tra nhất quán khi khởi động.

## 8 Tiêu chí nghiệm thu và quản lý thay đổi

| Mã | Kịch bản nghiệm thu | Điều kiện đạt |
| --- | --- | --- |
| AC01 | Thêm, sửa, tìm và nghỉ việc | Dữ liệu đúng; chặn trùng; giữ lịch sử. |
| AC02 | Chấm công và kiểm tra thiếu | Chặn ngày sai, trùng, ngoài thời gian làm; liệt kê đủ ngày thiếu. |
| AC03 | Tính lương ví dụ mục 5 | Thực lĩnh đúng 14.437.500 đồng. |
| AC04 | Nhân viên vào hoặc nghỉ giữa tháng | Công đúng khoảng hợp lệ, mẫu số công chuẩn toàn tháng. |
| AC05 | Chốt và thử sửa tháng | Chỉ chốt khi đủ và hợp lệ; sau chốt không sửa được. |
| AC06 | Thay lương hồ sơ sau chốt | Kết quả lịch sử giữ nguyên. |
| AC07 | Lưu, thoát và mở lại | Khôi phục đúng dữ liệu; lỗi lưu không báo thành công. |
| AC08 | Tra cứu thống kê và xuất | Tổng khớp bảng chi tiết; CSV ghi rõ kỳ và trạng thái. |

### 8.1 Điều kiện hoàn tất

Tất cả AC đạt trên bộ dữ liệu mẫu; không còn lỗi làm sai tiền, mất dữ liệu hoặc phá khóa tháng. Mã nguồn, hướng dẫn chạy, dữ liệu mẫu và biên bản kiểm thử được bàn giao cùng bộ tài liệu.

### 8.2 Kiểm soát thay đổi

Mỗi yêu cầu thay đổi ghi mã, người đề xuất, nội dung, ảnh hưởng tới nghiệp vụ, thiết kế, mã nguồn và kiểm thử. Nhóm thống nhất phạm vi, cập nhật phiên bản tài liệu và bảng lịch sử; chưa triển khai thay đổi làm khác công thức khi chưa sửa đồng bộ các tài liệu.

### 8.3 Quan hệ với tài liệu khác

Tài liệu PT TK 01 mô tả kiến trúc, dữ liệu và màn hình đáp ứng FR. Tài liệu KT 01 quy định cách phát triển, lưu trữ, kiểm thử và phân công. Khi có khác biệt nghiệp vụ, cập nhật URD trước rồi đồng bộ thiết kế và kỹ thuật.

## Liên kết bản triển khai

Xem [thiết kế kỹ thuật thực tế](04_Trien_khai_thuc_te.md) và [báo cáo kiểm thử](05_Ket_qua_kiem_thu.md). Các bảng phân công SV1 SV2 SV3 là kế hoạch để nhóm sử dụng, không phải xác nhận giờ công hoặc đóng góp thực tế.

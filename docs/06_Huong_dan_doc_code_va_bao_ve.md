# Hướng dẫn đọc mã và chuẩn bị bảo vệ

Phiên bản 1.2 ngày 05/11/2026. Bản này điều chỉnh mã nguồn theo bảng tiêu chí đánh giá do nhóm cung cấp. Các mức điểm là trọng số của bảng đánh giá, không phải điểm tự chấm hoặc cam kết điểm đạt.

## Đối chiếu tiêu chí đánh giá

| Tiêu chí | Trọng số | Phần thực hiện và bằng chứng |
| --- | ---: | --- |
| Tính đúng đắn chức năng | 4,0 | Nhân viên, công, lương, tìm kiếm, thống kê, kiểm tra nhập sai; bộ kiểm thử nghiệp vụ và console |
| Cấu trúc dữ liệu và giải thuật | 2,5 | Danh sách liên kết đơn EmployeeNode; thêm, sửa, xóa, tìm; Merge Sort đệ quy trên danh sách, sử dụng thật tại menu nhân viên mục 7 |
| Thao tác tệp tin | 1,5 | Đọc/ghi văn bản UTF-8; kiểm tra định dạng; bản sao lưu; giữ dữ liệu sau khi thoát và mở lại |
| Chất lượng mã nguồn | 1,0 | Tên biến rõ nghĩa, hàm kiểm tra riêng, dấu ngoặc đầy đủ, chú thích tiếng Việt, chia giao diện thành mô đun, main chỉ điều phối |
| Báo cáo và bảo vệ | 1,0 | URD, thiết kế, kỹ thuật, báo cáo kiểm thử và tài liệu giải thích thuật toán; nhóm cần tự luyện trình bày và trả lời vấn đáp |

Giảng viên có thể điều chỉnh trọng số theo đề cương. Kết quả kiểm thử không thay thế việc đọc mã và bảo vệ trực tiếp của từng thành viên.

## Thứ tự đọc mã đề xuất

1. **include/models.h**: hiểu Date, Employee, EmployeeNode, Attendance, PayrollInput, Payroll và AppData.
2. **src/employee.c**: đọc findEmployee, appendEmployee, saveEmployee, deleteEmployee; sau đó đọc ba hàm của Merge Sort.
3. **src/date_utils.c**: hiểu ngày hợp lệ, thứ và số công chuẩn.
4. **src/attendance.c**: hiểu khóa nhân viên/ngày, loại công, kiểm tra thiếu công.
5. **src/payroll.c**: bắt đầu từ calculatePayroll, rồi buildPayroll và finalizePayroll.
6. **src/ui_employee.c**: theo một luồng nhập → kiểm tra → xác nhận → gọi nghiệp vụ. Các giao diện công/lương có cách tổ chức tương tự.
7. **src/storage.c**: đọc saveData trước, rồi readBase, readSnapshot và commitTemp.
8. **src/main.c** và **src/ui.c**: xem cách chương trình ghép các phần lại.

Phần UTF-8 trong models.c, tham số dòng lệnh và công cụ kiểm thử Node.js là phần hỗ trợ. Có thể đọc sau khi đã hiểu nghiệp vụ và con trỏ. Chương trình chính vẫn viết bằng C; Node.js không tham gia tính lương.

## Danh sách liên kết được sử dụng như thế nào

~~~c
typedef struct EmployeeNode {
    Employee employee;
    struct EmployeeNode *next;
} EmployeeNode;
~~~

~~~text
app.employeeHead
        |
        v
 [NV003 | next] --> [NV001 | next] --> [NV002 | NULL]
~~~

Mỗi nút chứa một hồ sơ và địa chỉ nút sau. employeeHead là địa chỉ nút đầu; NULL nghĩa là không có nút. Không có mảng nhân viên song song làm nơi lưu chính.

- **Thêm**: malloc một nút, chép hồ sơ, đặt next bằng NULL và nối cuối danh sách.
- **Tìm**: current bắt đầu ở employeeHead, so mã rồi đi current = current->next.
- **Sửa**: findEmployee trả địa chỉ hồ sơ trong nút. Chép dữ liệu mới vào hồ sơ đó, giữ nguyên next.
- **Xóa**: dùng previous và current. Nút đầu thì đổi employeeHead; nút còn lại thì previous->next = current->next; cuối cùng free(current).
- **Giải phóng**: lưu địa chỉ nút sau trước khi free nút hiện tại, rồi tiếp tục tới NULL.

employeeAt là hàm hỗ trợ lấy hồ sơ thứ k khi tạo bảng hiển thị hoặc bảng công theo tháng. Hàm vẫn duyệt danh sách nên có độ phức tạp O(n); không phải phép truy cập mảng O(1). findEmployeeIndex trả thứ tự duyệt để đánh dấu cột nhân viên trong bảng tổng hợp công; thứ tự này được tính lại trong mỗi lần tổng hợp, không lưu cố định trong bản chấm công.

## Đệ quy thực hiện công việc gì

Hàm **mergeSortEmployees** sắp xếp danh sách theo mã. Chọn menu chính 1, rồi mục 7 để chạy thuật toán này.

~~~text
NV003 -> NV001 -> NV004 -> NV002
         chia đôi
  NV003 -> NV001      NV004 -> NV002
       chia đôi             chia đôi
 NV003   NV001        NV004   NV002
       trộn                 trộn
  NV001 -> NV003      NV002 -> NV004
                trộn
  NV001 -> NV002 -> NV003 -> NV004
~~~

Ba phần quan trọng:

1. **Điểm dừng**: head == NULL hoặc head->next == NULL. Danh sách không quá một phần tử đã được sắp xếp.
2. **Bước đệ quy**: splitEmployeeList chia thành hai nửa; mergeSortEmployees gọi lại chính nó trên từng nửa nhỏ hơn.
3. **Ghép kết quả**: mergeEmployeeLists nối hai nửa đã có thứ tự. Phần trộn dùng vòng lặp.

Thuật toán chỉ nối lại next của các nút, không cấp phát nút mới và không đổi địa chỉ hồ sơ. Thời gian O(n log n), độ sâu gọi đệ quy O(log n). Danh sách 200 nhân viên có số tầng chia đôi nhỏ; không dùng đệ quy theo số bản ghi chấm công 150.000.

Sắp xếp nhân viên không thay các khóa mã/ngày hoặc số tiền. Sau khi sắp xếp, dữ liệu công và lương vẫn liên kết bằng mã nhân viên.

## Giải thích công thức lương bằng dữ liệu mẫu

Trong calculatePayroll:

- standardDays là số công chuẩn thực tế của tháng.
- paidUnits là số đơn vị nửa giờ hưởng lương: 16 đơn vị bằng 1 ngày.
- overtimeUnits là số đơn vị nửa giờ tăng ca: 2 đơn vị bằng 1 giờ.
- Tiền dùng long long nguyên đồng, không dùng float để cộng tiền.

Ví dụ NV001 tháng 03/2026: 26 công chuẩn; 23 ngày làm đủ giờ và 1 ngày nghỉ có lương cho paidUnits = 384; 10 giờ tăng ca cho overtimeUnits = 20. Sau lương, phụ cấp, tăng ca, thưởng và khấu trừ, thực lĩnh là 14.437.500 đồng.

roundDiv chia số nguyên rồi xét phần dư để làm tròn. Từng khoản được làm tròn trước khi cộng thực lĩnh.

## Các hàm được tách để dễ giải thích

| Phần | Tệp | Nội dung |
| --- | --- | --- |
| Kiểm tra hồ sơ | employee.c | validateEmployeeFields, validateExistingRecords, validateLockedMonths |
| Thao tác liên kết | employee.c | appendEmployee, findEmployee, deleteEmployee |
| Đệ quy | employee.c | splitEmployeeList, mergeSortEmployees, mergeEmployeeLists |
| Nhập dữ liệu | input.c | readLine, parseInteger, parseHalfHours, các hàm ask |
| Tính lương | payroll.c | calculatePayroll không nhập bàn phím hoặc ghi tệp |
| Giao diện chung | ui_display.c | Nhập lựa chọn, phân trang, hiển thị danh sách và phiếu |
| Giao diện nhân viên | ui_employee.c | Mỗi thao tác có một hàm biểu mẫu, menu dùng switch |
| Giao diện công và lương | ui_attendance.c, ui_payroll.c | Tách khỏi nghiệp vụ để không lặp công thức |
| Khởi động | main.c | Đọc tùy chọn, cấp bộ nhớ, nạp, gọi giao diện/lệnh, giải phóng |

.clang-format lưu quy tắc định dạng: thụt 4 khoảng trắng, dòng khoảng 100 cột, không viết if/for nhiều lệnh trên một dòng, thêm ngoặc khối rõ ràng. Công cụ định dạng chỉ cần khi phát triển, không cần khi chạy chương trình.

## Kịch bản trình diễn theo tiêu chí

1. Mở bản sao dữ liệu mẫu và tạo thêm một nhân viên có mã ở giữa danh sách.
2. Tìm hồ sơ, sửa chức vụ; giải thích hàm tìm trả con trỏ tới đâu.
3. Chọn sắp xếp mục 7, chỉ ra điểm dừng và hai lời gọi đệ quy trong mã nguồn.
4. Thêm vài hồ sơ chưa phát sinh để trình diễn xóa đầu, giữa và cuối danh sách.
5. Thử nhập trùng mã, ngày không hợp lệ, giờ sai hoặc xóa người đã có công.
6. Xem NV001 với 14.437.500 đồng, chốt kỳ rồi thử sửa công tháng đã chốt.
7. Lưu, thoát, mở lại và chứng minh dữ liệu giữ nguyên.

Không chỉnh trực tiếp tệp nguồn dữ liệu đã chốt trong buổi trình diễn. Dùng bản sao demo riêng để không mất dữ liệu mẫu.

## Câu hỏi vấn đáp cần hiểu

- Vì sao EmployeeNode có con trỏ next trỏ lại chính kiểu EmployeeNode?
- Employee* khác EmployeeNode* ở đâu? Hàm nào trả từng loại?
- Vì sao xóa nút đầu phải đổi employeeHead?
- Nếu free(current) rồi mới đọc current->next thì sai ở đâu?
- Vì sao sắp xếp không làm mất chấm công dù thứ tự nhân viên thay đổi?
- Điểm dừng đệ quy là gì? Vì sao lần gọi sau có bài toán nhỏ hơn?
- Vì sao phần trộn dùng vòng lặp? Khi nào nên tránh đệ quy sâu?
- Vì sao không dùng fflush(stdin)? Dòng quá dài được xử lý ra sao?
- Vì sao tiền dùng long long, còn giờ dùng đơn vị nửa giờ?
- Lỗi ghi tệp xảy ra thì tháng có được đánh dấu đã chốt không?
- Vì sao đổi lương hồ sơ không làm đổi bảng lương cũ?

Mỗi thành viên nên tự chạy và giải thích mã, không chỉ học thuộc mô tả. Các câu trả lời phải dựa vào chính hàm trong dự án.

## Phân công bổ sung trong kế hoạch ba thành viên

Giữ số giờ bằng nhau của mỗi giai đoạn ở tài liệu 03. Bổ sung trọng tâm học tập:

| Giai đoạn | SV1 | SV2 | SV3 |
| --- | --- | --- | --- |
| Phân tích | Quy tắc hồ sơ và xóa | Ngày công và dữ liệu biên | Lương, lưu tệp, tiêu chí chấm |
| Thiết kế | Nút và liên kết nhân viên | Bảng công theo tháng | Hợp đồng hàm, snapshot và đệ quy |
| Nền tảng | Danh sách và cấp phát | Nhập liệu và lịch | Parser, lưu tệp và rà con trỏ |
| Hồ sơ và công | Thêm sửa xóa tìm | Chấm công và kiểm tra thiếu | Merge Sort, tích hợp và rà liên kết |
| Lương và báo cáo | Thông số và tìm kiếm | Tổng hợp và thống kê | Công thức và chốt kỳ |
| Kiểm thử | Lỗi hồ sơ và giao diện | Xóa nút, sắp xếp và lịch | Tệp, lương và hồi quy |
| Bàn giao | Giải thích liên kết | Giải thích công và đệ quy | Giải thích lương và lưu tệp |

Đây là kế hoạch phân công để nhóm áp dụng, không phải xác nhận đóng góp thực tế của từng sinh viên.

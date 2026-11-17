# Kết quả kiểm thử thực tế

Phiên bản 1.2 ngày 05/11/2026, kiểm tra trên Windows x64. Báo cáo tổng hợp từ lần chạy thành công sau khi chuyển nhân viên sang danh sách liên kết và chỉnh mã dễ đọc.

## Tổng hợp

| Hạng mục | Kết quả |
| --- | --- |
| Biên dịch | Zig 0.13.0 C compiler, chuẩn C11, Wall Wextra Wpedantic Werror |
| Mục tiêu dựng | Ứng dụng, test_core, test_linked_list đều thành công |
| Nghiệp vụ và tệp | 152 điều kiện, không thất bại |
| Danh sách liên kết và đệ quy | 853 điều kiện, không thất bại |
| Tích hợp và console | 50 kiểm tra, không thất bại |
| Sức chứa | Nạp 200 nhân viên và 150.000 công thành công |
| Thời gian nạp lần đo cuối | 0.649 giây trên máy kiểm thử; không phải cam kết hiệu năng |
| Vượt giới hạn công | Từ chối 150.001 công, không ghi đè nguồn |
| Vượt giới hạn nhân viên | Từ chối 201 nhân viên, không ghi đè nguồn |

## Kiểm tra danh sách liên kết

Bộ test_linked_list kiểm tra danh sách rỗng, một nút, tìm không thấy, thêm, sửa tại nút, mã trùng, xóa đầu/giữa/cuối/nút duy nhất, sắp xếp danh sách đảo, sắp lại danh sách đã có thứ tự, giới hạn 200, địa chỉ hồ sơ không đổi sau sắp xếp, số nút và đuôi NULL, lưu rồi nạp lại thứ tự. Bộ kiểm thử tích hợp còn kiểm tra sắp xếp không làm lệch công/lương và không phá bản chốt.

853 điều kiện gồm các lần lặp kiểm tra từng nút của bộ dữ liệu 200 nhân viên; không nên hiểu đây là 853 tình huống sử dụng độc lập.

## Kết quả lương mẫu

| Mã | Thực lĩnh đồng |
| --- | ---: |
| NV001 | 14.437.500 |
| NV002 | 660.000 |
| NV003 | 11.440.000 |
| Tổng quỹ tháng 03/2026 | 26.537.500 |

## Danh mục tích hợp và console

| STT | Kiểm tra | Kết quả |
| --- | --- | --- |
| 1 | Nạp mẫu 3 nhân viên 58 công | Đạt |
| 2 | Ví dụ lương URD và tổng quỹ | Đạt |
| 3 | Xuất CSV tạm | Đạt |
| 4 | Không ghi đè CSV từ CLI | Đạt |
| 5 | Chốt bảng lương thành công | Đạt |
| 6 | Mở lại nhận tháng đã chốt | Đạt |
| 7 | Chặn chốt trùng | Đạt |
| 8 | Đọc snapshot chính thức | Đạt |
| 9 | Xuất CSV đã chốt | Đạt |
| 10 | Thiếu công chặn chốt | Đạt |
| 11 | Không tạo snapshot khi thiếu | Đạt |
| 12 | Tệp có mã trùng bị từ chối | Đạt |
| 13 | Khóa công trùng bị từ chối | Đạt |
| 14 | Chủ nhật trong tệp bị từ chối | Đạt |
| 15 | Phát hiện tệp tạm chưa phục hồi | Đạt |
| 16 | Thiếu tệp chính còn backup không coi là rỗng | Đạt |
| 17 | Chặn thực lĩnh âm | Đạt |
| 18 | Snapshot sai tiền bị từ chối | Đạt |
| 19 | Snapshot không khớp nguồn bị từ chối | Đạt |
| 20 | Menu nhập sai rồi thoát an toàn | Đạt |
| 21 | Màn hình danh sách nhân viên | Đạt |
| 22 | Màn hình kiểm tra thiếu | Đạt |
| 23 | Màn hình phiếu lương | Đạt |
| 24 | Tìm tên và phòng ban AND | Đạt |
| 25 | Thống kê không trộn lương tạm vào chính thức | Đạt |
| 26 | Thống kê lương tạm tổng đúng | Đạt |
| 27 | Thêm hồ sơ qua console và lưu | Đạt |
| 28 | Hồ sơ nhập console giữ sau mở lại | Đạt |
| 29 | EOF có thay đổi không tự ghi và không lặp vô hạn | Đạt |
| 30 | EOF không ghi dữ liệu chưa xác nhận lưu | Đạt |
| 31 | Console chặn xóa công tháng đã chốt | Đạt |
| 32 | Xem đầy đủ hồ sơ không sửa dữ liệu | Đạt |
| 33 | Sửa hồ sơ và lưu qua console | Đạt |
| 34 | Đổi mức hồ sơ không tự đổi thông số tháng | Đạt |
| 35 | Ghi nhận nghỉ việc qua console | Đạt |
| 36 | Nhập thông số tháng qua console | Đạt |
| 37 | Tính lại theo thưởng mới | Đạt |
| 38 | Nhập công một người qua console | Đạt |
| 39 | Bổ sung công phục hồi tính lương đúng | Đạt |
| 40 | Sửa công qua console | Đạt |
| 41 | Sửa nửa công thay đổi lương đúng | Đạt |
| 42 | Xóa nhân viên chưa phát sinh qua console | Đạt |
| 43 | Hủy biểu mẫu không thay dữ liệu | Đạt |
| 44 | Bỏ thay đổi khi thoát không ghi tệp | Đạt |
| 45 | CSV escape dấu nháy và dấu phẩy | Đạt |
| 46 | Menu sắp xếp liên kết và lưu thứ tự | Đạt |
| 47 | Sắp xếp nhân viên không làm lệch tổng công lương | Đạt |
| 48 | Cho sắp xếp sau chốt mà không sửa lịch sử | Đạt |
| 49 | Mở lại bảng chốt sau khi sắp xếp | Đạt |
| 50 | CLI kiểm tra tháng ngoài miền | Đạt |

## Tái lập

~~~text
node tools/build.mjs --cc "DUONG_DAN_DEN_GCC_CLANG_HOAC_ZIG"
node tools/test.mjs
node tools/stress.mjs
~~~

Báo cáo máy đọc: build/build_report.json, build/test_report.json, build/stress_report.json. Dữ liệu kiểm thử được tạo riêng trong thư mục tạm; không ghi đè bộ mẫu hoặc dữ liệu của người dùng.

## Phạm vi xác nhận

- Đã biên dịch và chạy trên Windows x64. Linux, MSVC và đường dựng CMake chưa được nghiệm thu bằng chạy thực tế.
- Các luồng console được kiểm tra bằng dữ liệu nhập UTF-8 tự động; chưa quan sát thủ công trên mọi terminal.
- Chưa dùng công cụ phát hiện rò rỉ bộ nhớ chuyên dụng. Bộ kiểm thử kiểm tra các liên kết và trạng thái sau giải phóng trong phạm vi có thể quan sát.
- Không có giao dịch đồng bộ nhiều tệp, khóa nhiều tiến trình hoặc phục hồi tự động. Giữ quy trình sao lưu và phục hồi trong README.
- Các giả định tính lương học tập vẫn áp dụng. Kết quả kiểm thử không phải chứng nhận dùng để trả lương thực tế và không phải điểm tự chấm theo rubric.

# Kiểm chứng bản 1.0

## Build

- C++17, SFML 2.6.2, nlohmann/json 3.11.3, CMake 3.28.3.
- Linux: GCC 13, Debug, build/link thành công.
- Windows x64: cross-compile bằng MinGW-w64 GCC 13 POSIX, Release; tạo executable dạng Windows GUI, không mở console phụ.
- Không có warning/error từ mã game trong các lượt build cuối với `-Wall -Wextra -Wpedantic`.
- Đã kiểm tra bảng import của executable: ngoài DLL hệ thống Windows chỉ cần `openal32.dll`, có kèm trong gói. Bản DLL OpenAL đi kèm chỉ phụ thuộc DLL hệ thống.
- SFML và runtime GCC liên kết tĩnh; không cần cài compiler để chơi bản Windows.

## Các kiểm tra đã đạt

CTest `core_behaviour` đạt, bao gồm:

- Nạp JSON, 13 topping, 6 khách, 8 loại nâng cấp, 5 topping mở đầu, ngày mặc định 120 giây.
- Tô có giới hạn sức chứa; sau trộn không nhận thêm topping; reset đúng.
- Công thức chính xác/thiếu/thừa/sai; trọng số tốc độ; tiền món/tip và cập nhật ví đúng, không trả tiền hai lần.
- Mua nâng cấp đủ/thiếu tiền, cấp tối đa, hiệu lực từng cấp, mở khóa topping không trừ tiền lặp.
- Xé đủ/thiếu đường, xé ngược, hủy thao tác và nâng cấp dung sai.
- Timing trộn Perfect/Miss, chặn double-click, phản xạ con trỏ, đủ số nhịp và nâng cấp vùng xanh.
- Khách đầu ngày, spawn, hàng đợi 3/4 người, độ phức tạp tăng, kết thúc ngày đúng một lần, sao trong khoảng 1–5.
- Ghi/đọc JSON giữ tiền/ngày/mở khóa/nâng cấp/âm lượng; phục hồi từ backup khi save bị hỏng.
- Font có glyph tiếng Việt, cache tài nguyên, fallback ảnh/âm thanh thiếu.
- Nút click một lần, hủy khi kéo ngoài/mất focus, state bị che không nhận update; push/pop/replace/clear không hủy đối tượng đang chạy callback.

Đã chạy game thật qua X11/Xvfb với OpenGL phần mềm, điều khiển bằng sự kiện chuột/phím:

- Menu → cách chơi → hai thanh âm lượng → mở quán.
- Xé ba đường → thêm topping → trộn ba nhịp → kéo tô giao → kiểm tra tiền và save.
- Pause/tiếp tục → hết giờ → tổng kết → mua nâng cấp → mở sa tế → ngày tiếp → rưới sốt.
- Về menu/thoát → mở lại → đối chiếu JSON, tiền/ngày/nâng cấp/âm lượng giữ nguyên.
- Mở toàn bộ topping; phân trang đủ 13 loại, hiển thị 4 khách/đơn phức tạp, đèn/rèm và resize 900×900 có letterbox.
- Đã xem ảnh render để rà tiếng Việt, khay, đơn hàng và cửa hàng.

Dữ liệu kiểm thử UI nằm ở thư mục tạm riêng: ngày rút còn 30 giây, tiền tăng để kiểm tra mua hàng nhanh. **Dữ liệu bàn giao vẫn là 120 giây/ngày, 30.000 đ ban đầu và 5 topping mở sẵn.** Không kèm save kiểm thử trong gói.

Mã game được chạy thêm với AddressSanitizer và UndefinedBehaviorSanitizer; bộ kiểm thử và vòng UI trên không báo lỗi truy cập bộ nhớ/undefined behavior. Bật khi tự build trên GCC/Clang Unix bằng `-DTIEM_SANITIZERS=ON -DTIEM_BUILD_TESTS=ON`.

## Phạm vi chưa xác minh

- Chưa chạy executable trên máy Windows thật; kiểm chứng Windows ở đây là cross-compile/link và kiểm tra DLL. Chưa kiểm thử MSVC/macOS thực tế.
- Âm thanh đã nạp và chạy qua OpenAL null backend trong kiểm thử; chưa nghe qua thiết bị âm thanh thật trong môi trường này.
- LeakSanitizer không chạy được vì môi trường hạn chế truy cập `/proc`/ptrace; phép kiểm tra ASan/UBSan dùng `detect_leaks=0`. Không khẳng định đã chứng minh toàn bộ thư viện/driver không rò rỉ.
- OpenGL ảo không hỗ trợ AA4, vsync và udev monitor; SFML vẫn chạy với cấu hình khả dụng và giới hạn FPS riêng. Đây là thông báo môi trường kiểm thử.

Đồ họa hiện là hình khối pastel, nhạc/SFX tổng hợp nguyên bản. Bộ PNG minh họa trong ASSETS.md là hướng dẫn sản xuất/thay hình, không phải bộ tranh đã có sẵn.

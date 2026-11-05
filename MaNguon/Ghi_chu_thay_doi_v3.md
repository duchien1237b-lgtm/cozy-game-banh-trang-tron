# Bánh Tráng Trộn Tiệm — Ghi chú thay đổi v3

Gói: `Banh_Trang_Tron_v3_TinhNangMoi.zip`, làm tiếp từ `Banh_Trang_Tron 2.zip`. Giữ nguyên kiến trúc OOP cũ (StateStack/State, các lớp System tĩnh làm việc trên `Progress`, dữ liệu JSON).

## Tính năng mới

1. **Thời tiết nắng mưa**: mỗi ngày có dự báo Nắng / Nhiều mây / Mưa, có thể có mưa rào giữa ngày. Nắng: khách đến nhiều hơn 10% nhưng kém kiên nhẫn hơn. Mưa: khách ít hơn, kiên nhẫn hơn 18% và tip thêm 10% giá món. Thời tiết cũng đổi loại topping khách hay gọi. Có hạt mưa, mây, tiếng mưa. Ngày 1 luôn nắng.
2. **Chuẩn bị nguyên liệu**: màn Chuẩn bị trước mỗi ngày, có dự báo, số khách dự kiến, giỏ hàng gợi ý sẵn. Giá vốn bằng 50% giá bán, tối đa 60 phần mỗi loại. Hết hàng giữa ngày thì *Nhập gấp* 5 phần với giá gấp đôi. Hàng tươi để qua đêm bị hỏng một phần.
3. **Nhịp trộn ngẫu nhiên**: sau mỗi nhịp, thanh trộn đổi kiểu (Đều, Nhanh, Chậm, Chập chờn, Đảo chiều), đổi độ rộng và vị trí vùng xanh. Nhịp đầu luôn chạy đều.
4. **Giờ 7:00 – 21:00**: đồng hồ trên HUD, trời sáng dần, trưa, hoàng hôn rồi tối, quán lên đèn. Giờ trưa (11–13h) và tan tầm (16:30–18:30) đông khách hơn. Có thông báo lúc 11h, 18h, 20h.
5. **Khách có tính cách và biểu cảm**: 7 tính cách trong `data/personalities.json` (Điềm đạm, Vui vẻ, Nóng tính, Khó tính, Hào phóng, Dễ tính, Nói nhiều), ảnh hưởng kiên nhẫn, tip, độ khắt khe. Mặt khách đổi theo thời gian chờ (vui, bình thường, chán, lo, giận, buồn, bất ngờ), có câu thoại riêng và phản ứng khi nhận món. Rê chuột lên khách để xem tính cách.
6. **Bảng kết quả cuối ngày mới**: hạng S/A/B/C/D, sao, 6 ô chỉ số, sổ thu chi (doanh thu, tip, tiền thời tiết, tiền nhập hàng, hàng hỏng, lãi ròng), biểu đồ khách phục vụ / bỏ về theo giờ, khách nổi bật, mục tiêu và thành tựu.
7. **Hướng dẫn lần đầu**: 7 trang, có 3 trang tập thật (xé bánh, thêm topping, trộn). Tự mở khi chơi lần đầu, có nút Bỏ qua, xem lại bằng nút *Cách chơi* ở menu.

Luồng chơi mới: Menu → (Hướng dẫn lần đầu) → Chuẩn bị → Mở quán → Kết quả cuối ngày → Chuẩn bị ngày sau.

## Thay đổi trong mã nguồn

- Lớp mới trong `src/gameplay/`: `WeatherSystem`, `WeatherPlan`, `DayClock`, `InventorySystem`, `CustomerMood`, struct `Personality`.
- State mới trong `src/states/`: `PrepState`, `DayReportState`, `TutorialState` (đăng ký trong `Game::registerStates`).
- `src/ui/Atmosphere.*`: mưa, ánh sáng theo giờ, emote khách.
- Sửa: `Model` (Progress có kho hàng, `DayResult` có sổ thu chi và thống kê theo giờ, `DaySession` có thời tiết/giờ), `MixingMinigame`, `PlayingState`, `MainMenuState`, `SaveSystem`, `AudioManager`.
- Dữ liệu: thêm `personalities.json`, thêm khóa mới trong `balance.json` và `customers.json`.
- Save cũ vẫn đọc được. Người đã chơi qua ngày 1 không bị bắt xem lại hướng dẫn.
- CMake: thêm tùy chọn `TIEM_USE_SYSTEM_DEPS=ON` để build không cần Internet.

## Kiểm tra

- Test tự động `tests/CoreTests.cpp` (đã thêm test cho thời tiết, giờ, kho, tính cách, nhịp trộn, save): PASS trên Linux, và PASS với bản Windows chạy dưới Wine.
- `GocNhoBanhTrang.exe` đã build lại, mở tới menu được dưới Wine. **Chưa chạy trên máy Windows thật**, nên chạy thử một lượt trước khi nộp.
- Ảnh minh họa các màn mới nằm trong `anh_minh_hoa/`.

## Chưa làm

Sơ đồ UML trong `UML/` vẫn là bản trước, chưa có các lớp mới.

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

## Kiểm chứng cập nhật tương tác chế biến

- Build Release MinGW thành công, CTest `core_behaviour` đạt.
- Kiểm tra 6 biến thể đường xé × 3 đường: đi hai chiều đạt điểm cao, đường tắt thẳng/đường chưa hết bị từ chối; hủy và nâng cấp độ dễ hoạt động.
- Thanh lực: không tăng khi chưa nhấn, bỏ click quá ngắn, trả đúng 1/2/3 muỗng, không cộng lặp khi giữ tối đa hoặc thả lần hai, hủy trả về 0.
- Vùng trộn: di chuyển sau Perfect và Miss, không di chuyển bởi nhấp đúp bị chặn, chấm theo tâm mới, vùng nâng cấp nằm trong thanh.
- Sinh đơn 80 trường hợp qua ngày 1/3/7/30: đều có 3–4 loại và không dùng nguyên liệu khóa. Kiểm tra đủ 5 mức sao/nhận xét.
- Công cụ preview đã chạy chuỗi tương tác qua PlayingState đến giao món thành công, xuất ảnh thật bằng SFML RenderTexture. Đã xem ảnh hủy thanh lực, giữ lực, vùng trộn đổi vị trí, nhận xét có sao, bốn khách/bốn loại topping.
- Chưa kiểm thử thủ công toàn bộ một ngày chơi. Kết quả ở trên là kiểm thử tự động và kiểm tra ảnh render.

## Kiểm chứng 29/10/2026: xé tự do và khay trong suốt

- Build Release MinGW thành công trên Windows; CTest `core_behaviour` đạt 1/1.
- Kiểm tra hai mốc 1,5/3 giây; nhấn ngoài bánh, giữ yên, rê không nhấn và rê ngoài bánh không tăng tiến độ. Thả giữa chừng/mất focus giữ tiến độ; có thể tiếp tục khi đã hiện ảnh cuối. Tô mới đặt lại tiến độ. Nâng cấp mở rộng vùng nhận chuột, không đổi thời lượng.
- Preview SFML phát các sự kiện chuột vào PlayingState, thực hiện 180 khung rê tại 60 FPS, chụp ảnh tại giây 1,5 và 3, thả để sang topping, hủy/thêm topping, trộn và giao món; kiểm tra số món đã giao tăng đúng một.
- Đã xem ảnh render với nguyên liệu lớn, nền hai khu chế biến trong suốt, hai ảnh bánh mới, bố cục bốn khách và các trang nguyên liệu mở đủ. Ảnh ở `previews/free-tearing/`.
- Tài nguyên mới đối chiếu từng byte với ZIP ở cả hai thư mục assets. Thử nghiệm dùng save riêng, không ghi vào tiến trình thật. Chưa chơi thủ công toàn bộ một ngày hoặc nghe lại âm thanh.

Bổ sung bánh vừa xé trong tô: build Release thành công; kịch bản preview xé/thêm topping/trộn/giao món đạt, đã xem ảnh tô chứa đúng sprite bánh xé nhỏ.

Cập nhật font và thông báo giữa màn: build Release, CTest 1/1, preview giao món thành công; đã xem menu, bốn khách, đánh giá và trạng thái sau 3,6 giây.

Trang trí mua được: build Release, CTest 1/1; preview kiểm tra từng món, mua đủ và lưu/tải lại với tiến trình thử riêng, đã xem ảnh các model. Tài nguyên hai thư mục đối chiếu từng byte với ZIP.

Bố cục decor.png: build Release; preview --decorated hoàn thành xé/thêm topping/trộn/giao món, kiểm tra ảnh trang trí riêng lẻ và đầy đủ với ba/bốn khách.

## Review sau trải nghiệm

- Build Release và CTest `core_behaviour` đạt; kiểm tra review mỗi lượt giao hợp lệ, tên/nội dung riêng, sao, giới hạn 100 bài, phản hồi Unicode 240 ký tự, sửa bài, lưu/tải lại, tương thích save cũ và dữ liệu review tùy chọn bị lỗi.
- Preview chạy màn chơi có trang trí đến giao món, mở cửa sổ review bằng nút menu thật, kiểm tra trạng thái trống/danh sách, từ chối phản hồi trống, nhập tiếng Việt và Backspace, lưu/sửa bằng Ctrl+A, lọc chưa phản hồi, trả lời đúng bài ở trang sau và tải lại.
- Đã xem ảnh menu, rèm che dây cờ, review, ô nhập và phản hồi đã lưu. Chưa chơi thủ công toàn bộ ngày hoặc kiểm tra bộ gõ IME của mọi hệ điều hành.

## Kiểm tra nhạc MP3

Build Release và CTest `core_behaviour` đạt. Công cụ preview với OpenAL null backend đã mở và giải mã mẫu từ cả hai MP3, xác nhận thời lượng 162,769 giây và 118,047 giây, vòng lặp, và chuỗi chuyển menu → chơi → tổng kết/cửa hàng → chơi → menu. Hai bản tài nguyên và thư mục runtime đối chiếu nguyên byte với file nguồn. Kiểm tra tự động không thay thế việc nghe thủ công qua loa.

## Kiểm tra hiệu ứng MP3

Build Release và CTest `core_behaviour` đạt. Preview với OpenAL null backend giải mã cả 9 MP3, phát các sự kiện thật từ PlayingState/ManagementState và menu, kiểm tra âm thanh xé, chọn topping, Perfect/Good/Miss, giao món, mua và nút. Tiếng nền được kiểm tra lặp trong màn chơi, dừng ngoài màn chơi, giảm về 0 khi tắt hiệu ứng. Kịch bản nhạc menu/màn chơi vẫn đạt. Đối chiếu từng byte các file trong ZIP với hai thư mục tài nguyên và bản runtime; chưa nghe thủ công qua loa.

## Kiểm tra add-effect

Release build và CTest core_behaviour đạt. Preview --sfx-test dùng OpenAL null đã giải mã pop/tiếng nền mới, kiểm tra ba lượt giữ với mức 1/2/3 phát chính xác 2/4/6 pop, chạy tới giao món và đánh giá. Kiểm tra tiếng nền lặp, âm lượng 21 khi thanh Hiệu ứng là 60 (35%), tắt khi rời màn chơi và khi âm lượng bằng 0. Ảnh các mức tại previews/add-effects/ingredient-drop-level-1.png đến level-3.png. Đã xem các khung hình video tham khảo và ảnh giao diện; chưa nghe thử qua loa.

## Kiểm tra nâng cấp cửa hàng

Release build và CTest đạt. Test mua thớt lần lượt ba cấp, kiểm tra thời gian mỗi giai đoạn và không hoàn thành sớm khi rê; các test nâng cấp và lưu tiến trình hiện có vẫn đạt. Preview mua toàn bộ nâng cấp qua UpgradeSystem ở ba mốc, xuất ảnh cửa hàng/thớt/tô ở previews/upgrade-feedback; kiểm tra lại luồng xé–thêm–trộn–giao và hiệu ứng âm thanh đạt. Không thay đổi save của người chơi.

## Kiểm tra logo menu

Build Release và CTest đạt; preview menu được kiểm tra trực quan tại previews/logo-menu/menu.png, kịch bản hiệu ứng và luồng chơi vẫn đạt. Đối chiếu ba tài nguyên với ZIP trong hai thư mục assets và runtime; file exe chứa tài nguyên icon Windows.

## Kiểm tra 5 khách và 5 hiệu ứng bổ sung

Build Release và CTest đạt. Mô phỏng 1000 hạt giống xác nhận cả 11 nhân vật có thể xuất hiện và Chị Dung có tần suất thấp hơn khách thường. Preview --sfx-test giải mã các MP3 mới, kiểm tra khách vui sau giao món 5 sao (fixture chỉ tính chất lượng xé), giọng nam/nữ lúc hết kiên nhẫn, kết thúc ngày và rót sốt kèm đúng hai pop cho mức 1. Xuất ảnh 5 nhân vật qua PlayingState và nạp đủ 15 biểu cảm; kiểm tra trực quan Chị Dung và Anh Bảo. Save dùng thư mục thử nghiệm; chưa nghe thử qua loa.

## Cập nhật âm thanh khách vui

Build Release và CTest đạt. Preview --sfx-test xác nhận dùng alter_customer_happy khi giao món đạt 4–5 sao, không phát âm thanh rót sốt, vẫn có đúng hai tiếng pop ở mức 1. Chưa nghe thử qua loa.

## Ngày chơi 3 phút và đơn nhiều topping

Release build, CTest và preview hiệu ứng đạt. Kiểm tra ngày chưa kết thúc ở 179 giây, kết thúc đúng 180 giây; đơn sinh ra chỉ dùng topping đã mở, mỗi loại 1–3 muỗng và vừa sức chứa. Ngày 7 có thể yêu cầu đủ 6 loại khi đã mở đủ nguyên liệu. Kiểm tra ảnh ô thoại ba hàng với bốn khách ở previews/larger-orders.

## Kiểm tra nhu cầu từng khách và tip 100%

Build Release, CTest và preview --sfx-test đạt. Kiểm tra khoảng số loại và số muỗng theo từng nhân vật trên nhiều hạt giống, nguyên liệu khóa và sức chứa tô. Test tip khách đặc biệt bằng giá trên đơn cả khi điểm thấp hoặc có nâng cấp trang trí; kiểm tra tiền vào ví/tổng kết và chống trả hai lần. Lưu ảnh giao diện tại previews/customer-preferences.

## Kiểm tra âm thanh nạp topping

Release build, CTest và preview --sfx-test đạt: sau 1,2 giây thanh đạt 50%; file MP3 giải mã được, thời gian phát khớp thanh lực 2,4 giây. Kiểm tra bắt đầu khi giữ, dừng khi rời tô/thả/đầy/tạm dừng, tắt theo âm lượng Hiệu ứng; tiếng pop vẫn đúng số icon. Bản gốc MP3 được đối chiếu trong assets và runtime. Chưa nghe thử qua loa.

## Kiểm tra khung giao diện

Release build, CTest và preview --sfx-test đạt sau khi đổi khung/vị trí. Kiểm tra trực quan màn chơi, bố cục có trang trí và 4 khách, thớt cấp 3 và thanh lực; tọa độ xé–thêm–trộn–giao, nút đổi trang và âm thanh nạp vẫn hoạt động. Ảnh tại previews/cozy-frames. Bốn PNG được đối chiếu nguyên byte với ZIP trong hai thư mục assets và runtime.

## Hoán đổi hai hiệu ứng

Release build và preview --sfx-test đạt; nhịp trộn Perfect phát alter_customer_happy.mp3, giao món đạt 4–5 sao phát perfect-mixed.mp3. Chưa nghe thử qua loa.

## Cập nhật âm thanh trộn

Build Release và preview --sfx-test đạt; ba nhịp Perfect/Good/Miss và khách vui phát đúng file mới. Hai file mới được đối chiếu nguyên byte với file do người dùng cung cấp. Chưa nghe thử qua loa.

## Kiểm tra menu và chơi lại từ đầu

Build Release, CTest và preview --sfx-test đạt. Kiểm tra vị trí nút review mới; mở Cài đặt → Cách chơi, ESC trở lại Cài đặt; tiếp tục ngày 10 giữ tiến trình; hủy chơi lại không đổi dữ liệu; xác nhận đưa về ngày 1 và lưu đúng tiền, topping, nâng cấp, đánh giá, giữ âm lượng. Test lỗi lưu xác nhận không làm mất tiến trình trong bộ nhớ. Tất cả dùng thư mục save thử nghiệm riêng, không thay đổi save thật. Ảnh tại previews/menu-navigation.

## Xoay thứ tự âm thanh trộn

Build Release và preview --sfx-test đạt: âm Miss cũ → Perfect, Good cũ → Miss, Perfect cũ → Good. Các nhịp được mô phỏng và kiểm tra file phát tương ứng.

## Ba file trộn theo tên

Build Release và preview --sfx-test đạt: ba nhịp Miss/Good/Perfect phát đúng ba file tương ứng trong thư mục mix. Đối chiếu nguyên byte file gốc với assets bản chạy, mã nguồn và runtime. Chưa nghe thử qua loa.

## Kiểm tra độ khó có trần

Build Release, CTest và preview hiệu ứng/menu đạt. Kiểm tra từng nhân vật trên 20 hạt giống, tại ngày 1/2/7/14/30/1000: khẩu phần không giảm theo ngày và không vượt 165% mức đầu; ngày 1 mỗi loại một muỗng; thời gian chờ dễ hơn và không giảm quá hệ số cho phép; ngày 14/30/1000 có cùng đơn và kiên nhẫn với cùng hạt giống. Tốc độ trộn giữ nguyên sau trần. Tiến trình thật không bị đặt lại.

## Kiểm tra cooking-fx

Build Release và CTest (1/1) đạt. Preview --sfx-test kiểm tra thao tác xé, chọn/giữ/thả/hủy topping, 2/4/6 icon và pop tương ứng, ba kết quả trộn, giao món và menu. Đã xem ảnh xé đang chạy, ba mức thanh lực và ba kết quả trộn tại previews/cooking-fx. Preview dùng save riêng, âm thanh dùng OpenAL null; không thử nghe bằng loa.

## Kiểm tra bộ thư mục đã dọn

Build Release đạt sau khi danh sách âm thanh trong công cụ preview được cập nhật theo AudioManager hiện tại. CTest đạt 1/1. Chạy preview --sfx-test trực tiếp với assets/ và data/ của bộ chơi sau khi xóa tài nguyên thừa: thao tác xé, thanh lực, hủy lượt giữ, icon/pop, ba mức trộn, giao món, menu, hướng dẫn và xác nhận chơi lại đều đạt. Preview dùng thư mục lưu kiểm tra riêng và OpenAL null.

Giữ mã nguồn, tests, tools/PreviewUi.cpp, công cụ tạo audio, script build, assets/data cần thiết và giấy phép. Xóa build cache/công cụ tạm/ảnh kiểm tra sau khi kiểm tra xong. SHA256SUMS.txt được làm mới theo các file còn lại.

# Hướng dẫn assets — Bánh Tráng Trộn Tiệm

Bản 1.0 đã có đồ họa hình khối pastel, font tiếng Việt và nhạc/SFX tổng hợp để chơi ngay. Danh sách PNG dưới đây là bộ tranh đề xuất khi thay đồ họa hình khối bằng tranh vẽ tay; gói hiện tại không chứa bộ PNG này. Mọi con số, tên và đơn hàng do UI vẽ, không nằm trong tranh.

## 1. Quy chuẩn mỹ thuật

- Cozy hand-drawn: hoạt hình vẽ tay ấm áp, nét nâu đậm mảnh, pastel ấm, bóng nhẹ.
- Bàn, tô, thớt, khay: nhìn thẳng từ trên xuống; không trộn phối cảnh isometric.
- Khách: chân dung nhìn chính diện hoặc hơi nghiêng, cùng một góc cho cả bộ.
- Bảng màu gợi ý: kem `#FFF8E6`, nâu `#51362C`, xanh lá `#4F6F4C`, đào `#EFA67C`, gỗ `#E8D1B0`.
- Hướng sáng cố định từ trên trái; bóng đổ xuống phải. Nét viền khoảng 2–3 px trên canvas 256×256.
- Sprite PNG RGBA, sRGB, nền trong suốt thật. Nền toàn màn hình được phép kín màu, xuất PNG 1280×720.
- Không vẽ chữ, số giá, thanh chỉ số hoặc lời thoại vào tranh: game hiển thị bằng font tiếng Việt để dễ sửa.
- Tên file: chữ thường, ASCII, `snake_case`, không dấu/khoảng trắng; đường dẫn phân biệt hoa thường trên Linux.
- Kích thước dưới đây là canvas xuất cuối, không phải kích thước AI bắt buộc phải sinh trực tiếp. Có thể vẽ lớn rồi thu nhỏ đồng bộ.

## 2. Danh sách hình ảnh

Tất cả đường dẫn ảnh tương đối với `assets/images/`.

### Quầy, thớt, bánh tráng và tô

| File | Canvas (px) | Nội dung |
|---|---:|---|
| `backgrounds/counter_bg.png` | 1280×720 | Nền quầy nhìn từ trên, để trống vùng thao tác và hàng khách |
| `backgrounds/main_menu_bg.png` | 1280×720 | Nền menu, vùng trái trống để đặt tên/nút |
| `backgrounds/day_result_bg.png` | 1280×720 | Nền tổng kết nhẹ, vùng giữa thoáng |
| `backgrounds/upgrade_shop_bg.png` | 1280×720 | Nền cửa hàng, các ô nâng cấp vẽ bằng UI |
| `tools/cutting_board.png` | 384×288 | Thớt gỗ bo góc |
| `rice_paper/paper_whole.png` | 256×256 | Tờ bánh tráng nguyên |
| `rice_paper/paper_strip_01.png` … `03.png` | 64×64 mỗi file | Ba biến thể sợi bánh |
| `rice_paper/paper_piece_01.png` … `03.png` | 64×64 mỗi file | Ba biến thể mảnh bánh |
| `bowls/bowl_empty.png` | 256×256 | Tô rỗng, tâm tô trùng tâm canvas |
| `bowls/bowl_full_01.png` | 256×256 | Mức ít bánh |
| `bowls/bowl_full_02.png` | 256×256 | Mức vừa |
| `bowls/bowl_full_03.png` | 256×256 | Mức đầy |
| `tools/spoon.png` | 64×128 | Muỗng, đầu muỗng hướng lên |

Giữ cùng viền tô/pivot cho mọi mức đầy. Topping động sẽ được vẽ thêm lên tô, tránh vẽ sẵn topping làm sai số lượng đơn. Đường xé nét đứt do code vẽ, không đóng cứng vào ảnh tờ bánh.

### Topping

Mỗi hàng có **3 PNG riêng** trong `toppings/`: khay `128×128`, hạt/mảnh `64×64`, icon `48×48`. Sốt dùng ảnh vệt/giọt ở cột mảnh; vẫn có icon nhận diện riêng.

| Topping / ID | Khay 128×128 | Mảnh 64×64 | Icon 48×48 | Nhóm / mở khóa |
|---|---|---|---|---|
| Muối tôm / `muoi_tom` | `muoi_tom_tray.png` | `muoi_tom_piece.png` | `muoi_tom_icon.png` | Khô / ban đầu |
| Xoài sống / `xoai_song` | `xoai_song_tray.png` | `xoai_song_piece.png` | `xoai_song_icon.png` | Tươi / ban đầu |
| Trứng cút / `trung_cut` | `trung_cut_tray.png` | `trung_cut_piece.png` | `trung_cut_icon.png` | Tươi / ban đầu |
| Hành phi / `hanh_phi` | `hanh_phi_tray.png` | `hanh_phi_piece.png` | `hanh_phi_icon.png` | Khô / ban đầu |
| Rau răm / `rau_ram` | `rau_ram_tray.png` | `rau_ram_piece.png` | `rau_ram_icon.png` | Tươi / ban đầu |
| Khô bò / `kho_bo` | `kho_bo_tray.png` | `kho_bo_piece.png` | `kho_bo_icon.png` | Khô / mua |
| Khô gà / `kho_ga` | `kho_ga_tray.png` | `kho_ga_piece.png` | `kho_ga_icon.png` | Khô / mua |
| Đậu phộng / `dau_phong` | `dau_phong_tray.png` | `dau_phong_piece.png` | `dau_phong_icon.png` | Khô / mua |
| Xúc xích / `xuc_xich` | `xuc_xich_tray.png` | `xuc_xich_piece.png` | `xuc_xich_icon.png` | Tươi / mua |
| Tóp mỡ / `top_mo` | `top_mo_tray.png` | `top_mo_piece.png` | `top_mo_icon.png` | Khô / mua |
| Sa tế / `sa_te` | `sa_te_tray.png` | `sa_te_piece.png` | `sa_te_icon.png` | Sốt / mua |
| Tắc / `tac` | `tac_tray.png` | `tac_piece.png` | `tac_icon.png` | Sốt/nước tắc / mua |
| Bơ / `bo` | `bo_tray.png` | `bo_piece.png` | `bo_icon.png` | Sốt bơ / mua |

Bánh tráng là nguyên liệu nền riêng ở bước xé, không tính thành topping cộng thêm. “Bơ” ở đây là sốt bơ dùng cho bánh tráng, không phải quả bơ; “tắc” là nước tắc ở thao tác rưới.

| Chai và vệt sốt | Canvas (px) | Ghi chú |
|---|---:|---|
| `tools/bottle_sa_te.png` | 96×192 | Chai sa tế đứng |
| `tools/bottle_tac.png` | 96×192 | Chai nước tắc đứng |
| `tools/bottle_bo.png` | 96×192 | Chai sốt bơ đứng |
| `tools/bottle_sa_te_tilt.png`, `bottle_tac_tilt.png`, `bottle_bo_tilt.png` | 192×192 | Góc nghiêng rưới, cùng pivot ở cổ chai |
| `effects/sauce_stream.png` | 32×128 | Vệt rưới có thể nhuộm màu bằng code |

### UI, hiệu ứng, trang trí

| File | Canvas (px) | Ghi chú |
|---|---:|---|
| `ui/button_normal.png`, `button_hover.png`, `button_pressed.png`, `button_disabled.png` | 256×64 | Nút không chữ, chừa biên 12 px để 9-slice |
| `ui/panel.png` | 256×256 | Khung hộp, biên 20 px để 9-slice |
| `ui/progress_track.png`, `progress_fill.png` | 256×32 | Thanh nền và phần đầy |
| `ui/order_bubble.png` | 256×160 | Bong bóng trống, đuôi hướng xuống |
| `ui/tooltip.png` | 256×96 | Tooltip trống |
| `ui/coin_icon.png` | 48×48 | Tiền |
| `ui/star_full.png`, `star_empty.png` | 48×48 | Sao đánh giá |
| `ui/lock_icon.png`, `pause_icon.png` | 48×48 | Khóa và tạm dừng |
| `effects/sparkle.png`, `crumb.png`, `coin_particle.png` | 32×32 | Hạt lấp lánh, vụn, tiền |
| `decor/lantern.png` | 128×192 | Đèn lồng |
| `decor/curtain.png` | 512×192 | Rèm cửa, lớp phía trên |
| `decor/potted_plant.png` | 128×128 | Cây nhỏ trang trí |

### Khách hàng: 6 người × 3 biểu cảm

Mỗi PNG `256×256`, cùng pivot và tỉ lệ đầu/thân, nền trong suốt. Ảnh tên thường `normal`, vui `happy`, giận `angry`.

| Nhân vật | 3 file trong `customers/` |
|---|---|
| Học sinh | `student_normal.png`, `student_happy.png`, `student_angry.png` |
| Sinh viên | `college_normal.png`, `college_happy.png`, `college_angry.png` |
| Cô hàng xóm | `auntie_normal.png`, `auntie_happy.png`, `auntie_angry.png` |
| Chú hàng xóm | `uncle_normal.png`, `uncle_happy.png`, `uncle_angry.png` |
| Shipper | `courier_normal.png`, `courier_happy.png`, `courier_angry.png` |
| Nhân viên văn phòng | `office_normal.png`, `office_happy.png`, `office_angry.png` |

Không dùng logo thương hiệu thật trên đồng phục. Biểu cảm giận nhẹ, phù hợp không khí thư giãn.

## 3. Mẫu prompt tạo ảnh

Giữ nguyên đoạn style này cho toàn bộ ảnh:

> cozy hand-drawn 2D game asset, top-down view, [OBJECT], thin dark brown outline, warm pastel colors, soft shading, light from upper left, transparent background, centered, no text, no logo, consistent style, clean readable silhouette

Ví dụ khay xoài:

> cozy hand-drawn 2D game asset, top-down view, a small square ingredient tray filled with thin Vietnamese green mango strips, thin dark brown outline, warm pastel colors, soft shading, light from upper left, transparent background, centered, no text, no logo, consistent style, clean readable silhouette

Ví dụ bộ khách: thay `top-down view` bằng `front-facing waist-up character portrait`; mô tả rõ người Việt, trang phục, tỉ lệ và biểu cảm. Dùng ảnh `normal` đã duyệt làm tham chiếu khi tạo `happy`/`angry`, không tạo ba nhân vật độc lập.

Nền quầy: thay `transparent background, centered` bằng `full 1280x720 scene, empty central work area, no characters, no UI, no text`.

Quy trình giữ phong cách:

1. Chốt 3 ảnh chuẩn: tô, khay, một khách. Duyệt bảng màu, độ dày nét, góc nhìn và hướng sáng.
2. Dùng lại đoạn style, cùng model/phiên bản, tham số và seed nếu công cụ hỗ trợ. Seed chỉ hỗ trợ tái lập trong cùng cấu hình; ảnh tham chiếu đã duyệt vẫn cần thiết để kiểm soát nét vẽ.
3. Tạo mỗi vật riêng hoặc theo lưới rõ ràng; không yêu cầu AI vẽ chữ tiếng Việt vào sprite.
4. Kiểm tra alpha thật bằng cách đặt ảnh lên nền trắng, đen và màu. Họa tiết caro bị vẽ vào ảnh không phải nền trong suốt.
5. Tách nền bằng công cụ chọn vùng/mask; giữ viền chống răng cưa, xóa quầng trắng. Vật bán trong suốt như bánh tráng cần xem lại thủ công.
6. Cắt sprite đúng canvas, chừa lề trong suốt 4–8 px. Ghi pivot nhất quán; không tự trim mỗi biểu cảm thành kích thước khác nhau.
7. Thu nhỏ về kích thước bảng, xuất PNG RGBA; kiểm tra độ đọc ở kích thước thật trong game. Giữ bản gốc độ phân giải cao ngoài thư mục runtime.
8. Đặt tên theo bảng; tối ưu PNG lossless, bỏ metadata không cần thiết. Không dùng JPEG cho sprite cần alpha.
9. Ghi tác giả/nguồn/quyền sử dụng và các chỉnh sửa vào README. Quyền sử dụng đầu ra AI cần kiểm tra theo điều khoản công cụ tại thời điểm tạo.

## 4. Âm thanh

Bản phát hành đã kèm hai bản nhạc WAV lặp và 11 SFX WAV PCM 16-bit mono 22.05 kHz, tạo bằng `tools/generate_audio.py`. Đây là âm thanh tổng hợp nhẹ; có thể thay bằng bản thu thật có giấy phép phù hợp. Giữ nguyên tên `.wav` để code tự nạp. Nếu chuyển sang OGG, sửa phần đuôi trong `AudioManager.cpp`. Với bản thu mới, có thể dùng 44.1/48 kHz, cắt khoảng lặng đầu, tránh clipping và làm điểm nối nhạc êm.

| File tương đối với `assets/audio/` | Vai trò / gợi ý tìm |
|---|---|
| `music/cozy_shop.wav` | Lo-fi/cozy không lời, loop lúc chơi |
| `music/day_result.wav` | Nhẹ hơn, loop màn tổng kết |
| `sfx/ui_click.wav` | Nút bấm ngắn, mềm |
| `sfx/paper_tear.wav` | Xé giấy/bánh tráng |
| `sfx/topping_drop.wav` | Nguyên liệu rơi vào tô |
| `sfx/sauce_pour.wav` | Rưới sốt; phát lại mỗi muỗng |
| `sfx/mix.wav` | Trộn món |
| `sfx/coin_receive.wav` | Tiếng ting khi nhận tiền |
| `sfx/customer_happy.wav` | Khách vui, không lời hoặc cảm thán ngắn |
| `sfx/customer_leave.wav` | Khách bỏ về, âm thanh nhẹ |
| `sfx/day_end.wav` | Hết giờ |
| `sfx/perfect.wav` | Thao tác tốt, ngắn và không chói |
| `sfx/upgrade_buy.wav` | Mua nâng cấp thành công |

Nguồn tham khảo do người tạo asset lựa chọn và kiểm tra từng file:

- **Freesound**: <https://freesound.org/> — tìm “paper tear”, “food stir”, “coin”, “soft click”. Xem giấy phép trên từng trang; không coi mọi file là cùng một giấy phép. FAQ: <https://freesound.org/help/faq/>.
- **Kenney**: <https://kenney.nl/assets> — tìm UI/interface/audio. Trang hỗ trợ nêu game assets trên các trang asset theo CC0; vẫn giữ license đi kèm pack: <https://kenney.nl/support>.
- **OpenGameArt**: <https://opengameart.org/> — nhạc và SFX, kiểm tra license cùng yêu cầu credit của từng tác giả: <https://opengameart.org/content/faq>.
- **Pixabay Music**: <https://pixabay.com/music/> — tìm cozy/lo-fi/cafe; dùng theo Pixabay Content License, không tự gán CC0. Xem <https://pixabay.com/service/license-summary/> và <https://pixabay.com/service/faq/>; lưu bản license/thông tin tải và kiểm tra khả năng Content ID.

Ưu tiên nguồn có điều kiện sử dụng rõ ràng, phù hợp cách phát hành dự kiến. Đọc đầy đủ license thay vì chỉ dựa vào chữ “miễn phí”. Ghi credit trong README với: file, tên tác phẩm, tác giả, URL gốc, license và phiên bản, ngày tải, chỉnh sửa. Không đưa nhạc tải về vào kho mã nguồn công khai nếu chưa kiểm tra quyền phân phối lại file đó.

## 5. Font và fallback

- Đã kèm `assets/fonts/BeVietnamPro-Regular.ttf`, hỗ trợ dấu tiếng Việt, cùng `OFL.txt` nguyên bản.
- Nguồn: <https://github.com/google/fonts/tree/main/ofl/bevietnampro>. Tác giả/giấy phép ghi trong README và file OFL.
- Mọi chuỗi hiển thị đi qua `sf::String::fromUtf8`; file `.cpp/.hpp/.json` lưu UTF-8; MSVC bật `/utf-8`.
- `ResourceManager` cache font, ảnh, âm thanh; ảnh không tồn tại/hỏng trả `nullptr` để `drawImageOrPlaceholder()` vẽ hình màu và nhãn. Game hiện dùng `ui/Art.cpp` để tự vẽ nguyên liệu, chân dung, tô, đèn/rèm, kèm nhãn và số lượng trong state. Thiếu file âm thanh thì hiệu ứng tương ứng im lặng.
- Khi tích hợp PNG: ở hàm render tương ứng, gọi `ResourceManager::texture()`; nếu có texture thì vẽ sprite, nếu thiếu vẫn gọi hình khối trong `Art.cpp`. Hoặc dùng thẳng `drawImageOrPlaceholder()` với bounds và nhãn. Cache nhớ lần nạp thiếu, nên mở lại game sau khi thêm file. Chỉ chép PNG theo danh sách vào folder chưa tự thay toàn bộ renderer; cần nối sprite vào hàm vẽ tương ứng.
- Với khách, đặt tên file theo ID JSON: `student`, `college`, `auntie`, `uncle`, `courier`, `office`. Dùng ba hậu tố `normal`, `happy`, `angry`. Thêm topping/khách bằng JSON vẫn có hình khối tự động, không cần PNG mới.
- Font là tài nguyên bắt buộc để đọc UI: nếu bị xóa/hỏng, chương trình thoát có thông báo rõ (hộp thoại Windows hoặc terminal Linux/macOS), không dựng UI trống. Giải nén đầy đủ là có font ngay, không cần tự tìm/tải thêm.

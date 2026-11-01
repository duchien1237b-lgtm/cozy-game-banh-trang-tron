# Bánh Tráng Trộn Tiệm — 1.0

Game quản lý quán bánh tráng trộn bằng **C++17 / SFML 2.6.2**. Bản này đã nối đầy đủ vòng chơi, không còn các màn hình thử của từng mốc. Đồ họa dùng bộ sprite Cozy và nền đường phố riêng cho menu, màn chơi; nhạc nền do người dùng cung cấp và SFX đã kèm, không cần tải thêm tài nguyên để chơi.

## Chơi bản Windows có sẵn

Giải nén **toàn bộ** gói tải xuống, mở `GocNhoBanhTrang.exe` ở thư mục ngoài. Giữ `assets/`, `data/`, `openal32.dll` cạnh executable. Không mở trực tiếp executable từ bên trong trình xem ZIP. Không cần CMake, MinGW hoặc VS Code để chơi bản này.

Bản có sẵn dành cho Windows x64. Mã nguồn trong `MaNguon/` dành cho việc sửa và build lại; không cần chép đè lên thư mục mốc 1 hoặc 2.

## Cách chơi

- **Mở quán**: mỗi ngày dài 180 giây (3 phút). Chọn khách bằng chuột; mỗi đơn có 3–6 loại nguyên liệu và 1–3 muỗng/loại tùy khách, số ngày và topping đã mở.
- **Xé bánh**: nhấn giữ và rê tự do qua lại trên bánh. Sau 1,5 giây bánh thành dải, sau thêm 1,5 giây thành miếng nhỏ theo hai ảnh mới; thả chuột để sang thêm topping. Thả sớm hoặc rời bánh sẽ tạm dừng và giữ tiến độ; giữ yên không tăng tiến độ.
- **Thêm topping**: bấm chọn khay, nhấn giữ trên tô và thả chuột. Thanh lực chia ba mức tương ứng 1–3 muỗng; giữ tối đa 1,2 giây. Rê khỏi tô hoặc tạm dừng để hủy lượt giữ; bấm quá nhanh không thêm topping. Dòng trong tô / cần làm đối chiếu với khách đang chọn.
- **Rưới sốt**: chọn chai rồi giữ và thả trên tô theo cùng thanh lực 1–3 muỗng; mỗi lượt thả chỉ thêm một lần.
- **Trộn**: bấm Trộn món, sau đó bấm thanh hoặc nút Đánh nhịp khi vạch vào xanh. Vùng xanh đổi vị trí sau mỗi nhịp được nhận, kể cả nhịp trượt. Đủ ba nhịp thì có thể giao.
- **Giao**: kéo tô đến khách hoặc bấm Giao món. Nhận xét, điểm /100 và 1–5 sao hiện giữa màn hình rồi mờ đi sau 3,5 giây. Điểm = công thức 50%, xé 20%, trộn 20%, tốc độ 10%; thiếu, thừa và sai loại đều giảm điểm.
- **Làm lại tô**: bỏ tô hiện tại và xé tờ mới. Sau khi đã trộn không thêm được topping.
- **Cuối ngày**: xem tiền món, tip, khách đã phục vụ/bỏ về và sao; mua nâng cấp hoặc mở topping, rồi mở ngày tiếp theo.
- **ESC / Tạm dừng**: dừng cả khách và đồng hồ. Chuyển sang ứng dụng khác cũng tự tạm dừng. Menu Cài đặt có hai thanh âm lượng độc lập.

Ban đầu có bánh tráng nền và năm topping. Tám topping còn lại mua bằng tiền trong game. Có sáu kiểu khách, tám loại nâng cấp, phân trang khay và cửa hàng. Khách đến nhanh hơn và đơn phức tạp hơn theo ngày; tối đa bốn chỗ chờ khi nâng cấp.

## Tiến trình và bản lưu

Tự lưu sau mỗi món, mỗi giao dịch, thay đổi âm lượng, hết ngày và thoát bình thường. Giữ tiền, ngày, tổng khách, nâng cấp và topping đã mở. **Thoát giữa ngày giữ tiền đã kiếm, nhưng tô và hàng khách không lưu; lần sau bắt đầu lại ngày đó.** Khách còn chờ khi hết giờ được tính vào số chưa phục vụ.

- Windows: `%LOCALAPPDATA%\BanhTrangTronTiem\save.json`
- Linux: `$XDG_DATA_HOME/BanhTrangTronTiem/`, hoặc `~/.local/share/BanhTrangTronTiem/`
- macOS: `~/Library/Application Support/BanhTrangTronTiem/`

Có `save.bak`; file JSON hỏng được giữ lại với hậu tố `.corrupt-...`, rồi thử phục hồi bản dự phòng. Tiến trình không nằm trong thư mục game nên thay folder game không xóa tiền đã kiếm. Có thể sao lưu thư mục trên khi game đã đóng. Biến môi trường `TIEM_SAVE_DIR` chỉ để kiểm thử hoặc chủ động chọn nơi lưu khác.

## Cấu trúc mã nguồn

| Vị trí | Vai trò |
|---|---|
| `src/core/Game.*` | Cửa sổ, sự kiện, delta time, letterbox, vòng lặp 60 FPS |
| `src/core/StateStack.*` | Chuyển state có hàng đợi; Pause/Settings chặn state dưới |
| `src/core/ResourceManager.*` | Cache font/texture/sound; đường dẫn Unicode; fallback ảnh |
| `src/core/SaveSystem.*` | JSON tiến trình, ghi tạm, bản dự phòng, phục hồi |
| `src/core/AudioManager.*` | Nhạc lặp, âm lượng, pool hiệu ứng và vòng đời buffer |
| `src/gameplay/Model.*` | Customer, Order, Topping, Bowl, DaySession, UpgradeSystem, EconomySystem |
| `src/gameplay/Minigames.*` | PaperTearing, MixingMinigame, tính chất lượng thao tác |
| `src/states/` | MainMenu, Playing, Pause/Settings, DayResult/UpgradeShop |
| `src/ui/` | Button, ProgressBar, FloatingText, Tooltip, hạt và đồ họa hình khối |
| `data/` | Cấu hình cửa sổ, cân bằng, topping, khách, nâng cấp |
| `assets/` | Font có dấu, nhạc và SFX; chỗ dành cho PNG |
| `tests/CoreTests.cpp` | Kiểm thử tính điểm, giao dịch, save, minigame, input và vòng đời state |
| `tools/generate_audio.py` | Mã tạo lại nhạc/SFX gốc; cần Python + NumPy, không cần khi chơi/build |

## Build từ source (không cần nếu dùng executable có sẵn)

Cần CMake >= 3.16 và compiler C++17. Lần cấu hình đầu cần Internet để FetchContent tải **SFML 2.6.2** và **nlohmann/json 3.11.3** với SHA-256 cố định. Giữ toàn bộ cây source; font đã có sẵn.

### Windows — MinGW-w64

Mở thư mục source chứa `CMakeLists.txt`. Có thể chạy `build_mingw.bat`; script tự chuyển về đúng thư mục của nó, kể cả được gọi từ nơi khác. CMake, g++ và mingw32-make phải nằm trong PATH. Script dùng `build-mingw/` riêng để không nhầm cache cũ.

Hoặc mở terminal tại thư mục source và chạy:

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-mingw --parallel 4
.\build-mingw\bin\Release\BanhTrangTiem.exe
```

Không dùng lại thư mục build của project mốc cũ hoặc của compiler khác. Nếu đã di chuyển source, tạo thư mục build mới. VS Code chỉ là trình soạn thảo; dùng cùng compiler MinGW đã build được trong terminal. `build_mingw.bat` có thể gọi từ terminal của VS Code.

### Windows — Visual Studio / MSVC

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release --parallel
.\build-vs\bin\Release\BanhTrangTiem.exe
```

### Linux (Ubuntu/Debian)

```bash
sudo apt install build-essential cmake ninja-build libx11-dev libxrandr-dev libxcursor-dev libxi-dev libudev-dev libgl1-mesa-dev libfreetype6-dev libopenal-dev libflac-dev libogg-dev libvorbis-dev
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --parallel
./build-linux/bin/Release/BanhTrangTiem
```

### macOS

Cài Xcode Command Line Tools và CMake. Với Homebrew, dùng thư viện hệ thống cùng kiến trúc máy:

```bash
brew install cmake ninja freetype openal-soft libogg libvorbis flac
cmake -S . -B build-macos -G Ninja -DCMAKE_BUILD_TYPE=Release -DSFML_USE_SYSTEM_DEPS=ON -DCMAKE_PREFIX_PATH="$(brew --prefix);$(brew --prefix openal-soft)"
cmake --build build-macos --parallel
./build-macos/bin/Release/BanhTrangTiem
```

SFML/JSON vẫn do FetchContent quản lý. Bản macOS/MSVC chưa được chạy xác minh trong môi trường bàn giao này; xem `VERIFICATION.md` về phạm vi đã kiểm tra.

### Kiểm thử tự động

Thêm `-DTIEM_BUILD_TESTS=ON` khi cấu hình, build rồi chạy:

```bash
ctest --test-dir build-linux --output-on-failure
```

Với generator nhiều cấu hình thêm `-C Release`. Test font/UI cần màn hình OpenGL; trên máy Linux không có màn hình có thể dùng Xvfb. Test tạo và xóa thư mục save tạm riêng, không chạm tiến trình người chơi. Có thể bật `-DTIEM_SANITIZERS=ON` trên GCC/Clang Unix để kiểm tra mã game bằng AddressSanitizer/UBSan.

## Chỉnh dữ liệu và thêm nội dung

Sửa JSON trong `data/` cạnh executable rồi mở lại game. Nếu sửa trong source, build lại để sao chép dữ liệu sang thư mục chạy.

- `balance.json`: độ dài ngày, tần suất khách, kiên nhẫn, công thức khó dần, giá nền/tip, trọng số, thông số xé/trộn/rưới và âm lượng mặc định. Trọng số phải cộng thành 1; xé tự do có 2 giai đoạn, mỗi giai đoạn theo `paper_stage_seconds` (mặc định 1,5 giây), tối đa 4 khách, tối đa 6 loại topping/đơn. Các con số kinh tế không đặt trong UI.
- `toppings.json`: thêm phần tử có `id`, `name`, `type` (`dry`, `fresh`, `sauce`), `color` (RGB), `initial`, `unlock_price`, `unit_price`. ID phải duy nhất và ổn định. Topping mới tự xuất hiện ở cửa hàng/khay và được đưa vào đơn sau khi mở; không cần sửa C++.
- `customers.json`: thêm `id`, `name`, `style`, `color`, `patience_factor`. Dùng `student`, `auntie`, `uncle`, `courier`, `office` cho hình khối có sẵn; style khác dùng dáng cơ bản.
- `upgrades.json`: thêm ID/tên/mô tả, `effect` và các `levels` gồm `price`, `value`. Các effect hỗ trợ: `patience` (+giây), `bowl` (+tỉ lệ giá; tăng sức chứa theo số cấp), `mix` (+độ rộng vùng xanh), `paper` (+pixel bán kính nhận thao tác xé), `queue` (+chỗ), `decor`, `lantern`, `curtain` (+tỉ lệ tip). Giá từng cấp là tiền trả cho lần mua đó; value là hiệu lực tổng ở cấp hiện tại. Thêm nâng cấp dùng các effect này chỉ cần JSON; một cơ chế hiệu ứng hoàn toàn mới cần bổ sung C++.

Không đổi/xóa ID đã dùng trong save nếu muốn giữ tiến trình tương ứng. Hướng dẫn tạo/thay hình, tên file và nguồn asset nằm trong `ASSETS.md`.

## Credit và giấy phép

- SFML 2.6.2 — Laurent Gomila và các tác giả, zlib/png, `licenses/SFML.txt`, https://www.sfml-dev.org/.
- nlohmann/json 3.11.3 — Niels Lohmann và contributors, MIT, `licenses/nlohmann_json.txt`, https://github.com/nlohmann/json.
- Be Vietnam Pro Regular — The Be Vietnam Pro Project Authors (2021), SIL OFL 1.1; giữ nguyên `assets/fonts/OFL.txt`. Nguồn: https://github.com/google/fonts/tree/main/ofl/bevietnampro.
- FreeType — giấy phép FreeType, `licenses/FreeType-FTL.txt`. Portions of this software are copyright © The FreeType Project (www.freetype.org). All rights reserved.
- Ogg/Vorbis/FLAC — Xiph.Org Foundation và contributors, các thông báo trong `licenses/libogg.txt`, `libvorbis.txt`, `libflac.txt`.
- stb_image/stb_image_write — Sean Barrett và contributors, MIT/public domain; minimp3 — lieff và contributors, CC0. Xem `licenses/`.
- OpenAL Soft 1.18.1 — các tác giả OpenAL Soft, LGPL-2.0-or-later; `openal32.dll` nguyên bản đi kèm SFML. Có bản source tương ứng và giấy phép trong `licenses/`. DLL tách rời, có thể thay bằng bản tương thích; mã game không cấm sửa/relink thư viện này.
- MinGW-w64/GCC runtime — các tác giả dự án, xem `MinGW-w64.txt`, GCC Runtime Library Exception 3.1 và GPL đi kèm. SFML/GCC runtime liên kết tĩnh; OpenAL dùng DLL.
- Nhạc/SFX trong gói được tổng hợp riêng bằng `tools/generate_audio.py`; không lấy nhạc thương mại hoặc sample bên ngoài. Đồ họa hình khối nằm trong `Art.cpp`. Mã game và các nội dung tự tạo trong gói được cấp MIT tại `LICENSE.txt`; giấy phép thư viện/font giữ riêng.

Nếu thay bằng asset từ Freesound/Kenney/OpenGameArt/Pixabay hoặc nguồn khác, kiểm tra giấy phép từng file và bổ sung tác giả, URL, license, ngày tải, chỉnh sửa vào bảng credit của bạn.

## Cửa sổ đánh giá sau trải nghiệm

Nút **Đánh giá khách** ở menu mở cửa sổ đọc review. Sau mỗi món được phục vụ, game tạo một bài nhận xét mô phỏng trải nghiệm khi khách về nhà, với tên tài khoản riêng trong nhóm 32 tên và nhiều biến thể cảm xúc. Bài viết dựa trên công thức, độ trộn đều, bánh và thời gian chờ. Câu đánh giá ngắn giữa màn chơi được giữ riêng.

- Xem điểm trung bình, số lượng từng mức sao, bài mới nhất và lọc **Chưa phản hồi**. Mỗi trang có hai bài, có thể lật trang hoặc cuộn chuột.
- Bấm **Xem & phản hồi** để đọc đầy đủ và viết trả lời; có ba gợi ý cảm ơn, xin lỗi và ghi nhận. Có thể chỉnh sửa phản hồi đã lưu.
- Phản hồi tối đa 240 ký tự; hỗ trợ dấu tiếng Việt, Ctrl+A/C/V, Backspace, Enter để lưu, Esc để quay lại.
- Review và phản hồi nằm trong bản lưu hiện có. Bản lưu cũ vẫn mở được; các bài mới bắt đầu xuất hiện từ những lượt phục vụ tiếp theo. Giữ tối đa 100 bài gần nhất.

Dây cờ được vẽ phía sau rèm, phần giao nhau bị rèm che đúng lớp.

## Nhạc nền

Game phát `assets/audio/music/menu.mp3` ở menu, tổng kết và cửa hàng; `playing.mp3` phát trong màn chơi. Nhạc tự lặp và âm lượng chỉnh trong Cài đặt. Hai file được giữ nguyên từ file người dùng cung cấp; bản WAV tổng hợp cũ vẫn nằm trong thư mục tài nguyên.

## Hiệu ứng âm thanh

Game dùng bộ âm thanh trong `sound-effect.zip`: chọn topping, xé bánh, nhịp trộn Perfect/Good/Miss, giao món, mua nâng cấp và chọn nút. Tiếng người nền phát lặp ở mức nhẹ trong màn chơi và dừng khi rời màn. Thanh **Hiệu ứng** trong Cài đặt điều chỉnh cả âm thanh thao tác và tiếng nền; giảm về 0 để tắt.

## Hiệu ứng thêm nguyên liệu (add-effect.zip)

Khay nguyên liệu bật nhẹ khi chọn. Khi thả thanh lực, mức 1/2/3 tạo lần lượt 2/4/6 icon rơi lần lượt vào tô, mỗi icon phát một tiếng `pop.mp3`. Số icon tính theo lượng thực sự thêm được khi tô gần đầy. Đợi hiệu ứng kết thúc trước khi thêm tiếp hoặc trộn; làm lại tô sẽ hủy các icon còn chờ.

Tiếng người nền mới được tách từ `people-background-noise.mp4` trong ZIP thành MP3, phát lặp ở 35% thanh Hiệu ứng (trước đây 25%). Hai tài nguyên được đồng bộ trong assets của bản chạy và mã nguồn.

## Nâng cấp cửa hàng hiển thị rõ tác dụng

Mỗi thẻ nâng cấp hiển thị chỉ số đang dùng và mốc tiếp theo: thời gian chờ, sức chứa/giá món, vùng xanh, số chỗ và tip. Trong màn chơi, thớt và muỗng xuất hiện khi đã mua, tô lớn dần theo cấp (vùng bấm tăng tương ứng), ghế chờ hiện cạnh khách và nhãn tên ghi thời gian chờ cộng thêm. Cờ vẫn nằm sau rèm.

Thớt mốc 1 mở rộng vùng rê, giữ 1,50 giây/giai đoạn. Mốc 2 giảm còn 1,25 giây/giai đoạn; mốc 3 còn 0,75 giây/giai đoạn. Tổng hai giai đoạn tương ứng 3 / 2,5 / 1,5 giây. Các mốc sau giữ vùng rê rộng của mốc 1. Tiến trình cũ dùng cấp đã mua nên tự nhận tác dụng mới, không phải mua lại.

## Logo và icon Góc Nhỏ Bánh Tráng

Menu dùng logo từ bộ ảnh do người dùng cung cấp. Icon PNG được dùng cho cửa sổ game; icon ICO nhiều kích cỡ được nhúng vào file chạy Windows. Tên cửa sổ là Góc Nhỏ Bánh Tráng.

## Khách hàng và âm thanh bổ sung

Bổ sung Bé Mai, Bà Tư, Anh Khang, Anh Bảo và khách mời Chị Dung, mỗi người có ba biểu cảm. Chị Dung có tên và viền vàng, lời chào riêng, trọng số xuất hiện 0,35 so với 1 của mỗi khách thường (khoảng 3,4% mỗi lượt sinh khách). Tổng cộng 11 nhân vật trong cửa hàng.

Âm thanh perfect-mixed phát sau khi giao món đạt ít nhất 4 sao; alter_customer_happy phát khi trộn Perfect; female_customer_leave/male_customer_leave phát đúng giọng khách hết kiên nhẫn; khi thêm sốt vẫn phát pop theo từng icon; day_end phát lúc chuyển sang tổng kết ngày.

## Lượng topping theo khách và tip khách đặc biệt

Mỗi khách có khẩu phần khởi đầu riêng: nhóm nhỏ 3 muỗng, nhóm vừa 4 muỗng, nhóm lớn 5 muỗng; tăng dần theo ngày và tối đa 65%. Chỉ gọi nguyên liệu đã mở.

Mỗi lượt giao món hợp lệ cho khách đặc biệt được tip đúng 100% giá món ghi trên đơn, không phụ thuộc điểm đánh giá và không cộng chồng thêm thưởng trang trí. Ví dụ đơn 30.000đ luôn có tip 30.000đ. Tiền món cơ bản vẫn chấm theo chất lượng. Khách bỏ đi không có tip; cùng một đơn không thể nhận tip hai lần.

## Thanh lực chậm hơn và âm thanh nạp

Thanh lực thêm topping đầy sau 2,4 giây thay vì 1,2 giây. Dùng charging_sound.mp3 do người dùng cung cấp khi nhấn giữ trên tô. Căn tốc độ phát theo thời gian nạp, âm lượng bằng 85% thanh Hiệu ứng; một lượt chỉ có một âm thanh. Dừng khi thả, rời tô/cửa sổ, tạm dừng, đầy thanh hoặc rời màn chơi. Giữ đầy lực không tự thêm topping.

## Khung giao diện màn chơi mới

Dùng top_panel, chat_bubble, customer_name và customer_name_unselected từ Khung_UI_Goc_Nho_Banh_Trang.zip. Khung được co giãn theo từng phần để giữ đường viền và góc. Ngày nằm giữa khung đầu màn, thời gian bên trái, nút tạm dừng bên phải; ví nằm bên trái mép nâu của quầy.

Ẩn tên game và hướng dẫn dưới tên game ở đầu màn, các tiêu đề nguyên liệu/công đoạn, dòng trạng thái giai đoạn xé, cấp thớt/muỗng, thời gian chờ cộng thêm trên nhãn tên và toàn bộ thanh hướng dẫn ở chân màn. Giãn hai hàng tô nguyên liệu, hạ thớt/tô trộn/muỗng và các thanh tiến độ cho phù hợp khoảng trống mới. Giữ hiển thị đơn hàng, thanh kiên nhẫn, số muỗng và phản hồi giao món.

Nhịp trộn Perfect và Good dùng hai file mới do người dùng cung cấp; Miss dùng âm thanh trước đây của Perfect (`alter_customer_happy.mp3`). Tiếng khách vui vẫn dùng `perfect-mixed.mp3` cũ.

## Bố cục nút menu và bắt đầu lại

Nút đầu là Tiếp tục mở quán ngày N. Chơi lại từ đầu nằm ở vị trí nút đánh giá cũ; Đánh giá khách chuyển xuống vị trí Cách chơi cũ. Cách chơi nằm trong Cài đặt, có thể mở cả từ menu lẫn khi tạm dừng.

Chơi lại từ đầu mở hộp xác nhận; Hủy hoặc ESC giữ tiến trình. Xác nhận lưu lượt mới và mở ngày 1: tiền, nâng cấp, nguyên liệu mở khóa, thành tích và review về trạng thái ban đầu, giữ âm lượng đã chọn. Nếu lưu thất bại thì giữ nguyên tiến trình đang có.

Ánh xạ trộn hiện tại: Perfect → alter_customer_happy.mp3; Miss → good-mixed-20261030.mp3; Good → perfect-mixed-20261030.mp3.

Âm thanh trộn hiện dùng đúng ba file mới của người dùng, lưu trong assets/audio/sfx/sound-effect/mix/: Miss → miss-mixed.mp3; Good → good-mixed.mp3; Perfect → perfect-mixed.mp3.

## Độ khó dễ hơn và giới hạn tăng 65%

Hệ số ngày = 1 + min(0,65; 0,05 × (ngày − 1)): ngày đầu 100%, ngày 14 đạt 165% rồi giữ nguyên. Thời gian chờ cơ bản là 84 giây chia hệ số này, sau đó áp dụng nâng cấp ghế và đặc điểm từng khách. Vì vậy ở mức khó nhất, thời gian chờ cơ bản còn khoảng 50,9 giây, không tiếp tục giảm ở ngày cao hơn.

Ngày đầu mỗi đơn gồm 3–5 loại tùy khách, mỗi loại 1 muỗng. Tổng số muỗng tăng theo hệ số ngày, làm tròn xuống và không vượt 165% khẩu phần ngày đầu; thêm loại hoặc thêm muỗng đều dùng chung giới hạn này. Chỉ yêu cầu topping đã mở, giữ giới hạn loại/số muỗng riêng từng khách và sức chứa tô. Tần suất khách và tốc độ trộn cũng dùng cùng hệ số có trần; khách đến thưa hơn với khoảng cách cơ bản 22 giây ngày đầu. Thời lượng ngày vẫn 3 phút, tip Chị Dung vẫn 100% giá trên đơn.

## Cấu trúc sau khi dọn thư mục

Bản chơi nằm ở GocNhoBanhTrang.exe cùng assets/, data/ và openal32.dll. Mã nguồn, CMakeLists.txt, script build, tests và công cụ preview vẫn nằm trong MaNguon/. Giữ giấy phép và thông tin nguồn tài nguyên trong licenses/, assets/fonts/ và MaNguon/ASSETS.md. Lịch sử Git được giữ để tiếp tục phát triển.

Đã xóa công cụ tải tạm .tools/, thư mục build-mingw/, các EXE dự phòng, ảnh kiểm tra previews/ và tài nguyên không được code hiện tại tham chiếu. Muốn build lại, cài CMake và compiler C++17 trong PATH rồi dùng hướng dẫn phía trên; lần cấu hình đầu cần tải lại các thư viện qua FetchContent. Bản chạy có sẵn hoạt động độc lập với các công cụ này.

## Khôi phục giọng khách trên bản cập nhật

Đã chèn lại bốn giọng khách vào code của bản game mới: An, Minh và Bé Mai dùng childen_happy.mp3; Cô Hạnh, Chị Linh, Bà Tư và Chị Dung dùng female_happy.mp3; Anh shipper, Anh Khang và Anh Bảo dùng man_happy.mp3; Chú Bình dùng oldman_happy.mp3. Ưu tiên trẻ em trước giới tính; Bà Tư vẫn dùng giọng nữ. Mỗi sự kiện phát trễ giữ riêng giọng của đúng khách đã phục vụ. File âm thanh chung cũ đã được xóa.

## Icon Quà hằng ngày và Thành tựu ở hai góc menu

Quà hằng ngày dùng icon hộp quà ở góc trên trái; Thành tựu dùng icon cúp vàng ở góc trên phải. Hai nút hàng dưới cũ đã được bỏ. Icon có nền tròn, nhãn tên, trạng thái hover/nhấn; dấu báo quà chưa nhận theo sát icon trái và bộ đếm thành tựu nằm dưới cúp. PNG và prompt imagegen tích hợp được ghi tại assets/images/cozy/ui/menu_icons.md. Mã xử lý vẫn mở đúng cửa sổ Quà hằng ngày/Thành tựu hiện có.

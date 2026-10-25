# Bánh Tráng Trộn Tiệm — 1.0

Game quản lý quán bánh tráng trộn bằng **C++17 / SFML 2.6.2**. Bản này đã nối đầy đủ vòng chơi, không còn các màn hình thử của từng mốc. Đồ họa pastel tự vẽ bằng hình khối; nhạc và SFX tổng hợp đã kèm, không cần tải thêm tài nguyên để chơi.

## Chơi bản Windows có sẵn

Giải nén **toàn bộ** gói tải xuống, mở `BanhTrangTiem.exe` ở thư mục ngoài. Giữ `assets/`, `data/`, `openal32.dll` cạnh executable. Không mở trực tiếp executable từ bên trong trình xem ZIP. Không cần CMake, MinGW hoặc VS Code để chơi bản này.

Bản có sẵn dành cho Windows x64. Mã nguồn trong `MaNguon/` dành cho việc sửa và build lại; không cần chép đè lên thư mục mốc 1 hoặc 2.

## Cách chơi

- **Mở quán**: mỗi ngày dài 120 giây. Chọn khách bằng chuột; công thức hiện icon, tên và số muỗng.
- **Xé bánh**: giữ chuột tại một chấm xanh, vuốt hết đường nét đứt rồi thả. Làm đủ ba đường. Có thể xé theo cả hai chiều. Bám đường và đều tay để được Perfect.
- **Thêm topping**: kéo từ khay vào tô. Mỗi lượt thả được một muỗng; thả ngoài để hủy. Dòng `trong tô / cần làm` đối chiếu với khách đang chọn.
- **Rưới sốt**: kéo chai vào tô và giữ. Mỗi 0,6 giây thêm một muỗng. Thả chuột để ngừng; thanh nhỏ dưới chai báo tiến độ muỗng tiếp theo.
- **Trộn**: bấm Trộn món hoặc click tô, sau đó bấm thanh/nút khi vạch vào vùng xanh. Đủ ba nhịp thì có thể giao. Vùng giữa cho Perfect, vùng xanh còn lại Good, bên ngoài Miss.
- **Giao**: kéo tô đến khách bất kỳ hoặc bấm Giao món cho khách đang chọn. Điểm = công thức 50%, xé 20%, trộn 20%, tốc độ 10%. Thiếu, thừa và sai loại đều giảm điểm.
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

- `balance.json`: độ dài ngày, tần suất khách, kiên nhẫn, công thức khó dần, giá nền/tip, trọng số, thông số xé/trộn/rưới và âm lượng mặc định. Trọng số phải cộng thành 1; giao diện hỗ trợ 3 đường xé, tối đa 4 khách, tối đa 4 loại topping/đơn. Các con số kinh tế không đặt trong UI.
- `toppings.json`: thêm phần tử có `id`, `name`, `type` (`dry`, `fresh`, `sauce`), `color` (RGB), `initial`, `unlock_price`, `unit_price`. ID phải duy nhất và ổn định. Topping mới tự xuất hiện ở cửa hàng/khay và được đưa vào đơn sau khi mở; không cần sửa C++.
- `customers.json`: thêm `id`, `name`, `style`, `color`, `patience_factor`. Dùng `student`, `auntie`, `uncle`, `courier`, `office` cho hình khối có sẵn; style khác dùng dáng cơ bản.
- `upgrades.json`: thêm ID/tên/mô tả, `effect` và các `levels` gồm `price`, `value`. Các effect hỗ trợ: `patience` (+giây), `bowl` (+tỉ lệ giá; tăng sức chứa theo số cấp), `mix` (+độ rộng vùng xanh), `paper` (+pixel dung sai), `queue` (+chỗ), `decor`, `lantern`, `curtain` (+tỉ lệ tip). Giá từng cấp là tiền trả cho lần mua đó; value là hiệu lực tổng ở cấp hiện tại. Thêm nâng cấp dùng các effect này chỉ cần JSON; một cơ chế hiệu ứng hoàn toàn mới cần bổ sung C++.

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

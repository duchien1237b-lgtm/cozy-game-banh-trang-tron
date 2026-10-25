#include "core/SaveSystem.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
namespace tiem {
std::filesystem::path SaveSystem::defaultFolder() {
    if (const char *test = std::getenv("TIEM_SAVE_DIR"))
        return std::filesystem::u8path(test);
#ifdef _WIN32
    if (const wchar_t *app = _wgetenv(L"LOCALAPPDATA"))
        return std::filesystem::path(app) / L"BanhTrangTronTiem";
#elif defined(__APPLE__)
    if (const char *home = std::getenv("HOME"))
        return std::filesystem::path(home) / "Library/Application Support/BanhTrangTronTiem";
#else
    if (const char *app = std::getenv("XDG_DATA_HOME"))
        return std::filesystem::path(app) / "BanhTrangTronTiem";
    if (const char *home = std::getenv("HOME"))
        return std::filesystem::path(home) / ".local/share/BanhTrangTronTiem";
#endif
    return std::filesystem::current_path() / "saves";
}
SaveSystem::SaveSystem(std::filesystem::path folder) : folder_(std::move(folder)) {}
Progress SaveSystem::load(const GameData &d) {
    auto p = Progress::fresh(d);
    for (const auto &name : {"save.json", "save.bak"}) {
        const auto path = folder_ / name;
        if (!std::filesystem::exists(path))
            continue;
        try {
            std::ifstream stream(path);
            nlohmann::json j;
            stream >> j;
            if (j.at("version").get<int>() != 1)
                throw std::runtime_error("Phien ban save khong duoc ho tro");
            p.day = std::clamp(j.at("day").get<int>(), 1, 10000);
            p.money = std::clamp(j.at("money").get<int>(), 0, 1000000000);
            p.lifetime = std::clamp(j.value("lifetime", 0), 0, 1000000000);
            p.served = std::clamp(j.value("served", 0), 0, 10000000);
            p.music = std::clamp(j.value("music", 32.f), 0.f, 100.f);
            p.sfx = std::clamp(j.value("sfx", 60.f), 0.f, 100.f);
            p.bestRating = std::clamp(j.value("best_rating", 0.f), 0.f, 5.f);
            p.levels.clear();
            p.unlocked.clear();
            const auto unlocks = j.at("unlocked").get<std::set<std::string>>();
            const auto levels = j.at("upgrades").get<std::map<std::string, int>>();
            for (const auto &t : d.toppings)
                if (t.initial || unlocks.count(t.id))
                    p.unlocked.insert(t.id);
            for (const auto &u : d.upgrades) {
                const auto it = levels.find(u.id);
                if (it != levels.end())
                    p.levels[u.id] = std::clamp(it->second, 0, static_cast<int>(u.levels.size()));
            }
            error_ = std::string(name) == "save.bak" ? "Đã phục hồi bản lưu dự phòng." : "";
            return p;
        } catch (const std::exception &) {
            error_ = "Bản lưu hỏng đã được giữ lại. Đang dùng dữ liệu dự phòng.";
            std::error_code ec;
            const auto stamp = std::chrono::system_clock::now().time_since_epoch().count();
            std::filesystem::rename(path, folder_ / (std::string(name) + ".corrupt-" + std::to_string(stamp)),
                                    ec);
        }
    }
    return Progress::fresh(d);
}
bool SaveSystem::write(const Progress &p) {
    const auto file = folder_ / "save.json", backup = folder_ / "save.bak", temp = folder_ / "save.tmp";
    try {
        std::filesystem::create_directories(folder_);
        nlohmann::json j = {{"version", 1},           {"day", p.day},       {"money", p.money},
                            {"lifetime", p.lifetime}, {"served", p.served}, {"best_rating", p.bestRating},
                            {"music", p.music},       {"sfx", p.sfx},       {"unlocked", p.unlocked},
                            {"upgrades", p.levels}};
        {
            std::ofstream stream(temp, std::ios::trunc);
            if (!stream)
                throw std::runtime_error("Khong mo duoc file tam");
            stream << j.dump(2);
            stream.flush();
            if (!stream)
                throw std::runtime_error("Ghi file that bai");
        }
        // Ghi hoàn tất file tạm trước, giữ bản cũ để phục hồi nếu bị ngắt giữa chừng.
        if (std::filesystem::exists(file)) {
            std::filesystem::remove(backup);
            std::filesystem::rename(file, backup);
        }
        std::filesystem::rename(temp, file);
        error_.clear();
        return true;
    } catch (const std::exception &) {
        error_ = "Không lưu được tiến trình. Kiểm tra quyền ghi hoặc dung lượng ổ đĩa.";
        return false;
    }
}
} // namespace tiem

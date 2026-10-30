#include "core/SaveSystem.hpp"
#include "core/Utf8.hpp"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>
namespace tiem {
namespace {
bool boundedReviewText(const std::string &value, std::size_t limit) {
    if (value.empty() || value.size() > limit * 4) return false;
    const auto characters = utf8(value);
    if (characters.getSize() > limit) return false;
    bool visible = false;
    for (auto codepoint : characters) {
        if (codepoint < 32 || codepoint == 127) return false;
        visible = visible || (codepoint != ' ' && codepoint != 0xa0 && codepoint != 0x3000 &&
                              !(codepoint >= 0x2000 && codepoint <= 0x200b));
    }
    return visible;
}
std::uint64_t reviewId(const nlohmann::json &value) {
    if (!value.is_number_integer() || (!value.is_number_unsigned() && value.get<std::int64_t>() <= 0))
        return 0;
    const auto id = value.get<std::uint64_t>();
    return id < std::numeric_limits<std::uint64_t>::max() - 1000 ? id : 0;
}
void loadReviews(const nlohmann::json &json, Progress &progress) {
    progress.reviews.clear();
    progress.nextReviewId = 1;
    if (const auto counter = json.find("next_review_id"); counter != json.end())
        progress.nextReviewId = std::max<std::uint64_t>(1, reviewId(*counter));
    const auto entries = json.find("reviews");
    if (entries == json.end() || !entries->is_array()) return;
    std::set<std::uint64_t> seen;
    // Keep the latest valid entries. Bad optional records must not discard a player's save.
    for (auto it = entries->rbegin(); it != entries->rend() && progress.reviews.size() < 100; ++it) {
        try {
            if (!it->is_object()) continue;
            ShopReview review;
            review.id = reviewId(it->at("id"));
            review.author = it->at("author").get<std::string>();
            review.text = it->at("text").get<std::string>();
            if (!it->at("day").is_number_integer() || !it->at("stars").is_number_integer()) continue;
            const auto day = it->at("day").get<std::int64_t>();
            const auto stars = it->at("stars").get<std::int64_t>();
            if (!review.id || !boundedReviewText(review.author, 80) || !boundedReviewText(review.text, 600) ||
                day < 1 || day > 10000 || stars < 1 || stars > 5 || !seen.insert(review.id).second) continue;
            review.day = static_cast<int>(day);
            review.stars = static_cast<int>(stars);
            progress.reviews.push_back(review);
            progress.nextReviewId = std::max(progress.nextReviewId, review.id + 1);
            const auto reply = it->find("reply");
            if (reply != it->end() && reply->is_string())
                ReviewSystem::reply(progress, review.id, reply->get<std::string>());
        } catch (const std::exception &) {
            // A missing field or wrong type affects this review only.
        }
    }
    std::reverse(progress.reviews.begin(), progress.reviews.end());
}
} // namespace
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
            p = Progress::fresh(d);
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
            loadReviews(j, p);
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
        j["reviews"] = nlohmann::json::array();
        const std::size_t first = p.reviews.size() > 100 ? p.reviews.size() - 100 : 0;
        for (std::size_t i = first; i < p.reviews.size(); ++i) {
            const auto &review = p.reviews[i];
            j["reviews"].push_back({{"id", review.id}, {"author", review.author}, {"text", review.text},
                                     {"reply", review.reply}, {"day", review.day}, {"stars", review.stars}});
        }
        j["next_review_id"] = p.nextReviewId;
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

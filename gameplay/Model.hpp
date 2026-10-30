#pragma once
#include <SFML/Graphics/Color.hpp>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <random>
#include <set>
#include <string>
#include <vector>
namespace tiem {
struct Topping {
    std::string id, name, type;
    sf::Color color;
    int unlockPrice{}, unitPrice{};
    bool initial{};
};
struct CustomerKind {
    std::string id, name, style;
    sf::Color color;
    float patienceFactor{};
    bool femaleVoice{false}, special{false};
    float spawnWeight{1.f};
    int minTypes{4}, maxTypes{6}, minQuantity{1}, maxQuantity{3};
    bool child{false}, olderAdult{false};
    std::string happySound() const {
        if (child) return "customer_happy_child";
        if (femaleVoice) return "customer_happy_female";
        return olderAdult ? "customer_happy_oldman" : "customer_happy_man";
    }
};
struct UpgradeLevel {
    int price{};
    float value{};
};
struct Upgrade {
    std::string id, name, description, effect;
    std::vector<UpgradeLevel> levels;
};
struct GameData {
    nlohmann::json balance;
    std::vector<Topping> toppings;
    std::vector<CustomerKind> customers;
    std::vector<Upgrade> upgrades;
    explicit GameData(const std::filesystem::path &root);
    float number(const std::string &key) const {
        return balance.at(key).get<float>();
    }
    int integer(const std::string &key) const {
        return balance.at(key).get<int>();
    }
    const Topping &topping(const std::string &id) const;
    // Số liệu tùy chọn: bản data cũ thiếu khóa vẫn chạy với giá trị mặc định.
    float numberOr(const std::string &key, float fallback) const {
        const auto it = balance.find(key);
        return it != balance.end() && it->is_number() ? it->get<float>() : fallback;
    }
};
struct ShopReview {
    std::uint64_t id{};
    std::string author, text, reply;
    int day{1}, stars{1};
};
float difficultyMultiplier(const GameData &, int day);
struct Progress {
    int day{1}, money{}, lifetime{}, served{};
    float bestRating{}, music{}, sfx{};
    // Thống kê dài hạn cho thành tựu và quà hằng ngày (tất cả có mặc định để đọc được bản lưu cũ).
    int fiveStars{}, perfectMixes{}, bestCombo{}, rushServed{}, goalsDone{}, perfectDays{}, dailyStreak{};
    std::int64_t lastDaily{-1};
    bool tutorialDone{false}, fullscreen{false};
    std::set<std::string> achievements;
    std::set<std::string> unlocked;
    std::map<std::string, int> levels;
    std::vector<ShopReview> reviews;
    std::uint64_t nextReviewId{1};
    static Progress fresh(const GameData &data);
};
struct Order {
    std::map<std::string, int> recipe;
    int price{};
};
struct Customer {
    unsigned id{};
    std::size_t kind{};
    Order order;
    float patience{}, remaining{}, arrival{};
};
struct Bowl {
    std::vector<std::string> portions;
    float paperQuality{}, mixQuality{};
    int perfectHits{};
    bool mixed{false};
    bool add(const std::string &id, int capacity);
    // Bỏ muỗng cuối cùng (chỉ trước khi trộn). Trả về id vừa bỏ.
    bool removeLast(std::string *removed = nullptr);
    int count(const std::string &id) const;
    void clear() {
        portions.clear();
        paperQuality = mixQuality = 0;
        perfectHits = 0;
        mixed = false;
    }
};
struct Receipt {
    float recipe{}, paper{}, mix{}, speed{}, score{};
    int base{}, tip{};
    // comboBonus và rushBonus đã nằm trong tip; tách riêng để UI hiển thị.
    int comboBonus{}, rushBonus{}, combo{};
    bool rush{false};
    int stars() const;
    const char *review() const;
};
struct DayGoal {
    std::string id, text;
    int target{}, progress{}, reward{};
    bool done() const { return progress >= target; }
};
struct AchievementInfo {
    std::string id, name, description;
    int target{};
};
class AchievementSystem {
  public:
    static const std::vector<AchievementInfo> &all();
    static int current(const GameData &, const Progress &, const std::string &id);
    // Mở khóa mọi thành tựu đã đủ điều kiện; trả về danh sách vừa mở.
    static std::vector<std::string> check(const GameData &, Progress &);
};
class DailyReward {
  public:
    // Số ngày theo giờ địa phương (tính độc lập múi giờ, thuận tiện cho kiểm thử).
    static std::int64_t today();
    static bool canClaim(const Progress &, std::int64_t today);
    // Mốc (0..6) sẽ nhận nếu bấm nhận hôm nay.
    static int nextIndex(const Progress &, std::int64_t today);
    static int amount(const GameData &, int index);
    // Cộng tiền, cập nhật chuỗi và lưu thành tựu; trả 0 nếu đã nhận hôm nay.
    static int claim(const GameData &, Progress &, std::int64_t today);
};
class ReviewSystem {
  public:
    static void record(Progress &, const Receipt &, int day);
    static bool reply(Progress &, std::uint64_t id, const std::string &text);
};
struct DayResult {
    int day{1}, revenue{}, tips{}, served{}, left{};
    float scoreSum{};
    int bestCombo{}, comboBonus{}, rushBonus{}, rushServed{}, fiveStars{}, perfectMixes{}, fastServed{};
    int goalBonus{}, goalsDone{};
    bool perfectDay{false};
    std::vector<DayGoal> goals;
    std::vector<std::string> newAchievements;
    float rating() const;
};
class UpgradeSystem {
  public:
    static float effect(const GameData &, const Progress &, const std::string &effect);
    static int level(const Progress &, const std::string &id);
    static bool buy(const GameData &, Progress &, std::size_t index);
    static bool unlock(const GameData &, Progress &, std::size_t index);
};
class EconomySystem {
  public:
    static Receipt score(const GameData &, const Progress &, const Customer &, const Bowl &);
};
class DaySession {
  public:
    DaySession(const GameData &data, Progress &progress, unsigned seed);
    void update(float dt);
    Receipt serve(std::size_t index, const Bowl &bowl);
    void finish();
    bool ended() const {
        return ended_;
    }
    float remaining() const {
        return remaining_;
    }
    int capacity() const;
    int combo() const { return combo_; }
    bool rush() const { return rushActive_; }
    float rushRemaining() const { return rushActive_ ? std::max(0.f, remaining_ - rushEnd_) : 0.f; }
    bool rushDone() const { return rushFinished_; }
    const std::vector<DayGoal> &goals() const { return result_.goals; }
    // Thành tựu mới mở kể từ lần gọi trước (UI dùng để hiện thông báo).
    std::vector<std::string> takeUnlocked() { return std::move(unlockedQueue_); }
    static std::vector<DayGoal> makeGoals(const GameData &, int day);
    std::vector<Customer> &customers() {
        return customers_;
    }
    const DayResult &result() const {
        return result_;
    }

  private:
    void spawn();
    const GameData &data_;
    Progress &progress_;
    std::mt19937 random_;
    std::mt19937 reviewRandom_;
    std::vector<Customer> customers_;
    DayResult result_;
    float remaining_{}, spawnTimer_{};
    unsigned nextId_{1};
    bool ended_{false};
    int combo_{};
    float rushBegin_{}, rushEnd_{};
    bool rushActive_{false}, rushFinished_{false};
    std::vector<std::string> unlockedQueue_;
};
std::string moneyText(int amount);
// Văn bản mô tả nhiệm vụ ngày: đầy đủ (màn chuẩn bị/tổng kết) và gọn (HUD).
std::string goalText(const DayGoal &);
std::string goalShort(const DayGoal &);
std::string goalProgressText(const DayGoal &);
} // namespace tiem

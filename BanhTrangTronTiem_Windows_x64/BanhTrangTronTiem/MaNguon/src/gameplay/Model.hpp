#pragma once
#include <SFML/Graphics/Color.hpp>
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
};
struct Progress {
    int day{1}, money{}, lifetime{}, served{};
    float bestRating{}, music{}, sfx{};
    std::set<std::string> unlocked;
    std::map<std::string, int> levels;
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
    bool mixed{false};
    bool add(const std::string &id, int capacity);
    int count(const std::string &id) const;
    void clear() {
        portions.clear();
        paperQuality = mixQuality = 0;
        mixed = false;
    }
};
struct Receipt {
    float recipe{}, paper{}, mix{}, speed{}, score{};
    int base{}, tip{};
};
struct DayResult {
    int day{1}, revenue{}, tips{}, served{}, left{};
    float scoreSum{};
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
    std::vector<Customer> customers_;
    DayResult result_;
    float remaining_{}, spawnTimer_{};
    unsigned nextId_{1};
    bool ended_{false};
};
std::string moneyText(int amount);
} // namespace tiem

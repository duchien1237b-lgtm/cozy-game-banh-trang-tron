#include "gameplay/Model.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
namespace tiem {
namespace {
nlohmann::json read(const std::filesystem::path &path) {
    std::ifstream stream(path);
    if (!stream)
        throw std::runtime_error("Khong mo duoc " + path.u8string());
    nlohmann::json j;
    stream >> j;
    return j;
}
sf::Color color(const nlohmann::json &j) {
    if (!j.is_array() || j.size() != 3)
        throw std::runtime_error("Mau can 3 gia tri RGB");
    for (const auto &v : j)
        if (!v.is_number_integer() || v.get<int>() < 0 || v.get<int>() > 255)
            throw std::runtime_error("RGB ngoai khoang 0..255");
    return {j[0].get<sf::Uint8>(), j[1].get<sf::Uint8>(), j[2].get<sf::Uint8>()};
}
} // namespace
GameData::GameData(const std::filesystem::path &root) {
    balance = read(root / "data/balance.json");
    std::set<std::string> ids;
    const auto toppingData = read(root / "data/toppings.json");
    for (const auto &j : toppingData.at("toppings")) {
        Topping t{j.at("id"),           j.at("name"),       j.at("type"),   color(j.at("color")),
                  j.at("unlock_price"), j.at("unit_price"), j.at("initial")};
        if (t.id.empty() || t.name.empty() || !ids.insert(t.id).second || t.unlockPrice < 0 ||
            t.unitPrice < 0 || (t.type != "dry" && t.type != "fresh" && t.type != "sauce"))
            throw std::runtime_error("Topping khong hop le");
        toppings.push_back(t);
    }
    ids.clear();
    const auto customerData = read(root / "data/customers.json");
    for (const auto &j : customerData.at("customers")) {
        CustomerKind k{j.at("id"), j.at("name"), j.at("style"), color(j.at("color")),
                       j.at("patience_factor")};
        if (k.id.empty() || !ids.insert(k.id).second || k.patienceFactor <= 0)
            throw std::runtime_error("Khach khong hop le");
        customers.push_back(k);
    }
    ids.clear();
    const auto upgradeData = read(root / "data/upgrades.json");
    for (const auto &j : upgradeData.at("upgrades")) {
        Upgrade u{j.at("id"), j.at("name"), j.at("description"), j.at("effect"), {}};
        if (u.id.empty() || !ids.insert(u.id).second)
            throw std::runtime_error("Trung ID nang cap");
        for (const auto &l : j.at("levels")) {
            UpgradeLevel level{l.at("price"), l.at("value")};
            if (level.price < 0 || level.value < 0)
                throw std::runtime_error("Nang cap am");
            u.levels.push_back(level);
        }
        upgrades.push_back(u);
    }
    if (toppings.empty() || customers.empty())
        throw std::runtime_error("Du lieu rong");
    if (std::count_if(toppings.begin(), toppings.end(), [](const auto &t) { return t.initial; }) < 2)
        throw std::runtime_error("Can it nhat 2 topping ban dau");
    // Các số liệu gameplay phải hữu hạn; lỗi dữ liệu dừng ngay thay vì hỏng giữa ngày.
    for (auto it = balance.begin(); it != balance.end(); ++it)
        if (it.value().is_number() &&
            (!std::isfinite(it.value().get<float>()) || it.value().get<float>() < 0))
            throw std::runtime_error("Thong so can la so khong am: " + it.key());
    for (const auto &key : {"day_seconds", "spawn_min_seconds", "patience_min_seconds", "paper_tolerance",
                            "paper_ideal_seconds", "sauce_seconds_per_spoon", "mix_hit_cooldown"})
        if (number(key) <= 0)
            throw std::runtime_error("Thong so phai lon hon 0");
    if (integer("paper_lines") != 3 || integer("paper_samples") < 4 || integer("paper_samples") > 100 ||
        integer("mix_hits") < 1 || integer("mix_hits") > 10 || integer("queue_slots") < 1 ||
        integer("queue_max") > 4 || integer("queue_slots") > integer("queue_max") ||
        integer("order_types_every_days") < 1 || integer("bowl_capacity") < 8 ||
        integer("order_initial_types") < 1 || integer("order_max_types") > 4 ||
        integer("order_max_quantity") < 1 || integer("order_max_quantity") > 5)
        throw std::runtime_error("Thong so vuot gioi han giao dien");
    float total = 0;
    for (const auto &key : {"recipe", "paper", "mix", "speed"})
        total += balance.at("weights").at(key).get<float>();
    if (std::abs(total - 1.f) > .001f)
        throw std::runtime_error("Tong trong so phai bang 1");
}
const Topping &GameData::topping(const std::string &id) const {
    const auto it = std::find_if(toppings.begin(), toppings.end(), [&](const auto &t) { return t.id == id; });
    if (it == toppings.end())
        throw std::runtime_error("ID topping khong ton tai: " + id);
    return *it;
}
Progress Progress::fresh(const GameData &d) {
    Progress p;
    p.money = d.integer("initial_money");
    p.music = d.number("default_music");
    p.sfx = d.number("default_sfx");
    for (const auto &t : d.toppings)
        if (t.initial)
            p.unlocked.insert(t.id);
    return p;
}
bool Bowl::add(const std::string &id, int capacity) {
    if (static_cast<int>(portions.size()) >= capacity || mixed)
        return false;
    portions.push_back(id);
    return true;
}
int Bowl::count(const std::string &id) const {
    return static_cast<int>(std::count(portions.begin(), portions.end(), id));
}
float DayResult::rating() const {
    return served + left > 0 ? 1.f + 4.f * scoreSum / static_cast<float>(served + left) : 1.f;
}
int UpgradeSystem::level(const Progress &p, const std::string &id) {
    const auto it = p.levels.find(id);
    return it == p.levels.end() ? 0 : it->second;
}
float UpgradeSystem::effect(const GameData &d, const Progress &p, const std::string &effect) {
    float total = 0;
    for (const auto &u : d.upgrades) {
        const auto n = level(p, u.id);
        if (u.effect == effect && n > 0 && n <= static_cast<int>(u.levels.size()))
            total += u.levels[static_cast<std::size_t>(n - 1)].value;
    }
    return total;
}
bool UpgradeSystem::buy(const GameData &d, Progress &p, std::size_t index) {
    if (index >= d.upgrades.size())
        return false;
    const auto &u = d.upgrades[index];
    const auto n = level(p, u.id);
    if (n >= static_cast<int>(u.levels.size()) || p.money < u.levels[static_cast<std::size_t>(n)].price)
        return false;
    p.money -= u.levels[static_cast<std::size_t>(n)].price;
    p.levels[u.id] = n + 1;
    return true;
}
bool UpgradeSystem::unlock(const GameData &d, Progress &p, std::size_t index) {
    if (index >= d.toppings.size())
        return false;
    const auto &t = d.toppings[index];
    if (p.unlocked.count(t.id) || p.money < t.unlockPrice)
        return false;
    p.money -= t.unlockPrice;
    p.unlocked.insert(t.id);
    return true;
}
Receipt EconomySystem::score(const GameData &d, const Progress &p, const Customer &c, const Bowl &b) {
    // So sánh cả hợp hai tập: sai loại, thiếu và thừa đều bị trừ điểm.
    int required = 0, difference = 0;
    std::map<std::string, int> actual;
    for (const auto &id : b.portions)
        ++actual[id];
    for (const auto &pair : c.order.recipe) {
        required += pair.second;
        difference += std::abs(actual[pair.first] - pair.second);
        actual.erase(pair.first);
    }
    for (const auto &pair : actual)
        difference += pair.second;
    Receipt r;
    r.recipe =
        std::clamp(1.f - static_cast<float>(difference) /
                             static_cast<float>(std::max({1, required, static_cast<int>(b.portions.size())})),
                   0.f, 1.f);
    r.paper = std::clamp(b.paperQuality, 0.f, 1.f);
    r.mix = b.mixed ? std::clamp(b.mixQuality, 0.f, 1.f) : 0.f;
    r.speed = std::clamp(c.remaining / c.patience, 0.f, 1.f);
    // Trọng số và kinh tế lấy từ JSON, UI không tự tính một công thức khác.
    const auto &w = d.balance.at("weights");
    r.score = r.recipe * w.at("recipe").get<float>() + r.paper * w.at("paper").get<float>() +
              r.mix * w.at("mix").get<float>() + r.speed * w.at("speed").get<float>();
    const float floor = d.number("payment_floor");
    r.base = static_cast<int>(
                 std::round(static_cast<float>(c.order.price) * (floor + (1 - floor) * r.score) / 100.f)) *
             100;
    const float tips = d.number("tip_rate") + UpgradeSystem::effect(d, p, "decor") +
                       UpgradeSystem::effect(d, p, "lantern") + UpgradeSystem::effect(d, p, "curtain");
    r.tip =
        static_cast<int>(std::round(static_cast<float>(c.order.price) * tips * r.score * r.score / 100.f)) *
        100;
    return r;
}
DaySession::DaySession(const GameData &d, Progress &p, unsigned seed)
    : data_(d), progress_(p), random_(seed), remaining_(d.number("day_seconds")) {
    result_.day = p.day;
    spawn();
    spawnTimer_ = d.number("spawn_seconds");
}
int DaySession::capacity() const {
    int levels = 0;
    for (const auto &u : data_.upgrades)
        if (u.effect == "bowl")
            levels += UpgradeSystem::level(progress_, u.id);
    return data_.integer("bowl_capacity") + levels * data_.integer("bowl_capacity_per_level");
}
void DaySession::spawn() {
    const int slots = std::min(data_.integer("queue_max"),
                               data_.integer("queue_slots") +
                                   static_cast<int>(UpgradeSystem::effect(data_, progress_, "queue")));
    if (static_cast<int>(customers_.size()) >= slots)
        return;
    Customer c;
    c.id = nextId_++;
    c.kind = std::uniform_int_distribution<std::size_t>(0, data_.customers.size() - 1)(random_);
    c.patience = (std::max(data_.number("patience_min_seconds"),
                           data_.number("patience_seconds") - static_cast<float>(progress_.day - 1) *
                                                                  data_.number("patience_decrease_per_day")) +
                  UpgradeSystem::effect(data_, progress_, "patience")) *
                 data_.customers[c.kind].patienceFactor;
    c.remaining = c.patience;
    std::vector<std::string> available;
    for (const auto &t : data_.toppings)
        if (progress_.unlocked.count(t.id))
            available.push_back(t.id);
    std::shuffle(available.begin(), available.end(), random_);
    const int types = std::min({data_.integer("order_max_types"), static_cast<int>(available.size()),
                                data_.integer("order_initial_types") +
                                    (progress_.day - 1) / data_.integer("order_types_every_days")});
    int price = data_.integer("base_price");
    for (int i = 0; i < types; ++i) {
        const int quantity =
            progress_.day >= data_.integer("order_quantity_increase_day")
                ? std::uniform_int_distribution<int>(1, data_.integer("order_max_quantity"))(random_)
                : 1;
        const auto &id = available[static_cast<std::size_t>(i)];
        c.order.recipe[id] = quantity;
        price += data_.topping(id).unitPrice * quantity;
    }
    c.order.price =
        static_cast<int>(std::round(static_cast<float>(price) *
                                    (1.f + UpgradeSystem::effect(data_, progress_, "bowl")) / 100.f)) *
        100;
    customers_.push_back(c);
}
void DaySession::update(float dt) {
    if (ended_)
        return;
    dt = std::max(0.f, dt);
    remaining_ -= dt;
    for (auto &c : customers_) {
        c.remaining -= dt;
        c.arrival = std::min(1.f, c.arrival + dt * 4);
    }
    const auto old = customers_.size();
    customers_.erase(
        std::remove_if(customers_.begin(), customers_.end(), [](const auto &c) { return c.remaining <= 0; }),
        customers_.end());
    result_.left += static_cast<int>(old - customers_.size());
    if (remaining_ <= 0) {
        finish();
        return;
    }
    spawnTimer_ -= dt;
    if (spawnTimer_ <= 0) {
        spawn();
        spawnTimer_ = std::max(data_.number("spawn_min_seconds"),
                               data_.number("spawn_seconds") - static_cast<float>(progress_.day - 1) *
                                                                   data_.number("spawn_decrease_per_day"));
    }
}
Receipt DaySession::serve(std::size_t index, const Bowl &bowl) {
    if (ended_ || index >= customers_.size() || !bowl.mixed)
        return {};
    const auto receipt = EconomySystem::score(data_, progress_, customers_[index], bowl);
    result_.revenue += receipt.base;
    result_.tips += receipt.tip;
    ++result_.served;
    result_.scoreSum += receipt.score;
    progress_.money += receipt.base + receipt.tip;
    progress_.lifetime += receipt.base + receipt.tip;
    ++progress_.served;
    customers_.erase(customers_.begin() + static_cast<std::ptrdiff_t>(index));
    return receipt;
}
void DaySession::finish() {
    // Chốt ngày đúng một lần, kể cả nhiều sự kiện cùng xảy ra ở frame cuối.
    if (ended_)
        return;
    ended_ = true;
    remaining_ = 0;
    result_.left += static_cast<int>(customers_.size());
    customers_.clear();
    progress_.bestRating = std::max(progress_.bestRating, result_.rating());
    ++progress_.day;
}
std::string moneyText(int amount) {
    std::string text = std::to_string(amount);
    for (int i = static_cast<int>(text.size()) - 3; i > 0; i -= 3)
        text.insert(static_cast<std::size_t>(i), ".");
    return text + " đ";
}
} // namespace tiem

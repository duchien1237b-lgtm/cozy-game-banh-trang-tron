#include "gameplay/Inventory.hpp"
#include <cmath>
namespace tiem {
namespace {
int roundHundred(float value) { return static_cast<int>(std::round(value / 100.f)) * 100; }
} // namespace
int InventorySystem::unitCost(const GameData &d, const Topping &t) {
    return std::max(100, roundHundred(static_cast<float>(t.unitPrice) * d.numberOr("ingredient_cost_rate", .5f)));
}
int InventorySystem::rushCost(const GameData &d, const Topping &t) {
    return std::max(100, roundHundred(static_cast<float>(unitCost(d, t)) * d.numberOr("rush_restock_factor", 2.f)));
}
int InventorySystem::maxStock(const GameData &d) { return std::max(1, static_cast<int>(d.numberOr("stock_max", 60.f))); }
int InventorySystem::stock(const Progress &p, const std::string &id) {
    const auto it = p.stock.find(id);
    return it == p.stock.end() ? 0 : std::max(0, it->second);
}
int InventorySystem::buy(const GameData &d, Progress &p, const std::string &id, int amount, bool rush) {
    if (amount <= 0 || !p.unlocked.count(id)) return 0;
    const auto &t = d.topping(id);
    amount = std::min(amount, maxStock(d) - stock(p, id));
    const int price = rush ? rushCost(d, t) : unitCost(d, t);
    if (amount <= 0 || p.money < price * amount) return 0;
    p.money -= price * amount;
    p.todayCost += price * amount;
    p.stock[id] = stock(p, id) + amount;
    return amount;
}
int InventorySystem::cost(const GameData &d, const std::map<std::string, int> &cart) {
    int total = 0;
    for (const auto &item : cart)
        if (item.second > 0) total += unitCost(d, d.topping(item.first)) * item.second;
    return total;
}
bool InventorySystem::buyCart(const GameData &d, Progress &p, const std::map<std::string, int> &cart) {
    for (const auto &item : cart)
        if (item.second > 0 && (!p.unlocked.count(item.first) || stock(p, item.first) + item.second > maxStock(d)))
            return false;
    if (cost(d, cart) > p.money) return false;
    for (const auto &item : cart) buy(d, p, item.first, item.second);
    return true;
}
int InventorySystem::take(Progress &p, const std::string &id, int amount) {
    const int taken = std::clamp(amount, 0, stock(p, id));
    if (taken > 0) p.stock[id] = stock(p, id) - taken;
    return taken;
}
void InventorySystem::restore(const GameData &d, Progress &p, const std::string &id, int amount) {
    if (amount > 0) p.stock[id] = std::min(maxStock(d), stock(p, id) + amount);
}
InventorySystem::Spoilage InventorySystem::spoil(const GameData &d, Progress &p) {
    Spoilage lost;
    for (auto &item : p.stock) {
        const auto found = std::find_if(d.toppings.begin(), d.toppings.end(),
                                        [&](const Topping &t) { return t.id == item.first; });
        if (found == d.toppings.end() || !spoils(*found) || item.second <= 0) continue;
        lost.portions += item.second;
        lost.value += unitCost(d, *found) * item.second;
        item.second = 0;
    }
    return lost;
}
int InventorySystem::expectedCustomers(const GameData &d, const Progress &p, const WeatherPlan &plan) {
    const float interval = std::max(d.number("spawn_min_seconds"), d.number("spawn_seconds") / difficultyMultiplier(d, p.day));
    const float arrivals = 1.f + d.number("day_seconds") / interval * WeatherSystem::average(plan).spawn;
    // Một người làm trung bình ~22 giây một phần: không ước tính quá khả năng phục vụ.
    return std::max(1, static_cast<int>(std::round(std::min(arrivals, d.number("day_seconds") / 22.f + 1.f))));
}
std::map<std::string, int> InventorySystem::demand(const GameData &d, const Progress &p, const WeatherPlan &plan) {
    std::map<std::string, int> need;
    std::vector<const Topping *> open;
    for (const auto &t : d.toppings)
        if (p.unlocked.count(t.id)) open.push_back(&t);
    if (open.empty()) return need;
    const float bias = WeatherSystem::average(plan).freshBias;
    float weightSum = 0;
    for (const auto *t : open) weightSum += WeatherSystem::toppingWeight(t->type, bias);
    // Đơn trung bình ~4 loại × 1,4 muỗng, tăng dần theo độ khó.
    const float portionsPerOrder = 5.5f * difficultyMultiplier(d, p.day);
    const float typesPerOrder = std::min(static_cast<float>(open.size()), 4.f);
    const float customers = static_cast<float>(expectedCustomers(d, p, plan));
    for (const auto *t : open) {
        const float share = WeatherSystem::toppingWeight(t->type, bias) / weightSum * static_cast<float>(open.size());
        const float chance = std::min(1.f, typesPerOrder / static_cast<float>(open.size()) * share);
        const float expected = customers * chance * portionsPerOrder / typesPerOrder * 1.15f; // dư 15% phòng hờ
        need[t->id] = std::clamp(static_cast<int>(std::ceil(expected)), 2, maxStock(d));
    }
    return need;
}
std::map<std::string, int> InventorySystem::suggest(const GameData &d, const Progress &p, const WeatherPlan &plan) {
    std::map<std::string, int> cart;
    for (const auto &item : demand(d, p, plan)) {
        const int missing = item.second - stock(p, item.first);
        if (missing > 0) cart[item.first] = missing;
    }
    // Gợi ý chỉ tiêu tối đa 60% ví để còn tiền nâng cấp; giảm dần món nhiều nhất cho tới khi vừa ngân sách.
    const int budget = static_cast<int>(static_cast<float>(p.money) * d.numberOr("suggest_budget_rate", .6f));
    while (!cart.empty() && cost(d, cart) > budget) {
        auto most = std::max_element(cart.begin(), cart.end(), [](const auto &a, const auto &b) { return a.second < b.second; });
        if (--most->second <= 0) cart.erase(most);
    }
    return cart;
}
} // namespace tiem

#include "states/PrepState.hpp"
#include "gameplay/Inventory.hpp"
#include "ui/Art.hpp"
#include "ui/Atmosphere.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace tiem {
namespace {
constexpr std::size_t perPage = 8;
const char *typeName(const std::string &type) {
    return type == "dry" ? "Khô" : type == "fresh" ? "Tươi" : "Sốt";
}
std::string percent(float value) {
    const int n = static_cast<int>(std::lround(value * 100.f));
    return (n > 0 ? "+" : "") + std::to_string(n) + "%";
}
} // namespace

PrepState::PrepState(StateStack &stack, AppContext &context) : UiState(stack, context) {
    if (context_.audio) context_.audio->music("menu");
    auto &p = game().progress;
    for (std::size_t i = 0; i < game().data.toppings.size(); ++i)
        if (p.unlocked.count(game().data.toppings[i].id)) open_.push_back(i);
    plan_ = WeatherSystem::forecast(p.worldSeed, p.day);
    demand_ = InventorySystem::demand(game().data, p, plan_);
    // Mở màn với giỏ gợi ý sẵn: người mới chỉ cần bấm Nhập hàng.
    cart_ = InventorySystem::suggest(game().data, p, plan_);
    rebuild();
}
sf::FloatRect PrepState::cardRect(std::size_t slot) const {
    return {446.f + static_cast<float>(slot % 2) * 408.f, 104.f + static_cast<float>(slot / 2) * 122.f, 396.f, 110.f};
}
int PrepState::cartCost() const { return InventorySystem::cost(game().data, cart_); }
void PrepState::adjust(const std::string &id, int delta) {
    const auto &d = game().data;
    auto &p = game().progress;
    const auto &t = d.topping(id);
    int &amount = cart_[id];
    if (delta > 0) {
        const int room = InventorySystem::maxStock(d) - InventorySystem::stock(p, id) - amount;
        const int affordable = (p.money - cartCost()) / std::max(1, InventorySystem::unitCost(d, t));
        delta = std::min({delta, room, affordable});
    } else delta = std::max(delta, -amount);
    amount += delta;
    if (amount <= 0) cart_.erase(id);
    if (delta != 0) sound(delta > 0 ? "topping_select" : "undo");
    dirty_ = true;
}
void PrepState::commit() {
    auto &p = game().progress;
    const int cost = cartCost();
    if (cost > p.money) {
        effects_.say("Không đủ tiền!", {1010, 620}, ui::Theme{}.red, 1.4f, 24, true);
        return;
    }
    if (!cart_.empty()) {
        if (!InventorySystem::buyCart(game().data, p, cart_)) return;
        sound("upgrade_buy");
    }
    game().save();
    stack_.replace(StateID::Playing);
}
void PrepState::rebuild() {
    buttons_.clear();
    const auto &d = game().data;
    const auto &p = game().progress;
    addButton("Về menu", {1080, 26, 170, 44}, [this] { returnToMenu(); });
    addButton("Gợi ý nhập", {446, 600, 150, 44}, [this] {
        cart_ = InventorySystem::suggest(game().data, game().progress, plan_);
        sound("topping_select");
        dirty_ = true;
    });
    addButton("Xóa giỏ", {606, 600, 120, 44}, [this] {
        cart_.clear();
        sound("undo");
        dirty_ = true;
    }).setEnabled(!cart_.empty());
    addButton("Trước", {446, 652, 120, 40}, [this] {
        if (page_ > 0) --page_;
        dirty_ = true;
    }).setEnabled(page_ > 0);
    addButton("Sau", {606, 652, 120, 40}, [this] {
        ++page_;
        dirty_ = true;
    }).setEnabled((page_ + 1) * perPage < open_.size());
    const bool affordable = cartCost() <= p.money;
    addButton(cart_.empty() ? "Mở quán luôn" : "Nhập hàng & mở quán", {950, 640, 300, 52}, [this] { commit(); })
        .setEnabled(affordable);
    for (std::size_t slot = 0; slot < perPage; ++slot) {
        const auto index = page_ * perPage + slot;
        if (index >= open_.size()) break;
        const auto &t = d.toppings[open_[index]];
        const auto card = cardRect(slot);
        const int planned = cart_.count(t.id) ? cart_.at(t.id) : 0;
        const int room = InventorySystem::maxStock(d) - InventorySystem::stock(p, t.id) - planned;
        const bool canAdd = room > 0 && p.money - cartCost() >= InventorySystem::unitCost(d, t);
        const std::string id = t.id;
        addButton("−", {card.left + 232, card.top + 66, 44, 34}, [this, id] { adjust(id, -1); }).setEnabled(planned > 0);
        addButton("+1", {card.left + 282, card.top + 66, 50, 34}, [this, id] { adjust(id, 1); }).setEnabled(canAdd);
        addButton("+5", {card.left + 338, card.top + 66, 50, 34}, [this, id] { adjust(id, 5); }).setEnabled(canAdd);
    }
}
bool PrepState::handleEvent(const sf::Event &e, Pointer mouse) {
    pointer_ = mouse;
    if (e.type == sf::Event::KeyPressed) {
        if (game().progress.keys.matches("start", e.key.code)) {
            commit();
            return false;
        }
        if (e.key.code == sf::Keyboard::Escape) {
            returnToMenu();
            return false;
        }
        if (e.key.code == sf::Keyboard::Right && (page_ + 1) * perPage < open_.size()) { ++page_; dirty_ = true; }
        if (e.key.code == sf::Keyboard::Left && page_ > 0) { --page_; dirty_ = true; }
    }
    handleButtons(e, mouse);
    return false;
}
bool PrepState::update(float dt) {
    age_ += dt;
    if (dirty_) {
        dirty_ = false;
        rebuild();
    }
    effects_.update(dt);
    return UiState::update(dt);
}
void PrepState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &titleFont = context_.resources.titleFont();
    const auto &d = game().data;
    const auto &p = game().progress;
    ui::backdrop(target);
    ui::lightShafts(target, {0, 0, 1280, 640}, age_, 12);
    ui::text(target, titleFont, "Chuẩn bị nguyên liệu", 34, {30, 22}, theme.ink);
    ui::text(target, font, "Ngày " + std::to_string(p.day) + "  •  Tiệm mở cửa lúc 07:00 và đóng cửa lúc 21:00", 16, {32, 66}, theme.muted);
    // Ví tiền.
    ui::shadowPanel(target, {820, 26, 240, 44}, {255, 250, 233, 235}, 16, 3, 40);
    ui::image(target, context_.resources, "ui/coin_icon", {832, 33, 30, 30});
    ui::text(target, font, moneyText(p.money), 21, {872, 36}, theme.green);

    // Thẻ dự báo thời tiết.
    const sf::FloatRect weatherCard{30, 104, 396, 270};
    ui::shadowPanel(target, weatherCard, {240, 245, 250}, 22, 5, 45);
    ui::text(target, titleFont, "Dự báo thời tiết", 22, {52, 118}, {70, 104, 140});
    ui::weatherIcon(target, plan_.base, {96, 196}, 74, age_);
    if (plan_.changes()) {
        ui::text(target, font, "rồi", 13, {150, 226}, theme.muted);
        ui::weatherIcon(target, plan_.interlude, {190, 214}, 40, age_);
    }
    ui::paragraph(target, titleFont, WeatherSystem::summary(plan_), 19, {218, 160}, 196, 24, theme.ink);
    const auto avg = WeatherSystem::average(plan_);
    struct Fact { std::string label, value; sf::Color color; };
    const Fact facts[] = {
        {"Lượng khách", percent(avg.spawn - 1.f), std::abs(avg.spawn - 1.f) < .01f ? theme.ink : avg.spawn > 1.f ? theme.green : theme.red},
        {"Kiên nhẫn", percent(avg.patience - 1.f), std::abs(avg.patience - 1.f) < .01f ? theme.ink : avg.patience > 1.f ? theme.green : theme.red},
        {"Tip thêm", percent(avg.tip), avg.tip > 0 ? theme.green : theme.muted},
        {"Khẩu vị", avg.freshBias > .1f ? "Đồ tươi" : avg.freshBias < -.1f ? "Đồ khô, sốt" : "Cân bằng", theme.ink}};
    for (int i = 0; i < 4; ++i) {
        const float x = 48 + static_cast<float>(i % 2) * 190, y = 262 + static_cast<float>(i / 2) * 32;
        ui::text(target, font, facts[i].label, 14, {x, y}, theme.muted);
        ui::text(target, font, facts[i].value, 15, {x + 92, y - 1}, facts[i].color);
    }
    ui::paragraph(target, font, WeatherSystem::advice(plan_), 13, {48, 330}, 360, 17, {70, 104, 140});

    // Thẻ dự kiến và giỏ hàng.
    const sf::FloatRect planCard{30, 388, 396, 304};
    ui::shadowPanel(target, planCard, theme.paper, 22, 5, 45);
    const int customers = InventorySystem::expectedCustomers(d, p, plan_);
    int needed = 0, have = 0;
    for (const auto &item : demand_) {
        needed += item.second;
        have += InventorySystem::stock(p, item.first) + (cart_.count(item.first) ? cart_.at(item.first) : 0);
    }
    ui::text(target, titleFont, "Dự kiến hôm nay", 22, {52, 402}, theme.green);
    ui::text(target, font, "Khoảng " + std::to_string(customers) + " khách  •  cần ~" + std::to_string(needed) + " muỗng", 16, {52, 438}, theme.ink);
    ui::ProgressBar::draw(target, {52, 466, 352, 10}, static_cast<float>(have) / static_cast<float>(std::max(1, needed)),
                          have >= needed ? theme.green : theme.orange);
    ui::text(target, font, "Sẽ có " + std::to_string(have) + " muỗng sau khi nhập", 13, {52, 482}, theme.muted);
    ui::paragraph(target, font, "Đồ tươi (xoài, rau răm, trứng cút, xúc xích) hỏng lúc đóng cửa. Đồ khô và nước sốt để dành được sang hôm sau. Hết hàng giữa ngày vẫn nhập gấp được nhưng giá gấp đôi.",
                  13, {52, 508}, 352, 17, theme.muted);
    ui::panel(target, {48, 590, 360, 2}, {224, 217, 197}, 1);
    const int cost = cartCost();
    ui::text(target, font, "Tiền nhập hàng", 17, {52, 604}, theme.muted);
    const std::string costText = "−" + moneyText(cost);
    ui::text(target, titleFont, costText, 24, {404 - ui::textWidth(titleFont, costText, 24), 600}, cost > p.money ? theme.red : theme.ink);
    ui::text(target, font, "Còn lại trong ví", 15, {52, 642}, theme.muted);
    const std::string leftText = moneyText(std::max(0, p.money - cost));
    ui::text(target, font, leftText, 17, {404 - ui::textWidth(font, leftText, 17), 641}, theme.green);
    ui::text(target, font, p.keys.label("start") + ": nhập hàng & mở quán", 12, {52, 670}, theme.muted);

    // Thẻ từng topping.
    for (std::size_t slot = 0; slot < perPage; ++slot) {
        const auto index = page_ * perPage + slot;
        if (index >= open_.size()) break;
        const auto &t = d.toppings[open_[index]];
        const auto card = cardRect(slot);
        const bool hovered = pointer_ && card.contains(*pointer_);
        const int stock = InventorySystem::stock(p, t.id);
        const int planned = cart_.count(t.id) ? cart_.at(t.id) : 0;
        const int want = demand_.count(t.id) ? demand_.at(t.id) : 0;
        ui::shadowPanel(target, {card.left, card.top - (hovered ? 2.f : 0.f), card.width, card.height}, theme.paper, 16, hovered ? 6.f : 4.f, 40);
        if (hovered) ui::panelOutline(target, {card.left, card.top - 2, card.width, card.height}, {226, 168, 58, 160}, 16, 2);
        ui::panel(target, {card.left + 12, card.top + 14, 70, 70}, {250, 241, 214}, 35);
        if (!ui::image(target, context_.resources, "toppings/" + t.id + "_icon", {card.left + 17, card.top + 19, 60, 60}))
            ui::ingredient(target, context_.resources, t, {card.left + 47, card.top + 49}, 1.6f);
        ui::text(target, titleFont, t.name, 20, {card.left + 94, card.top + 10}, theme.ink);
        const sf::Color tagColor = t.type == "fresh" ? sf::Color(96, 150, 86) : t.type == "dry" ? sf::Color(176, 120, 64) : sf::Color(200, 90, 60);
        const float nameWidth = ui::textWidth(titleFont, t.name, 20);
        ui::panel(target, {card.left + 102 + nameWidth, card.top + 14, 46, 20}, tagColor, 10);
        ui::text(target, font, typeName(t.type), 12, {card.left + 125 + nameWidth, card.top + 23}, {255, 250, 232}, true);
        ui::text(target, font, moneyText(InventorySystem::unitCost(d, t)) + " / muỗng", 13, {card.left + 94, card.top + 40}, theme.muted);
        // Kho hiện có → sau khi nhập, so với nhu cầu.
        const int after = stock + planned;
        ui::text(target, font, "Kho " + std::to_string(stock) + (planned ? "  •  sau nhập " + std::to_string(after) : ""), 15,
                 {card.left + 94, card.top + 62}, after >= want ? theme.green : after == 0 ? theme.red : theme.orange);
        ui::text(target, font, "Nên có ~" + std::to_string(want), 12, {card.left + 94, card.top + 84}, theme.muted);
        if (planned > 0) {
            const std::string chip = "+" + std::to_string(planned);
            ui::panel(target, {card.left + card.width - 66, card.top + 12, 54, 26}, theme.gold, 13);
            ui::text(target, font, chip, 15, {card.left + card.width - 39, card.top + 25}, {255, 252, 238}, true);
        }
        if (InventorySystem::spoils(t) && stock > 0)
            ui::text(target, font, "Hỏng cuối ngày", 11, {card.left + 232, card.top + 44}, theme.red);
    }
    const auto pages = (open_.size() + perPage - 1) / perPage;
    ui::text(target, font, "Trang " + std::to_string(page_ + 1) + "/" + std::to_string(std::max<std::size_t>(1, pages)), 14, {742, 662}, theme.muted);
    renderButtons(target);
    effects_.render(target, font);
}
} // namespace tiem

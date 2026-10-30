#include "states/ManagementState.hpp"
#include "ui/Art.hpp"
#include "gameplay/Minigames.hpp"
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>
namespace tiem {
namespace {
std::string benefit(const GameData &data, const Upgrade &u, int level) {
    const float value = level > 0 ? u.levels.at(static_cast<std::size_t>(level - 1)).value : 0.f;
    const auto percent = [](float n) { return std::to_string(static_cast<int>(std::lround(n * 100))); };
    if (u.effect == "paper") {
        PaperTearing paper(data, {0, 0, 200, 200}, value, level);
        std::ostringstream text;
        text << std::fixed << std::setprecision(2) << paper.stageSeconds() << "s/giai đoạn";
        if (level > 0) text << " • vùng rê rộng";
        return text.str();
    }
    if (u.effect == "bowl") return std::to_string(data.integer("bowl_capacity") + level * data.integer("bowl_capacity_per_level")) + " muỗng • giá +" + percent(value) + "%";
    if (u.effect == "patience") return "Thời gian chờ +" + std::to_string(static_cast<int>(value)) + "s";
    if (u.effect == "mix") return "Vùng xanh " + percent(std::min(data.number("mix_window_max"), data.number("mix_window") + value)) + "%";
    if (u.effect == "queue") return std::to_string(std::min(data.integer("queue_max"), data.integer("queue_slots") + static_cast<int>(value))) + " chỗ đón khách";
    return std::string(level > 0 ? "Đã trang trí • tip +" : "Chưa trang trí • tip +") + percent(value) + "%";
}
}

ManagementState::ManagementState(StateStack &stack, AppContext &context, StateID kind)
    : UiState(stack, context), kind_(kind) {
    if (context_.audio) {
        context_.audio->music("menu");
    }
    rebuild();
}
void ManagementState::rebuild() {
    buttons_.clear();
    addButton("Về menu", {1080, 30, 170, 45}, [this] { returnToMenu(); });
    if (kind_ == StateID::DayResult) {
        addButton("Đến cửa hàng", {420, 598, 210, 51}, [this] { stack_.replace(StateID::UpgradeShop); });
        addButton("Mở ngày tiếp", {650, 598, 210, 51}, [this] { stack_.replace(StateID::Playing); });
        return;
    }
    addButton("Nâng cấp quán", {30, 103, 225, 44}, [this] {
        ingredients_ = false;
        page_ = 0;
        dirty_ = true;
    });
    addButton("Mở topping", {275, 103, 225, 44}, [this] {
        ingredients_ = true;
        page_ = 0;
        dirty_ = true;
    });
    addButton("Trước", {30, 646, 170, 42}, [this] {
        if (page_ > 0) {
            --page_;
            dirty_ = true;
        }
    }).setEnabled(page_ > 0);
    const auto size = ingredients_ ? game().data.toppings.size() : game().data.upgrades.size();
    addButton("Sau", {220, 646, 170, 42}, [this] {
        ++page_;
        dirty_ = true;
    }).setEnabled((page_ + 1) * 6 < size);
    addButton("Mở ngày " + std::to_string(game().progress.day), {965, 640, 285, 50},
              [this] { stack_.replace(StateID::Playing); });
    for (std::size_t slot = 0; slot < 6; ++slot) {
        const auto index = page_ * 6 + slot;
        if (index >= size)
            break;
        const float x = 30 + static_cast<float>(slot % 2) * 620, y = 171 + static_cast<float>(slot / 2) * 148;
        int price = 0;
        bool bought = false;
        if (ingredients_) {
            const auto &t = game().data.toppings[index];
            price = t.unlockPrice;
            bought = game().progress.unlocked.count(t.id) > 0;
        } else {
            const auto &u = game().data.upgrades[index];
            const int n = UpgradeSystem::level(game().progress, u.id);
            bought = n >= static_cast<int>(u.levels.size());
            if (!bought)
                price = u.levels[static_cast<std::size_t>(n)].price;
        }
        addButton(bought ? "Đã có" : moneyText(price), {x + 375, y + 72, 205, 42}, [this, index, x, y] {
            const bool success = ingredients_ ? UpgradeSystem::unlock(game().data, game().progress, index)
                                              : UpgradeSystem::buy(game().data, game().progress, index);
            if (success) {
                game().save();
                sound("upgrade_buy");
                effects_.say("Đã mua!", {x + 460, y + 67}, {70, 122, 68});
                dirty_ = true;
            }
        }).setEnabled(!bought && game().progress.money >= price);
    }
}
bool ManagementState::handleEvent(const sf::Event &e, Pointer p) {
    handleButtons(e, p);
    return false;
}
bool ManagementState::update(float dt) {
    if (dirty_) {
        dirty_ = false;
        rebuild();
    }
    effects_.update(dt);
    return UiState::update(dt);
}
void ManagementState::drawResult(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &r = game().lastDay;
    ui::text(target, context_.resources.titleFont(), "Khép lại một ngày ngon lành", 35, {640, 114}, theme.ink, true);
    ui::text(target, font, "Ngày " + std::to_string(r.day), 20, {640, 164}, theme.muted, true);
    ui::panel(target, {280, 201, 720, 364}, theme.paper, 25);
    for (int i = 0; i < 5; ++i)
        ui::star(target, {528 + static_cast<float>(i) * 56, 250}, 21,
                 static_cast<float>(i) + .5f <= r.rating() ? sf::Color(225, 176, 71)
                                                           : sf::Color(224, 217, 194));
    std::ostringstream rating;
    rating << std::fixed << std::setprecision(1) << r.rating() << " / 5 sao";
    ui::text(target, font, rating.str(), 19, {640, 296}, theme.green, true);
    const std::vector<std::pair<std::string, std::string>> rows = {
        {"Tiền món", moneyText(r.revenue)},
        {"Tiền tip", moneyText(r.tips)},
        {"Khách đã phục vụ", std::to_string(r.served)},
        {"Khách rời quán / hết giờ", std::to_string(r.left)}};
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const float y = 340 + static_cast<float>(i) * 43;
        ui::text(target, font, rows[i].first, 21, {325, y}, theme.muted);
        ui::text(target, font, rows[i].second, 21, {775, y}, theme.ink);
    }
    ui::text(target, font, "Ví hiện tại: " + moneyText(game().progress.money), 20, {640, 539}, theme.green,
             true);
    const auto &weights = game().data.balance.at("weights");
    const auto percent = [&](const char *key) {
        return std::to_string(static_cast<int>(weights.at(key).get<float>() * 100 + .5f)) + "%";
    };
    ui::text(target, font,
             "Công thức " + percent("recipe") + " • Xé " + percent("paper") + " • Trộn " + percent("mix") +
                 " • Tốc độ " + percent("speed"),
             16, {640, 686}, theme.muted, true);
}
void ManagementState::drawShop(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    ui::text(target, context_.resources.titleFont(), "Chút chăm chút cho tiệm nhỏ", 29, {30, 36}, theme.ink);
    ui::text(target, font, moneyText(game().progress.money), 25, {750, 35}, theme.green);
    const auto size = ingredients_ ? game().data.toppings.size() : game().data.upgrades.size();
    for (std::size_t slot = 0; slot < 6; ++slot) {
        const auto index = page_ * 6 + slot;
        if (index >= size)
            break;
        const float x = 30 + static_cast<float>(slot % 2) * 620, y = 171 + static_cast<float>(slot / 2) * 148;
        ui::panel(target, {x, y, 600, 130}, theme.paper, 16);
        if (ingredients_) {
            const auto &t = game().data.toppings[index];
            ui::ingredient(target, context_.resources, t, {x + 45, y + 44}, 1.7f);
            ui::text(target, font, t.name, 23, {x + 88, y + 22}, theme.ink);
            ui::text(target, font,
                     "Chọn → giữ trên tô → thả để thêm", 16,
                     {x + 27, y + 83}, theme.muted);
        } else {
            const auto &u = game().data.upgrades[index];
            const int level = UpgradeSystem::level(game().progress, u.id);
            ui::image(target, context_.resources, "ui/upgrade_" + u.id, {x + 12, y + 12, 62, 62});
            ui::text(target, font, u.name + "  (" + std::to_string(level) + "/" + std::to_string(u.levels.size()) + ")", 22, {x + 88, y + 10}, theme.ink);
            ui::text(target, font, u.description, 15, {x + 88, y + 40}, theme.muted);
            ui::text(target, font, "Đang dùng: " + benefit(game().data, u, level), 14,
                     {x + 18, y + 73}, theme.green);
            const bool maxed = level >= static_cast<int>(u.levels.size());
            ui::text(target, font, maxed ? "Đã đạt mốc tối đa" : "Mốc tiếp: " + benefit(game().data, u, level + 1),
                     14, {x + 18, y + 100}, theme.muted);
        }
    }
    ui::text(target, font, "Trang " + std::to_string(page_ + 1) + " / " + std::to_string((size + 5) / 6), 16,
             {475, 660}, theme.muted);
    if (!game().saves.error().empty())
        ui::text(target, font, "Không lưu được tiến trình; kiểm tra quyền ghi.", 14, {30, 620},
                 {170, 65, 45});
}
void ManagementState::render(sf::RenderTarget &target) {
    ui::backdrop(target);
    if (kind_ == StateID::DayResult)
        drawResult(target);
    else
        drawShop(target);
    renderButtons(target);
    effects_.render(target, context_.resources.uiFont());
}
} // namespace tiem

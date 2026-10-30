#include "states/ManagementState.hpp"
#include "gameplay/Minigames.hpp"
#include "ui/Art.hpp"
#include <algorithm>
#include <cmath>
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
float easeOutBack(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    t = std::clamp(t, 0.f, 1.f);
    return 1.f + c3 * std::pow(t - 1.f, 3.f) + c1 * std::pow(t - 1.f, 2.f);
}
const char *typeName(const std::string &type) {
    return type == "dry" ? "Khô" : type == "fresh" ? "Tươi" : "Sốt";
}
} // namespace

ManagementState::ManagementState(StateStack &stack, AppContext &context, StateID kind)
    : UiState(stack, context), kind_(kind) {
    if (context_.audio) {
        context_.audio->music("menu");
    }
    // Kiểm tra lại thành tựu khi vào cửa hàng (ví dụ mở topping, nâng cấp từ lượt trước).
    rebuild();
}
sf::FloatRect ManagementState::cardRect(std::size_t slot) const {
    return {30 + static_cast<float>(slot % 2) * 620, 171 + static_cast<float>(slot / 2) * 148, 600, 130};
}
float ManagementState::count(float delay, float seconds) const {
    return ui::easeOut(std::clamp((age_ - delay) / seconds, 0.f, 1.f));
}
void ManagementState::rebuild() {
    buttons_.clear();
    addButton("Về menu", {1080, 30, 170, 45}, [this] { returnToMenu(); });
    if (kind_ == StateID::DayResult) {
        addButton("Đến cửa hàng", {420, 646, 210, 50}, [this] { stack_.replace(StateID::UpgradeShop); });
        addButton("Mở ngày " + std::to_string(game().progress.day), {650, 646, 210, 50}, [this] { stack_.replace(StateID::Playing); });
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
        const auto card = cardRect(slot);
        const float x = card.left, y = card.top;
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
        addButton(bought ? "Đã có" : moneyText(price), {x + 375, y + 78, 205, 42}, [this, index, x, y] {
            const bool success = ingredients_ ? UpgradeSystem::unlock(game().data, game().progress, index)
                                              : UpgradeSystem::buy(game().data, game().progress, index);
            if (success) {
                for (auto &id : AchievementSystem::check(game().data, game().progress)) {
                    toasts_.push_back({id, 0.f});
                    sound("achievement");
                }
                game().save();
                sound("upgrade_buy");
                effects_.say("Đã mua!", {x + 477, y + 58}, {70, 122, 68}, 1.6f, 25, true);
                effects_.sparkle({x + 477, y + 99}, {255, 214, 98}, 20);
                dirty_ = true;
            }
        }).setEnabled(!bought && game().progress.money >= price);
    }
}
bool ManagementState::handleEvent(const sf::Event &e, Pointer p) {
    pointer_ = p;
    if (e.type == sf::Event::MouseLeft) pointer_.reset();
    if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Enter) {
            sound("ui_click");
            stack_.replace(StateID::Playing);
            return false;
        }
        if (kind_ == StateID::UpgradeShop) {
            const auto size = ingredients_ ? game().data.toppings.size() : game().data.upgrades.size();
            if (e.key.code == sf::Keyboard::Left && page_ > 0) { --page_; dirty_ = true; return false; }
            if (e.key.code == sf::Keyboard::Right && (page_ + 1) * 6 < size) { ++page_; dirty_ = true; return false; }
            if (e.key.code == sf::Keyboard::Tab) { ingredients_ = !ingredients_; page_ = 0; dirty_ = true; return false; }
        }
    }
    handleButtons(e, p);
    return false;
}
bool ManagementState::update(float dt) {
    age_ += dt;
    if (dirty_) {
        dirty_ = false;
        rebuild();
    }
    effects_.update(dt);
    for (auto &toast : toasts_) toast.age += dt;
    while (!toasts_.empty() && toasts_.front().age > 4.f) toasts_.pop_front();
    if (kind_ == StateID::DayResult) {
        // Sao nảy lần lượt kèm âm thanh; ngày tốt có thêm tia sáng chúc mừng.
        const auto &r = game().lastDay;
        for (unsigned i = 0; i < 5; ++i)
            if (!(starsPopped_ & (1u << i)) && age_ > .35f + static_cast<float>(i) * .17f) {
                starsPopped_ |= 1u << i;
                if (static_cast<float>(i) + .5f <= r.rating()) {
                    sound("ingredient_pop");
                    effects_.burst({345.f + (static_cast<float>(i) - 2.f) * 62.f, 165}, {255, 214, 98}, 8);
                }
            }
        if (!celebrated_ && age_ > 1.4f) {
            celebrated_ = true;
            if (r.rating() >= 4.f && r.served > 0) {
                effects_.sparkle({345, 160}, {255, 214, 98}, 30);
                sound("achievement");
            }
        }
    }
    return UiState::update(dt);
}
void ManagementState::drawResult(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &titleFont = context_.resources.titleFont();
    const auto &r = game().lastDay;
    ui::text(target, titleFont, "Khép lại một ngày ngon lành", 36, {640, 48}, theme.ink, true);
    ui::text(target, font, "Ngày " + std::to_string(r.day), 18, {640, 84}, theme.muted, true);
    // Thẻ trái: sao, doanh thu, chỉ số.
    const sf::FloatRect left{50, 108, 590, 520};
    ui::shadowPanel(target, left, theme.paper, 25, 6, 50);
    for (int i = 0; i < 5; ++i) {
        const float pop = (starsPopped_ & (1u << i)) ? easeOutBack((age_ - (.35f + static_cast<float>(i) * .17f)) / .35f) : 0.f;
        const bool lit = static_cast<float>(i) + .5f <= r.rating();
        ui::star(target, {345.f + (static_cast<float>(i) - 2.f) * 62.f, 160}, 25 * std::max(0.f, pop),
                 lit ? sf::Color(232, 176, 60) : sf::Color(224, 217, 194));
    }
    std::ostringstream rating;
    rating << std::fixed << std::setprecision(1) << r.rating() << " / 5 sao";
    ui::text(target, titleFont, rating.str(), 24, {345, 212}, theme.green, true);
    if (r.served > 0 && r.rating() >= game().progress.bestRating - .001f && r.rating() >= 1.5f) {
        const float pulse = .5f + .5f * std::sin(age_ * 5.f);
        ui::panel(target, {470, 198, 142, 28}, {static_cast<sf::Uint8>(226 + 20 * pulse), 168, 58}, 14);
        ui::text(target, font, "Kỷ lục mới!", 15, {541, 212}, {255, 250, 232}, true);
    }
    struct Row { std::string label, value; sf::Color color; };
    std::vector<Row> rows;
    const int plainTips = r.tips - r.comboBonus - r.rushBonus;
    rows.push_back({"Tiền món", moneyText(static_cast<int>(r.revenue * count(.6))), theme.ink});
    rows.push_back({"Tiền tip", moneyText(static_cast<int>(plainTips * count(.7))), theme.ink});
    if (r.comboBonus > 0) rows.push_back({"Thưởng combo (tối đa x" + std::to_string(r.bestCombo) + ")", moneyText(static_cast<int>(r.comboBonus * count(.8))), theme.gold});
    if (r.rushBonus > 0) rows.push_back({"Thưởng giờ cao điểm", moneyText(static_cast<int>(r.rushBonus * count(.9))), theme.orange});
    if (r.goalBonus > 0) rows.push_back({"Thưởng nhiệm vụ", moneyText(static_cast<int>(r.goalBonus * count(1.0))), theme.green});
    float y = 252;
    for (const auto &row : rows) {
        ui::text(target, font, row.label, 20, {86, y}, theme.muted);
        ui::text(target, font, row.value, 20, {604 - ui::textWidth(font, row.value, 20), y}, row.color);
        y += 36;
    }
    ui::panel(target, {86, y + 2, 518, 2}, {224, 217, 197}, 1);
    const int total = r.revenue + r.tips + r.goalBonus;
    ui::text(target, titleFont, "Tổng thu hôm nay", 22, {86, y + 14}, theme.ink);
    const std::string totalText = "+" + moneyText(static_cast<int>(total * count(1.1, 1.1)));
    ui::text(target, titleFont, totalText, 26, {604 - ui::textWidth(titleFont, totalText, 26), y + 12}, theme.green);
    const std::pair<std::string, std::string> chips[] = {{"Phục vụ", std::to_string(r.served)}, {"Bỏ về", std::to_string(r.left)},
        {"Combo cao nhất", "x" + std::to_string(r.bestCombo)}, {"Trộn Perfect", std::to_string(r.perfectMixes)}};
    for (int i = 0; i < 4; ++i) {
        const float x = 86 + static_cast<float>(i) * 132;
        ui::panel(target, {x, 520, 122, 62}, {250, 241, 214}, 14);
        ui::text(target, font, chips[i].first, 12, {x + 61, 536}, theme.muted, true);
        ui::text(target, titleFont, chips[i].second, 22, {x + 61, 562}, i == 1 && r.left > 0 ? theme.red : theme.ink, true);
    }
    // Thẻ phải: nhiệm vụ và thành tựu.
    const sf::FloatRect right{660, 108, 570, 520};
    ui::shadowPanel(target, right, theme.paper, 25, 6, 50);
    ui::text(target, titleFont, "Nhiệm vụ hôm nay", 24, {690, 126}, theme.green);
    for (std::size_t i = 0; i < r.goals.size() && i < 3; ++i) {
        const auto &goal = r.goals[i];
        const float top = 172 + static_cast<float>(i) * 80;
        const float appear = count(.5 + static_cast<float>(i) * .2, .4f);
        ui::panel(target, {684, top, 522, 70}, goal.done() ? sf::Color(226, 238, 208) : sf::Color(244, 238, 222), 14);
        if (goal.done()) ui::checkMark(target, {714, top + 35}, 14 * appear, theme.green);
        else {
            sf::CircleShape cross(14.f);
            cross.setOrigin(14, 14);
            cross.setPosition(714, top + 35);
            cross.setFillColor(sf::Color(214, 205, 184));
            target.draw(cross);
        }
        ui::textFit(target, font, goalText(goal), 17, {742, top + 10}, theme.ink, 330);
        const float ratio = goal.id == "nolose" ? (goal.done() ? 1.f : 0.f) : static_cast<float>(std::min(goal.progress, goal.target)) / static_cast<float>(std::max(1, goal.target));
        ui::ProgressBar::draw(target, {742, top + 44, 250, 8}, ratio * appear, goal.done() ? theme.green : theme.orange);
        ui::text(target, font, goalProgressText(goal), 12, {1000, top + 41}, theme.muted);
        const std::string reward = goal.done() ? "+" + moneyText(goal.reward) : "Chưa đạt";
        ui::text(target, font, reward, 16, {1190 - ui::textWidth(font, reward, 16), top + 24}, goal.done() ? theme.green : theme.muted);
    }
    ui::text(target, titleFont, "Thành tựu mới", 22, {690, 424}, theme.orange);
    if (r.newAchievements.empty()) {
        static const char *tips[] = {"Giao liên tiếp các đơn từ 4 sao để nhân combo.", "Nâng cấp thớt để xé bánh nhanh hơn.",
            "Đừng quên nhận quà hằng ngày ở menu chính.", "Ghế chờ êm giúp khách kiên nhẫn lâu hơn.",
            "Trộn trúng vùng xanh mỗi nhịp để món thật đều."};
        ui::paragraph(target, font, std::string("Chưa có thành tựu mới hôm nay. Mẹo: ") + tips[static_cast<std::size_t>(r.day) % 5], 16,
                      {690, 462}, 500, 22, theme.muted);
    } else {
        for (std::size_t i = 0; i < r.newAchievements.size() && i < 3; ++i) {
            const float top = 458 + static_cast<float>(i) * 54;
            ui::panel(target, {684, top, 522, 48}, sf::Color(252, 238, 190), 12);
            ui::star(target, {714, top + 24}, 16, theme.gold);
            for (const auto &info : AchievementSystem::all())
                if (info.id == r.newAchievements[i]) {
                    ui::text(target, titleFont, info.name, 18, {742, top + 4}, theme.ink);
                    ui::textFit(target, font, info.description, 12, {742, top + 27}, theme.muted, 440);
                }
        }
    }
    ui::shadowPanel(target, {50, 646, 330, 50}, {255, 250, 233, 235}, 16, 3, 40);
    ui::image(target, context_.resources, "ui/coin_icon", {64, 656, 30, 30});
    ui::text(target, font, "Ví: " + moneyText(game().progress.money), 21, {106, 659}, theme.green);
}
void ManagementState::drawShop(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &titleFont = context_.resources.titleFont();
    ui::text(target, titleFont, "Chút chăm chút cho tiệm nhỏ", 29, {30, 24}, theme.ink);
    ui::shadowPanel(target, {800, 26, 240, 46}, {255, 250, 233, 235}, 16, 3, 40);
    ui::image(target, context_.resources, "ui/coin_icon", {812, 34, 30, 30});
    ui::text(target, font, moneyText(game().progress.money), 22, {852, 37}, theme.green);
    // Dấu hiệu tab đang chọn.
    ui::panel(target, {ingredients_ ? 275.f : 30.f, 150, 225, 5}, theme.orange, 2);
    const auto size = ingredients_ ? game().data.toppings.size() : game().data.upgrades.size();
    // Gợi ý: nâng cấp rẻ nhất mà đủ tiền.
    int hint = -1, hintPrice = 1 << 30;
    if (!ingredients_)
        for (std::size_t i = 0; i < game().data.upgrades.size(); ++i) {
            const auto &u = game().data.upgrades[i];
            const int n = UpgradeSystem::level(game().progress, u.id);
            if (n >= static_cast<int>(u.levels.size())) continue;
            const int price = u.levels[static_cast<std::size_t>(n)].price;
            if (price <= game().progress.money && price < hintPrice) { hint = static_cast<int>(i); hintPrice = price; }
        }
    for (std::size_t slot = 0; slot < 6; ++slot) {
        const auto index = page_ * 6 + slot;
        if (index >= size)
            break;
        const auto card = cardRect(slot);
        const float x = card.left, y = card.top;
        const bool hovered = pointer_ && card.contains(*pointer_);
        ui::shadowPanel(target, {x, y - (hovered ? 3.f : 0.f), 600, 130}, theme.paper, 16, hovered ? 8.f : 5.f, hovered ? 70 : 45);
        if (hovered) ui::panelOutline(target, {x, y - 3, 600, 130}, {226, 168, 58, 170}, 16, 2);
        if (ingredients_) {
            const auto &t = game().data.toppings[index];
            const bool owned = game().progress.unlocked.count(t.id) > 0;
            ui::panel(target, {x + 14, y + 14, 70, 70}, {250, 241, 214}, 35);
            if (!ui::image(target, context_.resources, "toppings/" + t.id + "_icon", {x + 19, y + 19, 60, 60}))
                ui::ingredient(target, context_.resources, t, {x + 49, y + 49}, 1.7f);
            ui::text(target, titleFont, t.name, 24, {x + 100, y + 14}, theme.ink);
            ui::panel(target, {x + 100, y + 50, 56, 22}, theme.green, 11);
            ui::text(target, font, typeName(t.type), 13, {x + 128, y + 61}, {255, 250, 232}, true);
            ui::text(target, font, "Giá mỗi muỗng +" + moneyText(t.unitPrice), 15, {x + 168, y + 52}, theme.muted);
            ui::text(target, font, "Chọn khay, giữ trên tô, thả để thêm", 14, {x + 20, y + 98}, theme.muted);
            if (owned) ui::checkMark(target, {x + 562, y + 28}, 12, theme.green);
        } else {
            const auto &u = game().data.upgrades[index];
            const int level = UpgradeSystem::level(game().progress, u.id);
            const bool maxed = level >= static_cast<int>(u.levels.size());
            ui::panel(target, {x + 12, y + 12, 66, 66}, {250, 241, 214}, 33);
            ui::image(target, context_.resources, "ui/upgrade_" + u.id, {x + 14, y + 14, 62, 62});
            ui::text(target, titleFont, u.name, 22, {x + 92, y + 8}, theme.ink);
            // Chấm cấp độ.
            for (std::size_t i = 0; i < u.levels.size(); ++i) {
                sf::CircleShape pip(6.f);
                pip.setOrigin(6, 6);
                pip.setPosition(x + 100 + static_cast<float>(i) * 18, y + 48);
                pip.setFillColor(static_cast<int>(i) < level ? theme.gold : sf::Color(224, 217, 197));
                pip.setOutlineColor(static_cast<int>(i) < level ? sf::Color(184, 128, 30) : sf::Color(200, 190, 168));
                pip.setOutlineThickness(1.5f);
                target.draw(pip);
            }
            ui::text(target, font, u.description, 14, {x + 100 + static_cast<float>(u.levels.size()) * 18 + 10, y + 40}, theme.muted);
            ui::textFit(target, font, "Đang dùng: " + benefit(game().data, u, level), 14, {x + 18, y + 82}, theme.green, 350);
            ui::textFit(target, font, maxed ? "Đã đạt mốc tối đa" : "Mốc tiếp: " + benefit(game().data, u, level + 1), 14,
                        {x + 18, y + 106}, theme.muted, 350);
            if (maxed) {
                ui::panel(target, {x + 508, y + 10, 80, 24}, theme.gold, 12);
                ui::text(target, font, "TỐI ĐA", 13, {x + 548, y + 22}, {255, 250, 232}, true);
            } else if (static_cast<int>(index) == hint) {
                const float pulse = .5f + .5f * std::sin(age_ * 4.f);
                ui::panel(target, {x + 500, y + 10, 88, 24}, {static_cast<sf::Uint8>(232 + 10 * pulse), 140, 60}, 12);
                ui::text(target, font, "Nên mua", 13, {x + 544, y + 22}, {255, 250, 232}, true);
            }
        }
        // Báo còn thiếu bao nhiêu tiền cho món chưa mua nổi.
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
            if (!bought) price = u.levels[static_cast<std::size_t>(n)].price;
        }
        if (!bought && game().progress.money < price)
            ui::text(target, font, "Thiếu " + moneyText(price - game().progress.money), 13, {x + 477, y + 62}, theme.red, true);
    }
    ui::text(target, font, "Trang " + std::to_string(page_ + 1) + " / " + std::to_string((size + 5) / 6) + "   (phím mũi tên đổi trang, Tab đổi mục)", 15,
             {400, 660}, theme.muted);
    // Thông báo thành tựu vừa mở.
    if (!toasts_.empty()) {
        const auto &toast = toasts_.front();
        const float slide = ui::easeOut(std::min(1.f, toast.age / .35f)) * (toast.age > 3.6f ? 1.f - (toast.age - 3.6f) / .4f : 1.f);
        const sf::FloatRect box{560, 96 - 40.f * (1.f - std::max(0.f, slide)), 470, 52};
        ui::shadowPanel(target, box, {255, 248, 222}, 14, 4, 55);
        ui::panelOutline(target, box, theme.gold, 14, 2);
        ui::star(target, {box.left + 30, box.top + 26}, 17, theme.gold);
        for (const auto &info : AchievementSystem::all())
            if (info.id == toast.id) ui::text(target, titleFont, "Thành tựu mới: " + info.name, 19, {box.left + 58, box.top + 13}, theme.ink);
    }
    if (!game().saves.error().empty())
        ui::text(target, font, "Không lưu được tiến trình; kiểm tra quyền ghi.", 14, {30, 620},
                 {170, 65, 45});
}
void ManagementState::render(sf::RenderTarget &target) {
    ui::backdrop(target);
    ui::lightShafts(target, {0, 0, 1280, 640}, age_, 14);
    ui::dust(target, {0, 0, 1280, 640}, age_, 16, 60);
    if (kind_ == StateID::DayResult)
        drawResult(target);
    else
        drawShop(target);
    renderButtons(target);
    effects_.render(target, context_.resources.uiFont());
}
} // namespace tiem

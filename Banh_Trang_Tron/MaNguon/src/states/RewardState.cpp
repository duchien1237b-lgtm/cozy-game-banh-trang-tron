#include "states/RewardState.hpp"
#include "ui/Art.hpp"
#include <algorithm>
#include <cmath>

namespace tiem {
AchievementsState::AchievementsState(StateStack &stack, AppContext &context) : UiState(stack, context) {
    addButton("Đóng", {540, 618, 200, 42}, [this] { stack_.pop(); });
}
bool AchievementsState::handleEvent(const sf::Event &e, Pointer mouse) {
    if (e.type == sf::Event::KeyPressed && (e.key.code == sf::Keyboard::Escape || e.key.code == sf::Keyboard::Enter)) {
        stack_.pop();
        return false;
    }
    handleButtons(e, mouse);
    return false;
}
bool AchievementsState::update(float dt) {
    age_ += dt;
    return UiState::update(dt);
}
void AchievementsState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &progress = game().progress;
    sf::RectangleShape dim({1280, 720});
    dim.setFillColor({38, 26, 18, static_cast<sf::Uint8>(175 * std::min(1.f, age_ / .25f))});
    target.draw(dim);
    const sf::FloatRect panel{120, 22, 1040, 652};
    ui::shadowPanel(target, panel, theme.paper, 26, 8, 70);
    ui::panelOutline(target, panel, {226, 168, 58, 150}, 26, 2);
    const auto &list = AchievementSystem::all();
    int unlocked = 0;
    for (const auto &a : list) unlocked += progress.achievements.count(a.id) ? 1 : 0;
    ui::text(target, context_.resources.titleFont(), "Thành tựu", 36, {640, 62}, theme.ink, true);
    ui::text(target, font, "Đã mở " + std::to_string(unlocked) + " / " + std::to_string(list.size()), 16, {640, 98}, theme.muted, true);
    ui::ProgressBar::draw(target, {500, 112, 280, 8}, static_cast<float>(unlocked) / static_cast<float>(std::max<std::size_t>(1, list.size())), theme.gold);
    for (std::size_t i = 0; i < list.size(); ++i) {
        const auto &a = list[i];
        const bool done = progress.achievements.count(a.id) > 0;
        const float x = 146 + static_cast<float>(i % 2) * 506, y = 134 + static_cast<float>(i / 2) * 68;
        const sf::FloatRect cell{x, y, 488, 62};
        ui::panel(target, cell, done ? sf::Color(252, 238, 190) : sf::Color(238, 232, 214), 14);
        if (done) ui::panelOutline(target, cell, {226, 168, 58, 200}, 14, 2);
        ui::star(target, {x + 34, y + 31}, 21, done ? theme.gold : sf::Color(196, 188, 168));
        ui::text(target, context_.resources.titleFont(), a.name, 18, {x + 70, y + 6}, done ? theme.ink : theme.muted);
        ui::textFit(target, font, a.description, 13, {x + 70, y + 31}, theme.muted, done ? 400 : 270);
        if (done) {
            ui::checkMark(target, {x + 458, y + 31}, 12, theme.green);
        } else if (a.target > 1) {
            const int current = std::min(a.target, AchievementSystem::current(game().data, progress, a.id));
            const std::string label = a.id == "rich" ? moneyText(current) : std::to_string(current) + "/" + std::to_string(a.target);
            ui::ProgressBar::draw(target, {x + 350, y + 36, 120, 7}, static_cast<float>(current) / static_cast<float>(a.target), theme.orange);
            ui::text(target, font, label, 12, {x + 410, y + 22}, theme.muted, true);
        } else {
            ui::text(target, font, "Chưa đạt", 13, {x + 420, y + 31}, theme.muted, true);
        }
    }
    renderButtons(target);
}

DailyRewardState::DailyRewardState(StateStack &stack, AppContext &context)
    : UiState(stack, context), today_(DailyReward::today()) {
    addButton("Nhận quà", {490, 516, 300, 54}, [this] { claim(); }).setEnabled(DailyReward::canClaim(game().progress, today_));
    addButton("Đóng", {540, 582, 200, 40}, [this] { stack_.pop(); });
}
void DailyRewardState::claim() {
    if (claimed_ || !DailyReward::canClaim(game().progress, today_)) return;
    reward_ = DailyReward::claim(game().data, game().progress, today_);
    if (reward_ <= 0) return;
    claimed_ = true;
    game().save();
    sound("reward_claim");
    effects_.say("+" + moneyText(reward_), {640, 470}, ui::Theme{}.gold, 2.2f, 36, true);
    effects_.sparkle({640, 400}, {255, 214, 98}, 36);
    buttons_[0].setEnabled(false);
}
bool DailyRewardState::handleEvent(const sf::Event &e, Pointer mouse) {
    if (e.type == sf::Event::KeyPressed) {
        if (e.key.code == sf::Keyboard::Escape) { stack_.pop(); return false; }
        if (e.key.code == sf::Keyboard::Enter || e.key.code == sf::Keyboard::Space) {
            if (buttons_[0].enabled()) claim(); else stack_.pop();
            return false;
        }
    }
    handleButtons(e, mouse);
    return false;
}
bool DailyRewardState::update(float dt) {
    age_ += dt;
    effects_.update(dt);
    return UiState::update(dt);
}
void DailyRewardState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &progress = game().progress;
    sf::RectangleShape dim({1280, 720});
    dim.setFillColor({38, 26, 18, static_cast<sf::Uint8>(175 * std::min(1.f, age_ / .25f))});
    target.draw(dim);
    const sf::FloatRect panel{220, 80, 840, 560};
    ui::shadowPanel(target, panel, theme.paper, 28, 8, 70);
    ui::panelOutline(target, panel, {226, 168, 58, 150}, 28, 2);
    ui::text(target, context_.resources.titleFont(), "Quà hằng ngày", 38, {640, 128}, theme.ink, true);
    const bool can = DailyReward::canClaim(progress, today_) && !claimed_;
    const bool missed = progress.lastDaily >= 0 && progress.lastDaily < today_ - 1 && can;
    // Mốc đã nhận / mốc hôm nay.
    int received = -1, current = -1;
    if (can) current = DailyReward::nextIndex(progress, today_), received = current - 1;
    else received = (progress.dailyStreak - 1 + 7) % 7;
    std::string line = can ? (missed ? "Bạn đã lỡ một ngày nên chuỗi quay về ngày 1. Nhận quà hôm nay để bắt đầu lại!"
                                     : "Nhận quà mỗi ngày để lên mốc thưởng cao hơn!")
                           : "Đã nhận quà hôm nay. Hẹn gặp lại bạn vào ngày mai!";
    ui::textFit(target, font, line, 17, {640, 172}, theme.muted, 760, true);
    for (int i = 0; i < 7; ++i) {
        const float x = 254 + static_cast<float>(i) * 112, y = 214;
        const sf::FloatRect card{x, y, 100, 250};
        const bool got = i <= received;
        const bool today = i == current;
        const bool finalDay = i == 6;
        ui::shadowPanel(target, card, got ? sf::Color(226, 238, 208) : finalDay ? sf::Color(255, 238, 190) : sf::Color(250, 241, 214), 16, 4, 40);
        if (today) {
            const float pulse = .5f + .5f * std::sin(age_ * 5.f);
            ui::panelOutline(target, card, {226, 168, 58, static_cast<sf::Uint8>(160 + 95 * pulse)}, 16, 3 + 2 * pulse);
            ui::radial(target, {x + 50, y + 110}, {80, 90}, {255, 214, 98, static_cast<sf::Uint8>(70 + 50 * pulse)}, {255, 214, 98, 0}, true);
        }
        ui::text(target, context_.resources.titleFont(), "Ngày " + std::to_string(i + 1), 18, {x + 50, y + 22}, got ? theme.green : theme.ink, true);
        ui::image(target, context_.resources, "ui/coin_icon", {x + 22, y + 60, 56, 56});
        if (finalDay) ui::star(target, {x + 50, y + 142}, 14, theme.gold);
        ui::text(target, font, moneyText(DailyReward::amount(game().data, i)), 15, {x + 50, y + 180}, theme.ink, true);
        if (got) ui::checkMark(target, {x + 50, y + 218}, 14, theme.green);
        else if (today) ui::text(target, font, "Hôm nay", 14, {x + 50, y + 218}, theme.orange, true);
    }
    ui::text(target, font, "Chuỗi hiện tại: " + std::to_string(progress.dailyStreak) + " ngày", 16, {640, 488}, theme.green, true);
    renderButtons(target);
    effects_.render(target, context_.resources.titleFont());
}
} // namespace tiem

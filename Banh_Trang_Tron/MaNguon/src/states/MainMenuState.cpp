#include "states/MainMenuState.hpp"
#include "ui/Art.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace tiem {
MainMenuState::MainMenuState(StateStack &stack, AppContext &context) : UiState(stack, context) {
    if (context_.audio)
        context_.audio->music("menu");
    const auto &p = game().progress;
    addButton(p.day == 1 && p.served == 0 ? "Bắt đầu mở quán" : "Tiếp tục mở quán ngày " + std::to_string(p.day),
              {82, 334, 360, 50}, [this] { stack_.replace(StateID::Playing); });
    addButton("Chơi lại từ đầu", {82, 394, 360, 44}, [this] {
        confirmReset_ = true;
        onCovered();
        for (auto &button : resetButtons_) button.cancelInteraction();
    });
    addButton("Nâng cấp", {82, 448, 172, 44}, [this] { stack_.replace(StateID::UpgradeShop); });
    addButton("Cài đặt", {270, 448, 172, 44}, [this] { stack_.push(StateID::Settings); });
    addButton("Đánh giá khách", {82, 502, 172, 44}, [this] { stack_.push(StateID::Reviews); });
    addButton("Thoát", {270, 502, 172, 44}, [this] {
        game().save();
        context_.window.close();
    });
    addButton("Quà hằng ngày", {18, 14, 108, 94}, [this] { stack_.push(StateID::DailyReward); })
        .setIcon("assets/images/cozy/ui/daily_gift_icon.png");
    addButton("Thành tựu", {1154, 14, 108, 94}, [this] { stack_.push(StateID::Achievements); })
        .setIcon("assets/images/cozy/ui/achievement_icon.png");
    resetButtons_.emplace_back(context_.resources.uiFont(), "Hủy", sf::FloatRect(400, 431, 220, 48),
        [this] { confirmReset_ = false; });
    resetButtons_.emplace_back(context_.resources.uiFont(), "Bắt đầu lại", sf::FloatRect(660, 431, 220, 48), [this] {
        if (game().restart()) stack_.replace(StateID::Playing);
    });
    for (auto &button : resetButtons_) {
        button.setResources(context_.resources);
        button.setClickSound(context_.playClick);
    }
    // Tự mở quà hằng ngày một lần mỗi lần mở game khi có quà chưa nhận.
    if (!game().dailyPrompted && DailyReward::canClaim(p, DailyReward::today())) {
        game().dailyPrompted = true;
        stack_.push(StateID::DailyReward);
    }
}
bool MainMenuState::handleEvent(const sf::Event &e, Pointer mouse) {
    if (confirmReset_) {
        if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) {
            confirmReset_ = false;
            for (auto &button : resetButtons_) button.cancelInteraction();
        } else for (auto &button : resetButtons_) button.handleEvent(e, mouse);
        return false;
    }
    if (e.type == sf::Event::KeyPressed && (e.key.code == sf::Keyboard::Enter || e.key.code == sf::Keyboard::Space)) {
        sound("ui_click");
        stack_.replace(StateID::Playing);
        return false;
    }
    handleButtons(e, mouse);
    return false;
}
bool MainMenuState::update(float dt) {
    time_ += dt;
    if (confirmReset_) for (auto &button : resetButtons_) button.update(dt);
    return UiState::update(dt);
}
void MainMenuState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &titleFont = context_.resources.titleFont();
    const auto &p = game().progress;
    if (!ui::background(target, context_.resources, "background/menu", {0, 0, 1280, 720}))
        ui::backdrop(target);
    // Ánh nắng, bụi bay và vignette nhẹ cho cảm giác không gian thật hơn.
    ui::lightShafts(target, {0, 0, 1280, 700}, time_, 30);
    ui::dust(target, {0, 0, 1280, 720}, time_, 34, 100);
    ui::vignette(target, 40);
    const float logoFloat = std::sin(time_ * 1.3f) * 3.f;
    if (!ui::image(target, context_.resources, "ui/game_title_logo", {64, 79 + logoFloat, 395, 251})) {
        ui::text(target, titleFont, "Góc Nhỏ", 46, {82, 156}, theme.ink);
        ui::text(target, titleFont, "Bánh Tráng", 46, {82, 218}, theme.ink);
    }
    // Lấp lánh nhấp nháy quanh logo.
    for (int i = 0; i < 4; ++i) {
        const float phase = std::fmod(time_ * .5f + static_cast<float>(i) * .27f, 1.f);
        const float a = std::sin(phase * 3.14159f);
        const sf::Vector2f spots[] = {{150, 118}, {420, 150}, {330, 270}, {98, 250}};
        ui::radial(target, spots[i] + sf::Vector2f(0, logoFloat), {16 * a, 16 * a}, {255, 245, 190, static_cast<sf::Uint8>(200 * a)}, {255, 245, 190, 0}, true);
    }
    const auto shadow = [&](sf::Vector2f center, sf::Vector2f size) {
        sf::CircleShape ellipse(1.f, 60);
        ellipse.setOrigin(1.f, 1.f);
        ellipse.setPosition(center);
        ellipse.setScale(size.x, size.y);
        ellipse.setFillColor({79, 52, 32, 45});
        target.draw(ellipse);
    };
    shadow({782, 663}, {194, 20});
    shadow({1090, 665}, {53, 11});
    shadow({1168, 679}, {37, 9});
    ui::image(target, context_.resources, "props/cart", {540, 260, 480, 440});
    // Người bán nhún nhẹ như đang vẫy chào; chú chó lắc lư theo nhịp khác.
    const float vendorBob = std::sin(time_ * 2.4f) * 3.f;
    ui::image(target, context_.resources, "vendor/vendor_full_wave_01", {985, 331 + vendorBob, 210, 365}, std::sin(time_ * 1.2f) * .8f);
    ui::image(target, context_.resources, "props/mascot_dog_sit", {1120, 580 + std::sin(time_ * 3.1f + 1.f) * 2.f, 105, 105}, std::sin(time_ * 2.2f) * 2.f);
    // Bong bóng thoại của người bán: lời chào và mẹo xoay vòng.
    static const char *tips[] = {"Giao liên tiếp đơn 4 sao để nhân combo, tip tăng dần!",
        "Giờ cao điểm: khách đông hơn nhưng tip thêm 20%.",
        "Thớt tốt giúp xé bánh nhanh hơn. Ghé cửa hàng nhé!",
        "Trộn trúng vùng xanh thật nhiều để món thật đều.",
        "Phím 1-6 chọn nhanh khay, Space để trộn và giao món.",
        "Nhận quà mỗi ngày để mở topping mới sớm hơn."};
    const int tipIndex = static_cast<int>(time_ / 6.f) % 6;
    const float tipPhase = std::fmod(time_, 6.f);
    const float tipAlpha = std::min(1.f, std::min(tipPhase / .4f, (6.f - tipPhase) / .4f));
    const sf::FloatRect bubble{905, 222 + vendorBob * .5f, 330, 104};
    if (!ui::frame(target, context_.resources, "ui/cozy_frames/chat_bubble", bubble, {48, 64}, {15, 20}))
        ui::panel(target, bubble, theme.paper, 18);
    ui::text(target, context_.resources.dialogFont(), p.day == 1 && p.served == 0 ? "Chào bạn, mình bắt đầu nhé!" : "Chào mừng trở lại, ngày " + std::to_string(p.day) + "!", 16,
             {bubble.left + 18, bubble.top + 16}, theme.ink);
    ui::paragraph(target, font, tips[tipIndex], 14, {bubble.left + 18, bubble.top + 44}, bubble.width - 40, 19,
                  {theme.muted.r, theme.muted.g, theme.muted.b, static_cast<sf::Uint8>(255 * tipAlpha)});
    renderButtons(target);
    // Huy hiệu thông báo trên nút.
    const auto badge = [&](sf::Vector2f at, const std::string &label) {
        sf::CircleShape dot(11.f);
        dot.setOrigin(11, 11);
        dot.setPosition(at);
        dot.setFillColor(theme.red);
        dot.setOutlineColor({255, 250, 236});
        dot.setOutlineThickness(2.f);
        target.draw(dot);
        ui::text(target, font, label, 13, at + sf::Vector2f(0, -1), {255, 250, 236}, true);
    };
    if (DailyReward::canClaim(p, DailyReward::today()))
        badge({99, 24 + std::sin(time_ * 6.f) * 2.f}, "!");
    ui::panel(target, {1179, 68, 58, 18}, {89, 112, 65, 245}, 9);
    ui::text(target, font, std::to_string(p.achievements.size()) + "/" +
                 std::to_string(AchievementSystem::all().size()), 11, {1208, 77}, {255, 248, 219}, true);
    int unanswered = 0;
    for (const auto &review : p.reviews) unanswered += review.reply.empty() ? 1 : 0;
    if (unanswered > 0) badge({248, 506}, std::to_string(std::min(unanswered, 99)));
    // Các chỉ số nhanh dưới trái.
    const auto chip = [&](float x, float w, const std::string &label, const std::string &value) {
        ui::shadowPanel(target, {x, 650, w, 46}, {255, 250, 233, 232}, 14, 3, 40);
        ui::text(target, font, label, 11, {x + 14, 656}, theme.muted);
        ui::text(target, font, value, 18, {x + 14, 671}, theme.green);
    };
    chip(20, 190, "Ví", moneyText(p.money));
    std::ostringstream best;
    best << std::fixed << std::setprecision(1) << p.bestRating << " / 5";
    chip(220, 140, "Sao cao nhất", p.bestRating > 0 ? best.str() : "Chưa có");
    chip(370, 140, "Đã phục vụ", std::to_string(p.served) + " khách");
    ui::image(target, context_.resources, "ui/coin_icon", {190, 664, 18, 18});
    if (!game().saves.error().empty())
        ui::text(target, font, game().saves.error(), 14, {82, 612}, {168, 67, 44});
    if (confirmReset_) {
        sf::RectangleShape dim({1280, 720});
        dim.setFillColor({40, 31, 22, 175});
        target.draw(dim);
        ui::shadowPanel(target, {300, 222, 680, 310}, theme.paper, 26, 8, 70);
        ui::text(target, titleFont, "Chơi lại từ đầu?", 32, {640, 267}, theme.ink, true);
        ui::text(target, font, "Tiệm sẽ quay về ngày 1 với số tiền ban đầu.", 20, {640, 326}, theme.ink, true);
        ui::text(target, font, "Nâng cấp, topping đã mở và đánh giá sẽ được làm mới.", 17, {640, 362}, theme.muted, true);
        ui::text(target, font, "Thành tựu và chuỗi quà hằng ngày vẫn được giữ lại.", 15, {640, 392}, theme.green, true);
        for (const auto &button : resetButtons_) button.render(target);
        if (!game().saves.error().empty())
            ui::text(target, font, "Chưa thể lưu lượt chơi mới. Hãy thử lại.", 16, {640, 506}, {168, 67, 44}, true);
    }
}
} // namespace tiem

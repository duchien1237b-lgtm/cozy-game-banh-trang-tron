#include "states/MainMenuState.hpp"
#include "ui/Art.hpp"
#include <algorithm>
#include <cmath>
namespace tiem {
MainMenuState::MainMenuState(StateStack &stack, AppContext &context) : UiState(stack, context) {
    if (context_.audio)
        context_.audio->music("menu");
    addButton("Tiếp tục mở quán ngày " + std::to_string(game().progress.day), {82, 340, 360, 50},
              [this] { stack_.replace(StateID::Playing); });
    addButton("Chơi lại từ đầu", {82, 402, 360, 46}, [this] {
        confirmReset_ = true;
        onCovered();
        for (auto &button : resetButtons_) button.cancelInteraction();
    });
    addButton("Nâng cấp", {82, 460, 172, 46}, [this] { stack_.replace(StateID::UpgradeShop); });
    addButton("Cài đặt", {270, 460, 172, 46}, [this] { stack_.push(StateID::Settings); });
    addButton("Đánh giá khách", {82, 518, 172, 46}, [this] { stack_.push(StateID::Reviews); });
    addButton("Thoát", {270, 518, 172, 46}, [this] {
        game().save();
        context_.window.close();
    });
    resetButtons_.emplace_back(context_.resources.uiFont(), "Hủy", sf::FloatRect(400, 431, 220, 48),
        [this] { confirmReset_ = false; });
    resetButtons_.emplace_back(context_.resources.uiFont(), "Bắt đầu lại", sf::FloatRect(660, 431, 220, 48), [this] {
        if (game().restart()) stack_.replace(StateID::Playing);
    });
    for (auto &button : resetButtons_) {
        button.setResources(context_.resources);
        button.setClickSound(context_.playClick);
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
    if (!ui::image(target, context_.resources, "ui/game_title_logo", {64, 79, 395, 251})) {
        ui::text(target, titleFont, "Góc Nhỏ", 46, {82, 156}, theme.ink);
        ui::text(target, titleFont, "Bánh Tráng", 46, {82, 218}, theme.ink);
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
    ui::image(target, context_.resources, "vendor/vendor_full_wave_01", {985, 331, 210, 365});
    ui::image(target, context_.resources, "props/mascot_dog_sit", {1120, 580, 105, 105});
    ui::panel(target, {20, 671, 300, 37}, {255, 250, 233, 225}, 12);
    ui::text(target, font, "Ví: " + moneyText(p.money), 20, {35, 679}, theme.green);
    renderButtons(target);
    if (!game().saves.error().empty())
        ui::text(target, font, game().saves.error(), 14, {82, 656}, {168, 67, 44});
    if (confirmReset_) {
        sf::RectangleShape dim({1280, 720});
        dim.setFillColor({40, 31, 22, 175});
        target.draw(dim);
        ui::panel(target, {300, 222, 680, 310}, theme.paper, 26);
        ui::text(target, titleFont, "Chơi lại từ đầu?", 32, {640, 267}, theme.ink, true);
        ui::text(target, font, "Tiệm sẽ quay về ngày 1 với số tiền ban đầu.", 20, {640, 326}, theme.ink, true);
        ui::text(target, font, "Các nâng cấp, topping đã mở và đánh giá sẽ được làm mới.", 18,
                 {640, 364}, theme.muted, true);
        for (const auto &button : resetButtons_) button.render(target);
        if (!game().saves.error().empty())
            ui::text(target, font, "Chưa thể lưu lượt chơi mới. Hãy thử lại.", 16, {640, 506}, {168, 67, 44}, true);
    }
}
} // namespace tiem

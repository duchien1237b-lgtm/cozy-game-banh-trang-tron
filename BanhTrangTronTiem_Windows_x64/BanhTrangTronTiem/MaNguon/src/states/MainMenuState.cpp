#include "states/MainMenuState.hpp"
#include "ui/Art.hpp"
#include <algorithm>
#include <cmath>
namespace tiem {
MainMenuState::MainMenuState(StateStack &stack, AppContext &context) : UiState(stack, context) {
    if (context_.audio)
        context_.audio->music("cozy_shop");
    addButton("Mở quán • Ngày " + std::to_string(game().progress.day), {82, 358, 360, 56},
              [this] { stack_.replace(StateID::Playing); });
    addButton("Nâng cấp", {82, 434, 172, 48}, [this] { stack_.replace(StateID::UpgradeShop); });
    addButton("Cài đặt", {270, 434, 172, 48}, [this] { stack_.push(StateID::Settings); });
    addButton("Cách chơi", {82, 501, 172, 48}, [this] {
        help_ = true;
        onCovered();
    });
    addButton("Thoát", {270, 501, 172, 48}, [this] {
        game().save();
        context_.window.close();
    });
}
bool MainMenuState::handleEvent(const sf::Event &e, Pointer mouse) {
    if (help_) {
        if ((e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) ||
            (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left && mouse &&
             sf::FloatRect(490, 548, 300, 48).contains(*mouse)))
            help_ = false;
        return false;
    }
    handleButtons(e, mouse);
    return false;
}
bool MainMenuState::update(float dt) {
    time_ += dt;
    return UiState::update(dt);
}
void MainMenuState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &p = game().progress;
    ui::backdrop(target);
    ui::text(target, font, "MỘT GÓC NHỎ • MỘT MÓN QUEN", 16, {82, 99}, theme.green);
    ui::text(target, font, "Bánh tráng", 55, {77, 155}, theme.ink);
    ui::text(target, font, "trộn tiệm", 55, {77, 223}, theme.ink);
    ui::text(target, font, "Chút cay, chút chua, thật nhiều niềm vui.", 19, {82, 308}, theme.muted);
    ui::panel(target, {615, 120, 585, 480}, {206, 176, 134}, 28);
    ui::panel(target, {609, 112, 585, 480}, {241, 219, 182}, 28);
    ui::text(target, font, "TIỆM NHỎ, NIỀM VUI TO", 20, {901, 152}, theme.ink, true);
    for (std::size_t i = 0; i < std::min<std::size_t>(3, game().data.toppings.size()); ++i) {
        const float x = 706 + static_cast<float>(i) * 190;
        ui::panel(target, {x - 63, 188, 126, 88}, theme.paper, 14);
        ui::ingredient(target, game().data.toppings[i], {x, 225}, 1.7f);
        ui::text(target, font, game().data.toppings[i].name, 13, {x, 259}, theme.muted, true);
    }
    Bowl sample;
    for (const auto &t : game().data.toppings)
        if (t.initial)
            sample.portions.push_back(t.id);
    for (int i = 0; i < 2; ++i)
        sample.portions.push_back(game().data.toppings.front().id);
    ui::bowl(target, game().data, sample, {901, 421 + std::sin(time_) * 3}, 1.15f);
    ui::text(target, font, "Xé bánh • Thêm topping • Trộn • Giao món", 16, {901, 565}, theme.muted, true);
    ui::text(target, font, "Ví: " + moneyText(p.money), 20, {82, 584}, theme.green);
    ui::text(target, font, "Đã phục vụ " + std::to_string(p.served) + " khách", 16, {82, 666}, theme.ink);
    ui::text(target, font, "Tiến trình tự lưu sau mỗi món và mỗi lần mua.", 15, {710, 670}, theme.muted);
    renderButtons(target);
    if (!game().saves.error().empty())
        ui::text(target, font, game().saves.error(), 14, {82, 622}, {168, 67, 44});
    if (help_) {
        sf::RectangleShape dim({1280, 720});
        dim.setFillColor({40, 31, 22, 160});
        target.draw(dim);
        ui::panel(target, {210, 125, 860, 495}, theme.paper, 26);
        ui::text(target, font, "Một phần bánh tráng thật ngon", 30, {640, 176}, theme.ink, true);
        const char *lines[] = {"1. Bấm chọn khách. Mỗi topping trên đơn có số muỗng cần làm.",
                               "2. Giữ ở chấm xanh, vuốt hết 3 vạch nét đứt để xé bánh.",
                               "3. Kéo topping vào tô. Với sốt, giữ chai trên tô để rưới.",
                               "4. Bấm Trộn món, đánh 3 nhịp khi vạch chạy vào vùng xanh.",
                               "5. Bấm Giao món hoặc kéo tô đến khách. Nhận tiền và tip!",
                               "6. Hết 120 giây: xem tổng kết, nâng cấp rồi mở ngày mới."};
        for (int i = 0; i < 6; ++i)
            ui::text(target, font, lines[i], 19, {251, 237 + static_cast<float>(i) * 43}, theme.ink);
        ui::text(target, font, "ESC để nghỉ tay. Đơn càng đúng, tiền thưởng càng nhiều.", 17, {640, 513},
                 theme.muted, true);
        ui::panel(target, {490, 548, 300, 48}, theme.green, 12);
        ui::text(target, font, "Đã hiểu", 20, {640, 572}, theme.paper, true);
    }
}
} // namespace tiem

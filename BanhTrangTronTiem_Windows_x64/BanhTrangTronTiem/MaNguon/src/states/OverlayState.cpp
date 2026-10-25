#include "states/OverlayState.hpp"
#include "ui/Effects.hpp"
#include <algorithm>
namespace tiem {
OverlayState::OverlayState(StateStack &stack, AppContext &context, StateID kind)
    : UiState(stack, context), kind_(kind) {
    if (kind_ == StateID::Pause) {
        addButton("Tiếp tục", {470, 300, 340, 52}, [this] { stack_.pop(); });
        addButton("Âm thanh", {470, 373, 340, 52}, [this] { stack_.push(StateID::Settings); });
        addButton("Về menu", {470, 446, 340, 52}, [this] {
            if (confirmExit_) {
                game().save();
                returnToMenu();
            } else {
                confirmExit_ = true;
                buttons_[2].setLabel("Xác nhận về menu");
            }
        });
    } else
        addButton("Lưu và quay lại", {470, 480, 340, 52}, [this] {
            game().save();
            stack_.pop();
        });
}
bool OverlayState::handleEvent(const sf::Event &e, Pointer mouse) {
    if (e.type == sf::Event::LostFocus || e.type == sf::Event::MouseLeft)
        slider_ = -1;
    if (kind_ == StateID::Settings) {
        if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left && mouse) {
            if (sf::FloatRect(435, 315, 410, 38).contains(*mouse))
                slider_ = 0;
            if (sf::FloatRect(435, 409, 410, 38).contains(*mouse))
                slider_ = 1;
        }
        if (slider_ >= 0 && mouse &&
            (e.type == sf::Event::MouseMoved || e.type == sf::Event::MouseButtonPressed)) {
            float &volume = slider_ == 0 ? game().progress.music : game().progress.sfx;
            volume = std::clamp((mouse->x - 445) / 390.f * 100.f, 0.f, 100.f);
            if (context_.audio)
                context_.audio->update(game().progress.music, game().progress.sfx);
        }
        if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left &&
            slider_ >= 0) {
            const int old = slider_;
            slider_ = -1;
            game().save();
            if (old == 1)
                sound("ui_click");
            return false;
        }
    }
    handleButtons(e, mouse);
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) {
        if (kind_ == StateID::Settings)
            game().save();
        stack_.pop();
    }
    return false;
}
void OverlayState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    sf::RectangleShape dim({1280, 720});
    dim.setFillColor({43, 33, 25, 170});
    target.draw(dim);
    ui::panel(target, {355, 157, 570, 423}, theme.paper, 26);
    ui::text(target, font, kind_ == StateID::Pause ? "Nghỉ tay một chút" : "Âm thanh", 33, {640, 217},
             theme.ink, true);
    if (kind_ == StateID::Pause) {
        ui::text(target, font, "Khách và đồng hồ đều đang đợi bạn.", 18, {640, 265}, theme.muted, true);
        if (confirmExit_)
            ui::text(target, font, "Giữ tiền đã kiếm; ngày đang chơi sẽ bắt đầu lại.", 16, {640, 538},
                     theme.muted, true);
    } else {
        ui::text(target, font, "Nhạc nền: " + std::to_string(static_cast<int>(game().progress.music)) + "%",
                 19, {445, 281}, theme.ink);
        ui::text(target, font, "Hiệu ứng: " + std::to_string(static_cast<int>(game().progress.sfx)) + "%", 19,
                 {445, 375}, theme.ink);
        for (int i = 0; i < 2; ++i) {
            const float volume = i == 0 ? game().progress.music : game().progress.sfx,
                        y = 330 + static_cast<float>(i) * 94;
            ui::ProgressBar::draw(target, {445, y, 390, 10}, volume / 100, theme.green);
            sf::CircleShape knob(11);
            knob.setOrigin(11, 11);
            knob.setPosition(445 + 390 * volume / 100, y + 5);
            knob.setFillColor(theme.green);
            target.draw(knob);
        }
    }
    renderButtons(target);
    if (!game().saves.error().empty())
        ui::text(target, font, "Chưa lưu được. Kiểm tra quyền ghi thư mục dữ liệu.", 14, {640, 557},
                 {175, 67, 45}, true);
}
} // namespace tiem

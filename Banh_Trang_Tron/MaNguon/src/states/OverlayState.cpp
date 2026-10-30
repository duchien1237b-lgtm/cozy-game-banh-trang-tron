#include "states/OverlayState.hpp"
#include "ui/Effects.hpp"
#include <algorithm>
namespace tiem {
OverlayState::OverlayState(StateStack &stack, AppContext &context, StateID kind)
    : UiState(stack, context), kind_(kind) {
    if (kind_ == StateID::Pause) {
        addButton("Tiếp tục", {470, 300, 340, 52}, [this] { stack_.pop(); });
        addButton("Cài đặt", {470, 373, 340, 52}, [this] { stack_.push(StateID::Settings); });
        addButton("Về menu", {470, 446, 340, 52}, [this] {
            if (confirmExit_) {
                game().save();
                returnToMenu();
            } else {
                confirmExit_ = true;
                buttons_[2].setLabel("Xác nhận về menu");
            }
        });
    } else {
        addButton("Cách chơi", {470, 466, 340, 44}, [this] {
            help_ = true;
            slider_ = -1;
            onCovered();
        });
        addButton("Lưu và quay lại", {470, 524, 340, 44}, [this] {
            game().save();
            stack_.pop();
        });
    }
}
bool OverlayState::handleEvent(const sf::Event &e, Pointer mouse) {
    if (help_) {
        if ((e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) ||
            (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left && mouse &&
             sf::FloatRect(490, 548, 300, 48).contains(*mouse))) help_ = false;
        return false;
    }
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
    ui::panel(target, {355, 157, 570, kind_ == StateID::Settings ? 470.f : 423.f}, theme.paper, 26);
    ui::text(target, context_.resources.titleFont(), kind_ == StateID::Pause ? "Nghỉ tay một chút" : "Cài đặt", 33, {640, 217},
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
        ui::text(target, font, "Chưa lưu được. Kiểm tra quyền ghi thư mục dữ liệu.", 14, {640, kind_ == StateID::Settings ? 604.f : 557.f},
                 {175, 67, 45}, true);
    if (help_) {
        target.draw(dim);
        ui::panel(target, {210, 125, 860, 495}, theme.paper, 26);
        ui::text(target, context_.resources.titleFont(), "Một phần bánh tráng thật ngon", 30, {640, 176}, theme.ink, true);
        const char *lines[] = {"1. Chọn khách và xem số muỗng topping trên đơn.",
            "2. Giữ và rê trên bánh để xé hai giai đoạn. Thớt tốt giúp xé nhanh.",
            "3. Chọn topping, giữ trên tô rồi thả: thanh lực 2,4 giây, 1–3 muỗng.",
            "4. Bấm Trộn món. Vùng xanh đổi vị trí sau mỗi nhịp.",
            "5. Giao món để nhận tiền, tip và đánh giá. Chị Dung tip 100%.",
            "6. Mỗi ngày dài 3 phút. Hết ngày: tổng kết và nâng cấp tiệm."};
        for (int i = 0; i < 6; ++i)
            ui::text(target, font, lines[i], 19, {251, 237 + static_cast<float>(i) * 43}, theme.ink);
        ui::text(target, font, "ESC để nghỉ tay hoặc quay lại Cài đặt.", 17, {640, 513}, theme.muted, true);
        ui::panel(target, {490, 548, 300, 48}, theme.green, 12);
        ui::text(target, font, "Quay lại cài đặt", 20, {640, 572}, theme.paper, true);
    }
}
} // namespace tiem

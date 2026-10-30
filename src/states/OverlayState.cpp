#include "states/OverlayState.hpp"
#include "ui/Art.hpp"
#include "ui/Effects.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
namespace tiem {
namespace {
// Vị trí thanh trượt âm lượng: hộp chạm rộng hơn thanh vẽ để dễ bấm.
constexpr float musicBarY = 290.f, sfxBarY = 384.f;
}
OverlayState::OverlayState(StateStack &stack, AppContext &context, StateID kind)
    : UiState(stack, context), kind_(kind) {
    if (kind_ == StateID::Pause) {
        addButton("Tiếp tục", {470, 442, 340, 48}, [this] { stack_.pop(); });
        addButton("Cài đặt", {470, 498, 340, 48}, [this] { stack_.push(StateID::Settings); });
        addButton("Về menu", {470, 554, 340, 48}, [this] {
            if (confirmExit_) {
                game().save();
                returnToMenu();
            } else {
                confirmExit_ = true;
                buttons_[2].setLabel("Xác nhận về menu");
            }
        });
    } else {
        addButton("", {470, 430, 340, 44}, [this] {
            const bool on = !game().progress.fullscreen;
            if (context_.setFullscreen) context_.setFullscreen(on);
            game().progress.fullscreen = on;
            game().save();
            refreshLabels();
        });
        addButton("", {470, 484, 340, 44}, [this] {
            game().progress.tutorialDone = !game().progress.tutorialDone;
            game().save();
            refreshLabels();
        });
        addButton("Cách chơi và phím tắt", {470, 538, 340, 44}, [this] {
            help_ = true;
            slider_ = -1;
            onCovered();
        });
        addButton("Lưu và quay lại", {470, 592, 340, 44}, [this] {
            game().save();
            stack_.pop();
        });
        refreshLabels();
    }
    helpBack_ = std::make_unique<ui::Button>(context_.resources.uiFont(), "Quay lại", sf::FloatRect(490, 566, 300, 48),
                                             [this] { help_ = false; });
    helpBack_->setResources(context_.resources);
    helpBack_->setClickSound(context_.playClick);
}
void OverlayState::refreshLabels() {
    if (kind_ != StateID::Settings || buttons_.size() < 2) return;
    buttons_[0].setLabel(std::string("Toàn màn hình (F11): ") + (game().progress.fullscreen ? "Bật" : "Tắt"));
    buttons_[1].setLabel(std::string("Hướng dẫn khi chơi: ") + (game().progress.tutorialDone ? "Tắt" : "Bật"));
}
bool OverlayState::update(float dt) {
    age_ += dt;
    refreshLabels();
    if (help_) {
        helpBack_->update(dt);
        return false;
    }
    return UiState::update(dt);
}
bool OverlayState::handleEvent(const sf::Event &e, Pointer mouse) {
    if (help_) {
        if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) help_ = false;
        else helpBack_->handleEvent(e, mouse);
        return false;
    }
    if (e.type == sf::Event::LostFocus || e.type == sf::Event::MouseLeft)
        slider_ = -1;
    if (kind_ == StateID::Settings) {
        if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left && mouse) {
            if (sf::FloatRect(435, musicBarY - 15, 410, 38).contains(*mouse))
                slider_ = 0;
            if (sf::FloatRect(435, sfxBarY - 15, 410, 38).contains(*mouse))
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
    const auto &titleFont = context_.resources.titleFont();
    sf::RectangleShape dim({1280, 720});
    dim.setFillColor({43, 33, 25, static_cast<sf::Uint8>(175 * std::min(1.f, age_ / .2f))});
    target.draw(dim);
    const bool pause = kind_ == StateID::Pause;
    const sf::FloatRect panel = pause ? sf::FloatRect{330, 100, 620, 540} : sf::FloatRect{355, 120, 570, 560};
    ui::shadowPanel(target, panel, theme.paper, 26, 8, 70);
    ui::panelOutline(target, panel, {226, 168, 58, 140}, 26, 2);
    if (pause) {
        ui::text(target, titleFont, "Nghỉ tay một chút", 34, {640, 148}, theme.ink, true);
        ui::text(target, font, "Khách và đồng hồ đều đang đợi bạn.", 17, {640, 188}, theme.muted, true);
        const auto &day = game().liveDay;
        const std::pair<std::string, std::string> stats[] = {
            {"Đã phục vụ", std::to_string(day.served) + " khách"},
            {"Doanh thu", moneyText(day.revenue + day.tips)},
            {"Combo hiện tại", "x" + std::to_string(game().liveCombo)}};
        for (int i = 0; i < 3; ++i) {
            const float x = 364 + static_cast<float>(i) * 188;
            ui::panel(target, {x, 214, 176, 58}, {250, 241, 214}, 14);
            ui::text(target, font, stats[i].first, 12, {x + 88, 229}, theme.muted, true);
            ui::textFit(target, titleFont, stats[i].second, 20, {x + 88, 253}, theme.ink, 160, true, 13);
        }
        ui::text(target, titleFont, "Nhiệm vụ hôm nay", 18, {364, 288}, theme.green);
        for (std::size_t i = 0; i < day.goals.size() && i < 3; ++i) {
            const auto &goal = day.goals[i];
            const float y = 318 + static_cast<float>(i) * 36;
            const bool failed = goal.id == "nolose" && day.left > 0;
            if (goal.done()) ui::checkMark(target, {378, y + 11}, 9, theme.green);
            else {
                sf::CircleShape box(8.f);
                box.setOrigin(8, 8);
                box.setPosition(378, y + 11);
                box.setFillColor(sf::Color::Transparent);
                box.setOutlineColor(failed ? theme.red : theme.gold);
                box.setOutlineThickness(2.5f);
                target.draw(box);
            }
            ui::textFit(target, font, goalText(goal), 15, {396, y + 2}, goal.done() ? theme.green : failed ? theme.red : theme.ink, 400);
            const std::string progress = failed ? "Hỏng" : goalProgressText(goal);
            ui::text(target, font, progress, 14, {926 - 12 - ui::textWidth(font, progress, 14) - 4, y + 3}, theme.muted);
        }
        if (confirmExit_)
            ui::text(target, font, "Giữ tiền đã kiếm; ngày đang chơi sẽ bắt đầu lại.", 15, {640, 618}, theme.muted, true);
    } else {
        ui::text(target, titleFont, "Cài đặt", 34, {640, 170}, theme.ink, true);
        ui::text(target, font, "Nhạc nền: " + std::to_string(static_cast<int>(game().progress.music)) + "%",
                 19, {445, 241}, theme.ink);
        ui::text(target, font, "Hiệu ứng: " + std::to_string(static_cast<int>(game().progress.sfx)) + "%", 19,
                 {445, 335}, theme.ink);
        for (int i = 0; i < 2; ++i) {
            const float volume = i == 0 ? game().progress.music : game().progress.sfx,
                        y = i == 0 ? musicBarY : sfxBarY;
            ui::ProgressBar::draw(target, {445, y, 390, 10}, volume / 100, theme.green);
            sf::CircleShape knob(slider_ == i ? 13.f : 11.f);
            knob.setOrigin(knob.getRadius(), knob.getRadius());
            knob.setPosition(445 + 390 * volume / 100, y + 5);
            knob.setFillColor(theme.green);
            knob.setOutlineColor({255, 250, 236});
            knob.setOutlineThickness(2.f);
            target.draw(knob);
        }
    }
    renderButtons(target);
    if (!game().saves.error().empty())
        ui::text(target, font, "Chưa lưu được. Kiểm tra quyền ghi thư mục dữ liệu.", 14, {640, pause ? 628.f : 660.f},
                 {175, 67, 45}, true);
    if (help_) {
        target.draw(dim);
        const sf::FloatRect help{150, 70, 980, 570};
        ui::shadowPanel(target, help, theme.paper, 26, 8, 70);
        ui::text(target, titleFont, "Cách chơi", 34, {640, 118}, theme.ink, true);
        ui::text(target, titleFont, "Các bước", 20, {190, 160}, theme.green);
        const char *lines[] = {"1. Chọn khách và xem số muỗng topping trên đơn.",
            "2. Giữ và rê trên bánh để xé. Thớt tốt giúp xé nhanh.",
            "3. Chọn khay, giữ trên tô rồi thả: 1-3 muỗng.",
            "4. Bấm Trộn món, rồi đánh nhịp khi vạch vào vùng xanh.",
            "5. Giao món để nhận tiền, tip và đánh giá sao.",
            "6. Giao liên tiếp đơn 4 sao để có combo. Giờ cao điểm: khách đông, tip +20%.",
            "7. Hoàn thành 3 nhiệm vụ mỗi ngày để nhận thưởng thêm."};
        for (int i = 0; i < 7; ++i)
            ui::paragraph(target, font, lines[i], 16, {190, 196 + static_cast<float>(i) * 48}, 450, 20, theme.ink);
        ui::text(target, titleFont, "Phím tắt", 20, {700, 160}, theme.green);
        const std::pair<const char *, const char *> keys[] = {{"1 - 6", "Chọn khay topping"}, {"Space", "Trộn / Đánh nhịp / Giao món"},
            {"Mũi tên trái/phải, Tab", "Đổi khách đang chọn"}, {"Backspace / Chuột phải", "Bỏ muỗng cuối"},
            {"Q / E", "Trang khay trước / sau"}, {"ESC", "Tạm dừng"}, {"F11", "Bật/tắt toàn màn hình"},
            {"Enter", "Bắt đầu (menu, đầu ngày)"}};
        for (int i = 0; i < 8; ++i) {
            const float y = 196 + static_cast<float>(i) * 44;
            ui::keycap(target, font, keys[i].first, {700 + std::max(20.f, ui::textWidth(font, keys[i].first, 12) + 12.f) / 2, y + 10});
            ui::text(target, font, keys[i].second, 15, {700 + 190, y + 1}, theme.ink);
        }
        helpBack_->render(target);
    }
}
} // namespace tiem

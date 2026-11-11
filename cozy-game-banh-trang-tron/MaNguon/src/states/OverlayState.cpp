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
        addButton("Điều chỉnh phím tắt", {470, 538, 340, 44}, [this] {
            help_ = true;
            slider_ = -1;
            onCovered();
            keyDraft_ = game().progress.keys;
            keyNotice_.clear();
            refreshKeyLabels();
        });
        addButton("Lưu và quay lại", {470, 592, 340, 44}, [this] {
            game().save();
            stack_.pop();
        });
        refreshLabels();
    }
    helpBack_ = std::make_unique<ui::Button>(context_.resources.uiFont(), "Quay lại", sf::FloatRect(130, 612, 220, 42),
                                             [this] { help_ = false; waitingKey_ = -1; game().capturingKey = false; });
    helpBack_->setResources(context_.resources);
    helpBack_->setClickSound(context_.playClick);
    for (std::size_t i = 0; i < KeyBindings::actions().size(); ++i) {
        const float x = 130.f + (i / 8) * 520.f, y = 160.f + (i % 8) * 52.f;
        keyButtons_.emplace_back(context_.resources.uiFont(), "", sf::FloatRect(x + 266, y + 5, 222, 36), [this, i] {
            waitingKey_ = static_cast<int>(i); game().capturingKey = true;
            keyNotice_ = "Nhấn phím mới cho: " + std::string(KeyBindings::actions()[i].label) + " • Esc: hủy";
            refreshKeyLabels();
        });
        keyButtons_.back().setResources(context_.resources);
        keyButtons_.back().setClickSound(context_.playClick);
    }
    keyReset_ = std::make_unique<ui::Button>(context_.resources.uiFont(), "Khôi phục mặc định", sf::FloatRect(485, 612, 280, 42), [this] {
        keyDraft_ = {}; waitingKey_ = -1; game().capturingKey = false;
        keyNotice_ = "Đã chọn phím mặc định. Bấm Lưu để áp dụng."; refreshKeyLabels();
    });
    keySave_ = std::make_unique<ui::Button>(context_.resources.uiFont(), "Lưu phím tắt", sf::FloatRect(920, 612, 230, 42), [this] {
        const auto previous = game().progress.keys;
        game().progress.keys = keyDraft_;
        if (!game().save()) { game().progress.keys = previous; keyNotice_ = "Chưa lưu được. Hãy thử lại."; return; }
        help_ = false; waitingKey_ = -1; game().capturingKey = false;
    });
    for (auto *button : {keyReset_.get(), keySave_.get()}) {
        button->setResources(context_.resources); button->setClickSound(context_.playClick);
    }
}
OverlayState::~OverlayState() { if (help_) game().capturingKey = false; }
void OverlayState::onCovered() {
    UiState::onCovered(); slider_ = -1; waitingKey_ = -1; game().capturingKey = false;
    for (auto &button : keyButtons_) button.cancelInteraction();
    if (helpBack_) helpBack_->cancelInteraction();
    if (keySave_) keySave_->cancelInteraction();
    if (keyReset_) keyReset_->cancelInteraction();
    refreshKeyLabels();
}
void OverlayState::refreshKeyLabels() {
    for (std::size_t i = 0; i < keyButtons_.size(); ++i)
        keyButtons_[i].setLabel(waitingKey_ == static_cast<int>(i) ? "Nhấn phím..." : keyDraft_.label(KeyBindings::actions()[i].id, true));
}
void OverlayState::refreshLabels() {
    if (kind_ != StateID::Settings || buttons_.size() < 2) return;
    buttons_[0].setLabel("Toàn màn hình (" + game().progress.keys.label("fullscreen") + "): " + (game().progress.fullscreen ? "Bật" : "Tắt"));
    buttons_[1].setLabel(std::string("Hướng dẫn khi chơi: ") + (game().progress.tutorialDone ? "Tắt" : "Bật"));
}
bool OverlayState::update(float dt) {
    age_ += dt;
    refreshLabels();
    if (help_) {
        helpBack_->update(dt);
        keyReset_->update(dt); keySave_->update(dt);
        for (auto &button : keyButtons_) button.update(dt);
        return false;
    }
    return UiState::update(dt);
}
bool OverlayState::handleEvent(const sf::Event &e, Pointer mouse) {
    if (help_) {
        if (e.type == sf::Event::LostFocus) { onCovered(); keyNotice_.clear(); return false; }
        if (waitingKey_ >= 0) {
            if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Escape) { waitingKey_ = -1; game().capturingKey = false; keyNotice_.clear(); }
                else if (e.key.control || e.key.alt || e.key.shift || e.key.system)
                    keyNotice_ = "Dùng một phím đơn, không kèm Ctrl / Alt / Shift.";
                else {
                    keyNotice_ = keyDraft_.assign(KeyBindings::actions()[waitingKey_].id, e.key.code);
                    if (keyNotice_.empty()) { waitingKey_ = -1; game().capturingKey = false; keyNotice_ = "Đã đổi phím. Bấm Lưu để áp dụng."; }
                }
                refreshKeyLabels();
            }
            return false;
        }
        if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape) help_ = false;
        else {
            helpBack_->handleEvent(e, mouse); keyReset_->handleEvent(e, mouse); keySave_->handleEvent(e, mouse);
            for (auto &button : keyButtons_) button.handleEvent(e, mouse);
        }
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
    if (e.type == sf::Event::KeyPressed && (e.key.code == sf::Keyboard::Escape ||
        (kind_ == StateID::Pause && game().progress.keys.matches("pause", e.key.code)))) {
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
            ui::text(target, font, "Chỉ lưu tiền khi hết ngày; rời bây giờ sẽ chơi lại ngày này.", 15, {640, 618}, theme.muted, true);
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
        const sf::FloatRect help{100, 56, 1080, 620};
        ui::shadowPanel(target, help, theme.paper, 26, 8, 70);
        ui::text(target, titleFont, "Điều chỉnh phím tắt", 32, {640, 92}, theme.ink, true);
        ui::text(target, font, "Bấm ô phím, nhấn phím mới rồi Lưu. Quay lại để bỏ thay đổi chưa lưu.", 15, {640, 131}, theme.muted, true);
        for (std::size_t i = 0; i < KeyBindings::actions().size(); ++i) {
            const float x = 130.f + (i / 8) * 520.f, y = 160.f + (i % 8) * 52.f;
            ui::panel(target, {x, y, 496, 46}, waitingKey_ == static_cast<int>(i) ? sf::Color(240, 224, 181) : sf::Color(246, 239, 218), 10);
            ui::textFit(target, font, KeyBindings::actions()[i].label, 15, {x + 12, y + 13}, theme.ink, 245);
        }
        ui::text(target, font, keyNotice_.empty() ? "Esc vẫn dùng để hủy / quay lại trong các cửa sổ. Chuột giữ cách thao tác hiện tại." : keyNotice_,
                 14, {640, 592}, theme.muted, true);
        for (const auto &button : keyButtons_) button.render(target);
        keyReset_->render(target); keySave_->render(target);
        helpBack_->render(target);
    }
}
} // namespace tiem

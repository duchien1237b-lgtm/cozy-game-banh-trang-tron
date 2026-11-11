#include "states/TutorialState.hpp"
#include "gameplay/CustomerMood.hpp"
#include "ui/Art.hpp"
#include "ui/Atmosphere.hpp"
#include <algorithm>
#include <cmath>

namespace tiem {
namespace {
const sf::FloatRect card{70, 40, 1140, 600};
const sf::FloatRect stage{100, 150, 520, 420};      // vùng minh họa / thực hành
const sf::FloatRect paperArea{260, 248, 200, 184};
const sf::Vector2f bowlCenter{360, 390};
const sf::FloatRect trayArea{140, 200, 130, 120};
const sf::FloatRect mixBar{140, 330, 440, 48};
const char *titles[TutorialState::pageCount] = {"Chào mừng đến Góc Nhỏ Bánh Tráng", "Đọc đơn và hiểu khách",
    "Bước 1 • Xé bánh tráng", "Bước 2 • Thêm topping", "Bước 3 • Trộn đều theo nhịp", "Bước 4 • Giao món và tính điểm",
    "Một ngày ở tiệm"};
bool overBowl(sf::Vector2f p) {
    const float x = (p.x - bowlCenter.x) / 120.f, y = (p.y - bowlCenter.y) / 100.f;
    return x * x + y * y <= 1.f;
}
} // namespace

TutorialState::TutorialState(StateStack &stack, AppContext &context)
    : UiState(stack, context), charge_(context.game->data.number("topping_charge_seconds")), cookingFx_(context.resources) {
    if (context_.audio) context_.audio->music("menu");
    addButton("Bỏ qua hướng dẫn", {100, 584, 210, 42}, [this] { finish(); });
    addButton("Trước", {800, 584, 150, 42}, [this] { go(page_ - 1); });
    addButton("Tiếp", {966, 584, 214, 42}, [this] { go(page_ + 1); });
    addButton("Thử lại", {480, 152, 130, 34}, [this] { resetPractice(); });
    go(0);
}
TutorialState::~TutorialState() {
    if (context_.audio) context_.audio->charging(false);
}
void TutorialState::onCovered() {
    UiState::onCovered();
    charge_.cancel();
    if (paper_) paper_->cancel();
    if (context_.audio) context_.audio->charging(false);
}
void TutorialState::go(int page) {
    if (page >= pageCount) {
        finish();
        return;
    }
    page_ = std::clamp(page, 0, pageCount - 1);
    pageAge_ = 0;
    resetPractice();
    buttons_[1].setEnabled(page_ > 0);
    buttons_[2].setLabel(page_ == pageCount - 1 ? (game().guideFromMenu ? "Xong, về menu" : "Bắt đầu chơi!") : "Tiếp");
}
void TutorialState::finish() {
    game().progress.guideDone = true;
    game().save();
    const bool fromMenu = game().guideFromMenu;
    game().guideFromMenu = false;
    if (fromMenu) returnToMenu();
    else stack_.replace(StateID::Prep);
}
void TutorialState::resetPractice() {
    onCovered();
    bowl_.clear();
    cookingFx_.clear();
    lastMix_ = -1;
    paper_ = std::make_unique<PaperTearing>(game().data, paperArea, 0.f, 0);
    mix_ = std::make_unique<MixingMinigame>(game().data, 0.f, 1, mixSeed_++);
    if (page_ == 3) bowl_.paperQuality = 1;
}
bool TutorialState::practiceDone() const {
    switch (page_) {
    case 2: return paper_ && paper_->done();
    case 3: return bowl_.portions.size() >= 3;
    case 4: return mix_ && mix_->done();
    default: return true;
    }
}
bool TutorialState::handleEvent(const sf::Event &e, Pointer mouse) {
    pointer_ = mouse;
    if (e.type == sf::Event::KeyPressed) {
        if (!(page_ == 4 && game().progress.keys.matches("primary", e.key.code)) && (e.key.code == sf::Keyboard::Right || e.key.code == sf::Keyboard::Enter)) { sound("ui_click"); go(page_ + 1); return false; }
        if (!(page_ == 4 && game().progress.keys.matches("primary", e.key.code)) && (e.key.code == sf::Keyboard::Left || e.key.code == sf::Keyboard::BackSpace)) { if (page_ > 0) { sound("ui_click"); go(page_ - 1); } return false; }
        if (e.key.code == sf::Keyboard::Escape) { finish(); return false; }
        if (game().progress.keys.matches("primary", e.key.code) && page_ == 4 && mix_) {
            if (const auto v = mix_->hit()) {
                lastMix_ = mix_->hits();
                lastMixValue_ = *v;
                sound(*v > .9f ? "mix_perfect" : *v > .5f ? "mix_good" : "mix_miss");
                effects_.say(*v > .9f ? "Perfect!" : *v > .5f ? "Good!" : "Miss!", {360, 300}, *v > .5f ? sf::Color(60, 118, 70) : sf::Color(182, 90, 56));
            }
            return false;
        }
    }
    if (e.type == sf::Event::MouseLeft) onCovered();
    // Thực hành xé bánh.
    if (page_ == 2 && paper_) {
        if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left && mouse) paper_->begin(*mouse);
        if (e.type == sf::Event::MouseMoved) {
            if (mouse) paper_->move(*mouse);
            else paper_->cancel();
        }
        if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left) {
            if (mouse) paper_->move(*mouse);
            if (paper_->release()) {
                sound("upgrade_buy");
                effects_.say("Xé xong! Thả chuột là sang bước topping.", {360, 220}, {60, 118, 70}, 2.2f, 20, true);
            }
        }
    }
    // Thực hành thêm topping: chọn khay rồi giữ trên tô.
    if (page_ == 3) {
        if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left && mouse && overBowl(*mouse)) {
            charge_.begin();
            if (context_.audio) context_.audio->charging(true, game().data.number("topping_charge_seconds"));
        }
        if (e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Right && bowl_.removeLast()) sound("undo");
        if (e.type == sf::Event::MouseMoved && charge_.active() && (!mouse || !overBowl(*mouse))) onCovered();
        if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == sf::Mouse::Left && charge_.active()) {
            const int n = charge_.release();
            if (context_.audio) context_.audio->charging(false);
            for (int i = 0; i < n; ++i) bowl_.add(game().data.toppings.front().id, 24);
            if (n > 0) {
                sound("ingredient_pop");
                effects_.say("+" + std::to_string(n) + " muỗng", {360, 300}, ui::Theme{}.green);
            } else effects_.say("Giữ lâu hơn chút!", {360, 300}, ui::Theme{}.red);
        }
    }
    if (page_ == 4 && mix_ && e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == sf::Mouse::Left && mouse &&
        mixBar.contains(*mouse)) {
        if (const auto v = mix_->hit()) {
            lastMix_ = mix_->hits();
            lastMixValue_ = *v;
            sound(*v > .9f ? "mix_perfect" : *v > .5f ? "mix_good" : "mix_miss");
            effects_.say(*v > .9f ? "Perfect!" : *v > .5f ? "Good!" : "Miss!", {360, 300}, *v > .5f ? sf::Color(60, 118, 70) : sf::Color(182, 90, 56));
        }
    }
    if (!charge_.active()) handleButtons(e, mouse);
    return false;
}
bool TutorialState::update(float dt) {
    age_ += dt;
    pageAge_ += dt;
    effects_.update(dt);
    cookingFx_.update(dt);
    if (page_ == 2 && paper_) {
        const int before = paper_->completedStages();
        const float elapsed = paper_->elapsed();
        paper_->update(dt);
        if (paper_->elapsed() > elapsed && pointer_ && std::fmod(age_, .09f) < dt) cookingFx_.tear(*pointer_);
        if (paper_->completedStages() > before) {
            sound("paper_tear");
            cookingFx_.tear({360, 340}, true);
        }
    }
    if (page_ == 3) {
        if (charge_.active() && pointer_ && overBowl(*pointer_)) charge_.update(dt);
        else if (charge_.active()) onCovered();
    }
    if (page_ == 4 && mix_) mix_->update(dt);
    buttons_[3].setEnabled(page_ >= 2 && page_ <= 4);
    return UiState::update(dt);
}
void TutorialState::heading(sf::RenderTarget &target, const std::string &title, const std::string &sub) {
    const ui::Theme theme;
    ui::text(target, context_.resources.titleFont(), title, 32, {card.left + 40, card.top + 26}, theme.ink);
    ui::text(target, context_.resources.uiFont(), sub, 16, {card.left + 42, card.top + 70}, theme.muted);
}
void TutorialState::bullets(sf::RenderTarget &target, const std::vector<std::string> &items, sf::Vector2f at, float width) {
    const ui::Theme theme;
    float y = at.y;
    for (const auto &item : items) {
        sf::CircleShape dot(4.f);
        dot.setOrigin(4, 4);
        dot.setPosition(at.x + 6, y + 11);
        dot.setFillColor(theme.orange);
        target.draw(dot);
        const auto lines = ui::paragraph(target, context_.resources.uiFont(), item, 16, {at.x + 20, y}, width - 20, 22, theme.ink);
        y += static_cast<float>(lines) * 22.f + 10.f;
    }
}
void TutorialState::drawWelcome(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    ui::image(target, context_.resources, "props/cart", {120, 210, 330, 300});
    const float bob = std::sin(age_ * 2.4f) * 3.f;
    ui::image(target, context_.resources, "vendor/vendor_full_wave_01", {400, 196 + bob, 190, 330});
    // Vòng lặp chơi: 5 bước.
    const char *steps[] = {"Chọn khách", "Xé bánh", "Thêm topping", "Trộn", "Giao món"};
    for (int i = 0; i < 5; ++i) {
        const float x = 140 + static_cast<float>(i) * 96;
        const float pulse = static_cast<int>(age_ * 1.2f) % 5 == i ? 1.f : 0.f;
        ui::panel(target, {x, 528 - pulse * 4, 86, 40}, pulse > 0 ? theme.gold : sf::Color(250, 241, 214), 12);
        ui::text(target, font, std::to_string(i + 1), 12, {x + 12, 534 - pulse * 4}, pulse > 0 ? sf::Color(255, 250, 232) : theme.muted);
        ui::textFit(target, font, steps[i], 12, {x + 47, 554 - pulse * 4}, pulse > 0 ? sf::Color(255, 250, 232) : theme.ink, 80, true);
    }
    bullets(target, {"Bạn là chủ một xe bánh tráng trộn nhỏ trên phố. Mỗi ngày tiệm mở từ 07:00 đến 21:00 (khoảng 3 phút chơi).",
                     "Khách đến xếp hàng, mỗi người gọi một đơn riêng. Làm đúng đơn, thật nhanh để được nhiều sao, tiền tip và combo.",
                     "Tiền kiếm được dùng để nhập nguyên liệu, mở topping mới và nâng cấp tiệm.",
                     "Hướng dẫn có 3 trang cho bạn tập thử ngay. Bấm Tiếp (hoặc phím mũi tên phải) để đi tiếp, Bỏ qua nếu đã quen."},
            {660, 160}, 500);
}
void TutorialState::drawCustomers(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &d = game().data;
    // Khách mẫu chuyển dần qua các biểu cảm khi kiên nhẫn giảm.
    const float cycle = std::fmod(pageAge_, 9.f) / 9.f;
    Customer sample;
    sample.patience = 60;
    sample.remaining = 60 * (1.f - cycle);
    if (sample.waited() < 2.5f) sample.remaining = 57.f;
    const auto &persona = d.personalities.at(std::min<std::size_t>(1, d.personalities.size() - 1));
    const auto expression = CustomerMood::waiting(sample, persona);
    const sf::FloatRect bubble{130, 160, 300, 112};
    if (!ui::frame(target, context_.resources, "ui/cozy_frames/chat_bubble", {bubble.left, bubble.top, bubble.width, bubble.height + 16}, {48, 64}, {15, 20}))
        ui::panel(target, bubble, theme.paper, 18);
    ui::textFit(target, context_.resources.dialogFont(), CustomerMood::line(persona, expression, 1), 15, {bubble.left + 14, bubble.top + 12}, theme.ink, bubble.width - 28);
    const char *items[] = {"Muối tôm ×2", "Xoài sống ×1", "Hành phi ×1", "Rau răm ×1"};
    for (int i = 0; i < 4; ++i)
        ui::text(target, font, items[i], 14, {bubble.left + 16 + static_cast<float>(i % 2) * 140, bubble.top + 44 + static_cast<float>(i / 2) * 24}, theme.ink);
    const float tw = ui::textWidth(context_.resources.smallFont(), persona.name, 11) + 16;
    ui::panel(target, {bubble.left + bubble.width - tw - 6, bubble.top - 9, tw, 18}, persona.color, 9);
    ui::text(target, context_.resources.smallFont(), persona.name, 11, {bubble.left + bubble.width - tw / 2 - 6, bubble.top}, {255, 252, 240}, true);
    const auto motion = ui::expressionMotion(expression, age_, 3);
    ui::image(target, context_.resources, std::string("customers/student_") + CustomerMood::face(expression), {185 + motion.x, 296 + motion.y, 190, 190});
    ui::emote(target, font, expression, {326 + motion.x, 318 + motion.y}, age_);
    const float mood = sample.mood();
    ui::ProgressBar::draw(target, {180, 500, 200, 8}, mood, mood < .3f ? theme.red : mood < .55f ? theme.orange : theme.green);
    ui::text(target, font, std::string("Biểu cảm: ") + CustomerMood::label(expression), 15, {280, 518}, theme.ink, true);
    // Danh sách tính cách.
    ui::text(target, context_.resources.titleFont(), "Mỗi khách một tính cách", 20, {660, 150}, theme.green);
    float y = 186;
    for (const auto &p : d.personalities) {
        const float w = ui::textWidth(context_.resources.smallFont(), p.name, 13) + 20;
        ui::panel(target, {660, y, 104, 24}, p.color, 12);
        ui::textFit(target, context_.resources.smallFont(), p.name, 13, {712, y + 12}, {255, 252, 240}, 96, true);
        (void)w;
        ui::textFit(target, font, p.trait, 15, {776, y + 3}, theme.ink, 400);
        y += 32;
    }
    bullets(target, {"Bấm vào khách để chọn đơn. Dòng trong bong bóng là topping và số muỗng cần làm.",
                     "Thanh dưới chân là kiên nhẫn: khách sốt ruột (giọt mồ hôi), bực bội (gân đỏ) rồi bỏ về."},
            {660, y + 12}, 500);
}
void TutorialState::drawPaper(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const int stages = paper_->completedStages();
    const char *art = stages == 0 ? "rice_paper/paper_whole" : stages == 1 ? "paper_tearing/rice_paper/paper_torn_stage_01" : "paper_tearing/rice_paper/paper_torn_stage_02";
    ui::radial(target, {360, 430}, {110, 16}, {61, 36, 20, 60}, {61, 36, 20, 0});
    ui::image(target, context_.resources, art, paperArea);
    if (!paper_->done() && !paper_->active()) {
        // Bàn tay gợi ý rê qua lại.
        const float sway = std::sin(age_ * 4.f) * 60.f;
        ui::radial(target, {360 + sway, 340}, {18, 18}, {255, 214, 98, 200}, {255, 214, 98, 0}, true);
    }
    ui::ProgressBar::draw(target, {250, 456, 220, 10}, paper_->progress(), theme.green);
    ui::text(target, font, paper_->done() ? "Hoàn thành! Bánh đã thành miếng nhỏ." : "Xé: " + std::to_string(static_cast<int>(paper_->progress() * 100)) + "%",
             15, {360, 482}, paper_->done() ? theme.green : theme.muted, true);
    bullets(target, {"Nhấn GIỮ chuột trái trên bánh rồi RÊ qua lại. Chỉ khi chuột đang di chuyển trên bánh thì mới xé.",
                     "Sau một lúc bánh thành dải, thêm một lúc nữa thành miếng nhỏ. Thả chuột giữa chừng vẫn giữ tiến độ.",
                     "Xé xong thì thả chuột: bánh tự vào tô và chuyển sang thêm topping.",
                     "Nâng cấp Thớt ở cửa hàng giúp xé nhanh hơn."},
            {660, 160}, 500);
    ui::text(target, context_.resources.titleFont(), "Thử ngay bên trái!", 20, {660, 470}, theme.orange);
}
void TutorialState::drawToppings(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &t = game().data.toppings.front();
    ui::image(target, context_.resources, "toppings/" + t.id + "_tray", {trayArea.left, trayArea.top, trayArea.width, 106});
    sf::CircleShape ring(56.f);
    ring.setOrigin(56, 56);
    ring.setScale(1.1f, .45f);
    ring.setPosition(trayArea.left + 65, trayArea.top + 70);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(theme.green);
    ring.setOutlineThickness(3);
    target.draw(ring);
    ui::text(target, font, t.name + " (đang chọn)", 13, {205, 330}, theme.green, true);
    ui::bowl(target, context_.resources, game().data, bowl_, bowlCenter, .62f, charge_.active(), bowl_.portions.size() >= 3);
    // Thanh lực 1-3 muỗng giống màn chơi.
    const sf::FloatRect bar{250, 486, 254, 16};
    ui::panel(target, {bar.left - 4, bar.top - 4, bar.width + 8, bar.height + 8}, {139, 89, 49}, 10);
    ui::panel(target, bar, {105, 82, 55}, 6);
    const sf::Color colors[] = {{131, 192, 112}, {242, 191, 72}, {245, 139, 65}};
    for (int tier = 0; tier < 3; ++tier) {
        const float start = static_cast<float>(tier) / 3.f;
        const float fill = std::clamp(charge_.power() - start, 0.f, 1.f / 3);
        if (charge_.active() && fill > 0) ui::panel(target, {bar.left + bar.width * start, bar.top, bar.width * fill, bar.height}, colors[tier], 3);
        ui::text(target, context_.resources.smallFont(), std::to_string(tier + 1), 11, {bar.left + bar.width * (static_cast<float>(tier) + .5f) / 3, bar.top + 8}, {255, 251, 226}, true);
    }
    ui::text(target, font, "Trong tô: " + std::to_string(bowl_.portions.size()) + " muỗng  •  mục tiêu: 3", 15, {377, 516}, bowl_.portions.size() >= 3 ? theme.green : theme.ink, true);
    bullets(target, {"Bấm chọn một khay topping (hoặc phím 1–6).",
                     "Nhấn GIỮ chuột trên tô: thanh lực chạy qua 3 mức. Thả chuột ở mức 1, 2 hoặc 3 để thêm bấy nhiêu muỗng.",
                     "Thêm nhầm? Chuột phải hoặc nút Bỏ muỗng cuối. Thừa hoặc thiếu đều bị trừ điểm công thức.",
                     "Mỗi muỗng lấy từ kho. Hết hàng thì khay hiện HẾT, có thể nhập gấp với giá gấp đôi."},
            {660, 160}, 500);
    ui::text(target, context_.resources.titleFont(), "Thử thêm đúng 3 muỗng muối tôm!", 20, {660, 470}, theme.orange);
}
void TutorialState::drawMixing(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    Bowl mixed;
    for (int i = 0; i < 5; ++i) mixed.portions.push_back(game().data.toppings[static_cast<std::size_t>(i % 5)].id);
    mixed.mixed = mix_->done();
    ui::bowl(target, context_.resources, game().data, mixed, {360, 470}, .4f, false, true);
    ui::panel(target, {mixBar.left - 6, mixBar.top - 6, mixBar.width + 12, mixBar.height + 12}, {130, 80, 41}, 14);
    ui::panel(target, mixBar, {102, 73, 49}, 9);
    ui::panel(target, {mixBar.left + mixBar.width * (mix_->target() - mix_->width() / 2), mixBar.top, mixBar.width * mix_->width(), mixBar.height}, {100, 170, 106}, 5);
    ui::panel(target, {mixBar.left + mixBar.width * (mix_->target() - mix_->width() * .2f), mixBar.top, mixBar.width * mix_->width() * .4f, mixBar.height}, {213, 236, 150}, 4);
    ui::panel(target, {mixBar.left + mixBar.width * mix_->position() - 4, mixBar.top - 8, 8, mixBar.height + 16}, theme.ink, 3);
    const std::string rhythm = MixingMinigame::rhythmName(mix_->rhythm());
    const float rw = ui::textWidth(font, rhythm, 14) + 24;
    ui::panel(target, {360 - rw / 2, 292, rw, 24}, {120, 86, 40, 220}, 12);
    ui::text(target, font, rhythm, 14, {360, 303}, {255, 244, 214}, true);
    for (int i = 0; i < game().data.integer("mix_hits"); ++i) {
        sf::CircleShape dot(8.f);
        dot.setOrigin(8, 8);
        dot.setPosition(330.f + static_cast<float>(i) * 30.f, 400);
        dot.setFillColor(i < mix_->hits() ? theme.green : sf::Color(224, 217, 197));
        target.draw(dot);
    }
    if (mix_->done())
        ui::text(target, font, "Điểm trộn: " + std::to_string(static_cast<int>(std::lround(mix_->score() * 100))) + "%", 17, {360, 530}, theme.green, true);
    bullets(target, {"Bấm Trộn món, rồi bấm " + game().progress.keys.label("primary") + " (hoặc bấm vào thanh) khi vạch đen chạy vào vùng xanh. Giữa vùng xanh là Perfect.",
                     "Mỗi nhịp đổi kiểu ngẫu nhiên: nhịp nhanh, nhịp chậm, lắc lư (lúc nhanh lúc chậm) hay đảo chiều bất ngờ. Nhìn nhãn phía trên thanh để chuẩn bị.",
                     "Vùng xanh đổi chỗ và độ rộng sau mỗi nhịp. Đủ 3 nhịp là món sẵn sàng giao."},
            {660, 160}, 500);
    ui::text(target, context_.resources.titleFont(), "Thử trộn 3 nhịp: phím " + game().progress.keys.label("primary") + "!", 20, {660, 470}, theme.orange);
}
void TutorialState::drawScoring(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &w = game().data.balance.at("weights");
    struct Part { const char *name; float weight; sf::Color color; };
    const Part parts[] = {{"Đúng công thức", w.at("recipe").get<float>(), theme.green}, {"Xé bánh", w.at("paper").get<float>(), theme.gold},
                          {"Trộn đều", w.at("mix").get<float>(), theme.orange}, {"Tốc độ giao", w.at("speed").get<float>(), {70, 140, 180}}};
    ui::text(target, context_.resources.titleFont(), "Điểm mỗi món /100", 20, {130, 160}, theme.ink);
    float y = 200;
    for (const auto &p : parts) {
        const float grow = ui::easeOut(std::clamp(pageAge_ / .8f, 0.f, 1.f));
        ui::text(target, font, p.name, 16, {130, y}, theme.ink);
        ui::panel(target, {280, y + 2, 300, 18}, {236, 228, 206}, 9);
        ui::panel(target, {280, y + 2, 300 * p.weight * 2 * grow, 18}, p.color, 9);
        ui::text(target, font, std::to_string(static_cast<int>(std::lround(p.weight * 100))) + "%", 14, {590, y + 1}, theme.muted);
        y += 40;
    }
    for (int i = 0; i < 5; ++i) {
        const float pop = std::clamp(pageAge_ * 2.f - static_cast<float>(i) * .3f, 0.f, 1.f);
        ui::star(target, {200.f + static_cast<float>(i) * 50.f, 392}, 18 * pop, i < 4 ? sf::Color(232, 176, 60) : sf::Color(224, 217, 194));
    }
    ui::text(target, font, "Điểm càng cao càng nhiều sao (1–5) và tip.", 15, {300, 424}, theme.muted, true);
    ui::panel(target, {150, 452, 300, 46}, theme.gold, 14);
    ui::text(target, context_.resources.titleFont(), "COMBO x3 • Tip +8%", 18, {300, 475}, {255, 252, 238}, true);
    bullets(target, {"Món trộn xong: KÉO tô tới khách đang đợi hoặc bấm Giao món (" + game().progress.keys.label("primary") + ") cho khách đang chọn.",
                     "Giao liên tiếp các đơn từ 4 sao để lên combo, tip tăng dần. Khách bỏ về hoặc đơn dưới 4 sao sẽ mất combo.",
                     "Khách phản ứng theo tính cách: khách Kỹ tính cần nhiều sao hơn mới vui, khách Hào phóng bo tip rất đậm, khách Nóng tính bo thêm nếu giao nhanh."},
            {660, 160}, 500);
}
void TutorialState::drawDay(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    // Dải trời 07:00 → 21:00 tối dần.
    const sf::FloatRect strip{120, 180, 480, 70};
    const int steps = 48;
    for (int i = 0; i < steps; ++i) {
        const float hour = DayClock::openHour + static_cast<float>(i) / steps * DayClock::hours;
        const auto light = DayClock::light(hour, 0.f);
        sf::RectangleShape r({strip.width / steps + 1, strip.height});
        r.setPosition(strip.left + strip.width * static_cast<float>(i) / steps, strip.top);
        r.setFillColor({static_cast<sf::Uint8>(150 * light.tint.r / 255 + 40), static_cast<sf::Uint8>(200 * light.tint.g / 255 + 20),
                        static_cast<sf::Uint8>(240 * light.tint.b / 255)});
        target.draw(r);
    }
    ui::panelOutline(target, strip, {139, 89, 49}, 6, 2);
    const float sweep = std::fmod(pageAge_ / 6.f, 1.f);
    const float hour = DayClock::openHour + sweep * DayClock::hours;
    ui::clockIcon(target, {strip.left + strip.width * sweep, strip.top - 22}, 16, hour, {255, 250, 232}, theme.ink);
    for (int h : {7, 11, 13, 17, 21})
        ui::text(target, font, std::to_string(h) + "h", 13, {strip.left + strip.width * static_cast<float>(h - 7) / 14.f, strip.top + strip.height + 8}, theme.muted, true);
    ui::text(target, font, DayClock::format(hour) + " • " + DayClock::period(hour), 16, {360, 284}, theme.ink, true);
    // Thời tiết.
    const struct { Weather w; const char *name; const char *effect; } kinds[] = {
        {Weather::Sunny, "Nắng", "Đông khách, thích đồ tươi"}, {Weather::Cloudy, "Nhiều mây", "Bình thường"},
        {Weather::Rainy, "Mưa", "Vắng, kiên nhẫn, tip +10%"}};
    for (int i = 0; i < 3; ++i) {
        const float x = 190 + static_cast<float>(i) * 170;
        ui::weatherIcon(target, kinds[i].w, {x, 340}, 50, age_);
        ui::text(target, context_.resources.titleFont(), kinds[i].name, 15, {x, 380}, theme.ink, true);
        ui::textFit(target, font, kinds[i].effect, 12, {x, 402}, theme.muted, 160, true);
    }
    // Vòng một ngày.
    const char *flow[] = {"Chuẩn bị", "Mở quán", "Tổng kết", "Nâng cấp"};
    for (int i = 0; i < 4; ++i) {
        const float x = 130 + static_cast<float>(i) * 120;
        ui::panel(target, {x, 452, 104, 40}, static_cast<int>(age_) % 4 == i ? theme.gold : sf::Color(250, 241, 214), 12);
        ui::text(target, font, flow[i], 14, {x + 52, 471}, static_cast<int>(age_) % 4 == i ? sf::Color(255, 252, 238) : theme.ink, true);
    }
    bullets(target, {"Trước mỗi ngày: xem dự báo thời tiết rồi nhập nguyên liệu. Đồ tươi hỏng cuối ngày, đồ khô và sốt để dành được.",
                     "Trong ngày, đồng hồ chạy từ 07:00 tới 21:00, trời tối dần và phố lên đèn. Trưa và chiều tan tầm đông khách hơn, có cả giờ cao điểm bất ngờ.",
                     "Cuối ngày xem bảng tổng kết: xếp hạng, sổ thu chi, lãi ròng, khách theo giờ. Rồi mua nâng cấp và chuẩn bị ngày mới.",
                     "Xem và đổi phím thao tác trong Cài đặt → Điều chỉnh phím tắt."},
            {660, 160}, 500);
}
void TutorialState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    ui::backdrop(target);
    ui::lightShafts(target, {0, 0, 1280, 680}, age_, 12);
    const float appear = ui::easeOut(std::min(1.f, pageAge_ / .3f));
    ui::shadowPanel(target, card, theme.paper, 28, 8, 60);
    ui::panelOutline(target, card, {226, 168, 58, 150}, 28, 2);
    heading(target, titles[page_], "Hướng dẫn cách chơi • trang " + std::to_string(page_ + 1) + "/" + std::to_string(pageCount));
    ui::panel(target, {stage.left, stage.top - 6, stage.width, stage.height}, {250, 243, 224, static_cast<sf::Uint8>(255 * appear)}, 20);
    switch (page_) {
    case 0: drawWelcome(target); break;
    case 1: drawCustomers(target); break;
    case 2: drawPaper(target); break;
    case 3: drawToppings(target); break;
    case 4: drawMixing(target); break;
    case 5: drawScoring(target); break;
    default: drawDay(target); break;
    }
    if (page_ >= 2 && page_ <= 4) {
        const bool done = practiceDone();
        ui::panel(target, {660, 506, 300, 34}, done ? sf::Color(214, 236, 196) : sf::Color(244, 238, 222), 12);
        if (done) ui::checkMark(target, {680, 523}, 9, theme.green);
        ui::text(target, context_.resources.uiFont(), done ? "Đã tập xong! Bấm Tiếp." : "Chưa xong — cứ thử, không mất gì đâu.", 14,
                 {done ? 696.f : 676.f, 513}, done ? theme.green : theme.muted);
    }
    // Chấm trang.
    for (int i = 0; i < pageCount; ++i) {
        sf::CircleShape dot(i == page_ ? 7.f : 5.f);
        dot.setOrigin(dot.getRadius(), dot.getRadius());
        dot.setPosition(560.f + static_cast<float>(i) * 24.f, 605);
        dot.setFillColor(i == page_ ? theme.orange : i < page_ ? theme.gold : sf::Color(224, 217, 197));
        target.draw(dot);
    }
    ui::text(target, context_.resources.uiFont(), "Phím trái/phải: đổi trang  •  ESC: bỏ qua", 12, {632, 624}, theme.muted, true);
    for (std::size_t i = 0; i < buttons_.size(); ++i)
        if (i != 3 || buttons_[3].enabled()) buttons_[i].render(target);
    cookingFx_.render(target);
    effects_.render(target, context_.resources.uiFont());
}
} // namespace tiem

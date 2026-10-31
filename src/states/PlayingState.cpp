#include "states/PlayingState.hpp"
#include "core/Utf8.hpp"
#include "ui/Art.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace tiem {
namespace {
const sf::FloatRect paperBounds{637, 431, 200, 184};
const sf::Vector2f bowlCenter{737.f, 513.f};
const sf::Vector2f walletPos{41.f, 384.f};
const sf::FloatRect tutorialCard{500, 354, 470, 76};
const sf::FloatRect hintPill{488, 668, 500, 34};
const sf::FloatRect mixBar{557, 466, 360, 58};
const sf::FloatRect mainButtonRect{1010, 541, 227, 44};
float easeOutBack(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    t = std::clamp(t, 0.f, 1.f);
    return 1.f + c3 * std::pow(t - 1.f, 3.f) + c1 * std::pow(t - 1.f, 2.f);
}
const char *typeName(const std::string &type) {
    return type == "dry" ? "Khô" : type == "fresh" ? "Tươi" : "Sốt";
}
void ring(sf::RenderTarget &target, sf::FloatRect r, float time, sf::Color color) {
    const float pulse = .5f + .5f * std::sin(time * 5.f);
    sf::RectangleShape shape({r.width + 8 + pulse * 6, r.height + 8 + pulse * 6});
    shape.setPosition(r.left - 4 - pulse * 3, r.top - 4 - pulse * 3);
    shape.setFillColor(sf::Color::Transparent);
    color.a = static_cast<sf::Uint8>(150 + 100 * pulse);
    shape.setOutlineColor(color);
    shape.setOutlineThickness(3.f);
    target.draw(shape);
}

} // namespace
PlayingState::PlayingState(StateStack &stack, AppContext &context)
    : UiState(stack, context),
      day_(game().data, game().progress,
           static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count())),
      cookingFx_(context.resources),
      charge_(game().data.number("topping_charge_seconds")) {
    if (context_.audio)
        context_.audio->music("playing");
    for (std::size_t i = 0; i < game().data.toppings.size(); ++i)
        if (game().progress.unlocked.count(game().data.toppings[i].id))
            available_.push_back(i);
    addButton("Tạm dừng", {1085, 29, 162, 44}, [this] { stack_.push(StateID::Pause); });
    addButton("Trước", {40, 680, 195, 28}, [this] {
        if (page_ > 0)
            --page_;
        refreshButtons();
    });
    
    addButton("Sau", {255, 680, 195, 28}, [this] {
        if ((page_ + 1) * 6 < available_.size())
            ++page_;
        refreshButtons();
    });
    addButton("Giữ và rê bánh", mainButtonRect, [this] { primaryAction(); });
    addButton("Làm lại tô", {1010, 606, 227, 40}, [this] {
        resetBowl();
        say("Tô mới đã sẵn sàng. Xé bánh tráng trước nhé!");
    });
    addButton("Bỏ muỗng cuối", {474, 590, 152, 32}, [this] { undoPortion(); });
    startButton_ = std::make_unique<ui::Button>(context_.resources.uiFont(), "Mở cửa!",
                                                sf::FloatRect(500, 536, 280, 54), [this] { startDay(); });
    startButton_->setResources(context_.resources);
    startButton_->setClickSound(context_.playClick);
    skipTutorial_ = std::make_unique<ui::Button>(context_.resources.uiFont(), "Bỏ qua",
                                                 sf::FloatRect(870, 360, 94, 26), [this] {
        tutorial_ = false;
        game().progress.tutorialDone = true;
        game().save();
        say("Đã tắt hướng dẫn. Có thể bật lại trong Cài đặt.");
    });
    
    skipTutorial_->setResources(context_.resources);
    skipTutorial_->setClickSound(context_.playClick);
    tutorial_ = !game().progress.tutorialDone;
    walletShown_ = static_cast<float>(game().progress.money);
    resetBowl();
    refreshButtons();
    message_ = "Giữ và rê qua lại trên bánh để xé.";
}
PlayingState::~PlayingState() {
    if (context_.audio) context_.audio->charging(false);
}
    
void PlayingState::say(std::string text) {
    message_ = std::move(text);
    messageAge_ = 0;
}

void PlayingState::banner(std::string title, std::string sub, sf::Color color, float life) {
    banners_.push_back({std::move(title), std::move(sub), color, 0.f, life});
}

void PlayingState::startDay() {
    if (!briefing_) return;
    briefing_ = false;
    sound("whoosh");
    banner("Ngày " + std::to_string(game().progress.day), "Mở cửa nào!", {255, 244, 207}, 1.9f);
    say("Chọn khách để xem đơn, rồi xé bánh tráng nhé!");
}

void PlayingState::resetBowl() {
    bowl_.clear();
    drops_.clear();
    cookingFx_.clear();
    mixTrail_.clear();
    tearFxClock_ = 0;
    selectedTopping_.reset();
    stage_ = Stage::Paper;
    cancelDrag();
    paper_ = std::make_unique<PaperTearing>(game().data, paperBounds,
                                            UpgradeSystem::effect(game().data, game().progress, "paper"),
                                            UpgradeSystem::level(game().progress, "paper"));
    mix_.reset();
    refreshButtons();
}

sf::FloatRect PlayingState::tray(std::size_t slot) const {
    return {40.f + static_cast<float>(slot % 3) * 142, 414.f + static_cast<float>(slot / 3) * 126, 130, 120};
}

sf::FloatRect PlayingState::customerArea(std::size_t index) const {
    const int slots =
        std::min(game().data.integer("queue_max"),
                 game().data.integer("queue_slots") +
                     static_cast<int>(UpgradeSystem::effect(game().data, game().progress, "queue")));
    const bool decorated = hasDecorations();
    const float margin = decorated ? 100.f : 28.f;
    const float width = (1280.f - 2 * margin - static_cast<float>(slots - 1) * 12) / static_cast<float>(slots);
    const float top = decorated ? 155.f : 92.f;
    return {margin + static_cast<float>(index) * (width + 12), top, width, 362.f - top};
}

bool PlayingState::hasDecorations() const {
    for (const auto *id : {"decor", "lantern", "curtain"})
        if (UpgradeSystem::effect(game().data, game().progress, id) > 0) return true;
    return false;
}

bool PlayingState::toppingsComplete() {
    if (bowl_.portions.empty() || !drops_.empty()) return false;
    for (const auto &customer : day_.customers()) {
        if (customer.id != selectedId_) continue;
        for (const auto &item : customer.order.recipe)
            if (bowl_.count(item.first) < item.second) return false;
        return true;
    }
    return false;
}
float PlayingState::bowlScale() const {
    return .62f + .02f * std::clamp(UpgradeSystem::level(game().progress, "bowl"), 0, 3);
}
bool PlayingState::overBowl(sf::Vector2f p) const {
    const float verticalRadius = bowl_.portions.empty() ? 94.f : 120.f;
    const float x = (p.x - bowlCenter.x) / (120 * bowlScale()),
                y = (p.y - bowlCenter.y) / (verticalRadius * bowlScale());
    return x * x + y * y <= 1;
}
std::optional<std::size_t> PlayingState::selectedIndex() {
    auto &customers = day_.customers();
    if (customers.empty()) {
        selectedId_ = 0;
        return std::nullopt;
    }
    for (std::size_t i = 0; i < customers.size(); ++i)
        if (customers[i].id == selectedId_)
            return i;
    // Khách đang chọn đã rời đi/được phục vụ: chuyển sang người sắp hết kiên nhẫn nhất.
    std::size_t urgent = 0;
    for (std::size_t i = 1; i < customers.size(); ++i)
        if (customers[i].remaining < customers[urgent].remaining) urgent = i;
    selectedId_ = customers[urgent].id;
    return urgent;
}
void PlayingState::cycleCustomer(int direction) {
    auto &customers = day_.customers();
    if (customers.size() < 2) return;
    const auto current = selectedIndex().value_or(0);
    const auto count = static_cast<int>(customers.size());
    const auto next = static_cast<std::size_t>((static_cast<int>(current) + direction + count) % count);
    cancelCharge();
    selectedId_ = customers[next].id;
    sound("ui_click");
}
void PlayingState::refreshButtons() {
    if (buttons_.size() < 6)
        return;
    buttons_[1].setEnabled(page_ > 0 && !charge_.active());
    buttons_[2].setEnabled((page_ + 1) * 6 < available_.size() && !charge_.active());
    const char *name = stage_ == Stage::Paper      ? "Giữ và rê bánh"
                       : stage_ == Stage::Toppings ? "Trộn món"
                       : stage_ == Stage::Mixing   ? "Đánh nhịp"
                                                   : "Giao món";
    buttons_[3].setLabel(name);
    buttons_[3].setEnabled(drops_.empty() && stage_ != Stage::Paper && (stage_ != Stage::Toppings || !bowl_.portions.empty()) &&
                           (stage_ != Stage::Ready || !day_.customers().empty()));
    buttons_[5].setEnabled(stage_ == Stage::Toppings && !bowl_.portions.empty() && drops_.empty() && !charge_.active());
}
void PlayingState::cancelCharge() {
    charge_.cancel();
    if (context_.audio) context_.audio->charging(false);
}
void PlayingState::cancelDrag() {
    cancelCharge();
    bowlDrag_ = false;
}
void PlayingState::onCovered() {
    UiState::onCovered();
    cancelDrag();
    pointer_.reset();
    if (paper_)
        paper_->cancel();
    if (startButton_) startButton_->cancelInteraction();
    if (skipTutorial_) skipTutorial_->cancelInteraction();
}
void PlayingState::primaryAction() {
    if (stage_ == Stage::Toppings)
        startMix();
    else if (stage_ == Stage::Mixing)
        hitMix();
    else if (stage_ == Stage::Ready) {
        if (const auto i = selectedIndex())
            serve(*i);
    }
}
void PlayingState::startMix() {
    if (stage_ != Stage::Toppings || bowl_.portions.empty() || !drops_.empty())
        return;
    cancelDrag();
    stage_ = Stage::Mixing;
    mixTrail_.clear();
    mix_ = std::make_unique<MixingMinigame>(
        game().data, UpgradeSystem::effect(game().data, game().progress, "mix"), game().progress.day);
    say("Bấm Space (hoặc bấm vào thanh) khi vạch vào vùng xanh. Cần đủ " +
        std::to_string(game().data.integer("mix_hits")) + " nhịp.");
    refreshButtons();
}
void PlayingState::hitMix() {
    if (!mix_)
        return;
    const float hitPosition = 557 + 360 * mix_->position();
    const auto value = mix_->hit();
    if (!value)
        return;
    if (*value > .9f) ++bowl_.perfectHits;
    cookingFx_.pulse(*value > .9f ? ui::CookingEffects::Perfect :
                     *value > .5f ? ui::CookingEffects::Good : ui::CookingEffects::Miss,
                     {hitPosition, 489}, *value > .9f ? 108.f : 78.f, .65f);
    effects_.say(*value > .9f   ? "Perfect!"
                 : *value > .5f ? "Good!"
                                : "Miss!",
                 {737, 452}, *value > .5f ? sf::Color(60, 118, 70) : sf::Color(182, 90, 56),
                 game().data.number("feedback_seconds"));
    sound(*value > .9f ? "mix_perfect" : *value > .5f ? "mix_good" : "mix_miss");
    if (mix_->done()) {
        bowl_.mixQuality = mix_->score();
        bowl_.mixed = true;
        stage_ = Stage::Ready;
        say("Trộn xong! Chọn khách rồi Giao món, hoặc kéo tô đến khách.");
    }
    refreshButtons();
}
void PlayingState::addPortions(const std::string &id, int count) {
    int added = 0;
    while (added < count && bowl_.add(id, day_.capacity())) ++added;
    if (added == 0) {
        say("Tô đã đầy. Hãy trộn hoặc bỏ bớt muỗng.");
        return;
    }
    const auto &t = game().data.topping(id);
    say("Đã thêm " + std::to_string(added) + " muỗng " + t.name + (added < count ? ". Tô đã đầy!" : "."));
    // Two icons per actual portion: 2 / 4 / 6, each with its own pop.
    for (int i = 0; i < added * 2; ++i)
        drops_.push_back({id, -static_cast<float>(i) * .13f, i});
    cookingFx_.pulse(ui::CookingEffects::Glint, {610 + 254 * added / 3.f, 644}, 38.f + added * 8.f);
    effects_.say("+" + std::to_string(added) + " muỗng", {737, 408}, ui::Theme{}.green);
    refreshButtons();
}
void PlayingState::undoPortion() {
    if (stage_ != Stage::Toppings || !drops_.empty() || charge_.active()) return;
    std::string removed;
    if (!bowl_.removeLast(&removed)) return;
    sound("undo");
    say("Đã bỏ 1 muỗng " + game().data.topping(removed).name + ".");
    effects_.say("-1 muỗng", {737, 408}, ui::Theme{}.red);
    refreshButtons();
}
void PlayingState::selectTray(std::size_t slot) {
    if (stage_ != Stage::Toppings || !drops_.empty() || charge_.active()) return;
    const auto index = page_ * 6 + slot;
    if (index >= available_.size()) return;
    selectedTopping_ = available_[index];
    selectionAge_ = 0;
    say("Đã chọn " + game().data.toppings[*selectedTopping_].name +
        ". Giữ chuột trên tô, xem thanh lực rồi thả để thêm.");
    sound("topping_select");
}
void PlayingState::collectAchievements() {
    for (auto &id : day_.takeUnlocked()) {
        toasts_.push_back({id, 0.f});
        sound("achievement");
        effects_.sparkle({1090, 130}, {255, 214, 98}, 22);
    }
}
void PlayingState::serve(std::size_t index) {
    if (stage_ != Stage::Ready || index >= day_.customers().size())
        return;
    const auto &kind = game().data.customers[day_.customers()[index].kind];
    const auto customerName = kind.name;
    const auto happySound = kind.happySound();
    const auto receipt = day_.serve(index, bowl_);
    reviews_.push_back({customerName, receipt});
    effects_.say("+" + moneyText(receipt.base + receipt.tip), {737, 440}, {137, 105, 45}, 2.4f, 25, true);
    effects_.burst({737, 487}, {228, 177, 66});
    sound("serve_order");
    for (int i = 0; i < 6; ++i) coins_.push_back({bowlCenter, 0.f, static_cast<float>(i) * .07f});
    if (receipt.stars() >= 4) happySounds_.push_back({.55f, happySound});
    if (receipt.combo >= 2) {
        effects_.say("Combo x" + std::to_string(receipt.combo) + "!", {737, 372}, {255, 214, 98}, 1.8f, 32, true);
        sound("combo_up");
    }
    lastCombo_ = day_.combo();
    if (tutorial_) {
        tutorial_ = false;
        game().progress.tutorialDone = true;
        say("Tuyệt! Bạn đã giao đơn đầu tiên. Cố gắng đúng đơn và giao nhanh để có sao, tip và combo!");
    } else {
        std::ostringstream detail;
        detail << "Công thức " << static_cast<int>(receipt.recipe * 100) << "% • Xé "
               << static_cast<int>(receipt.paper * 100) << "% • Trộn " << static_cast<int>(receipt.mix * 100)
               << "% • Tốc độ " << static_cast<int>(receipt.speed * 100) << "%";
        say(detail.str());
    }
    collectAchievements();
    game().save();
    resetBowl();
    selectedIndex();
}
std::string PlayingState::stageHint() const {
    switch (stage_) {
    case Stage::Paper:
        return "Giữ chuột trái và rê qua lại trên bánh tráng để xé (thả chuột để nghỉ).";
    case Stage::Toppings: {
        const auto &customers = const_cast<PlayingState *>(this)->day_.customers();
        for (const auto &c : customers) {
            if (c.id != selectedId_) continue;
            bool exact = !bowl_.portions.empty();
            for (const auto &item : c.order.recipe)
                if (bowl_.count(item.first) != item.second) exact = false;
            int total = 0;
            for (const auto &item : c.order.recipe) total += item.second;
            if (exact && static_cast<int>(bowl_.portions.size()) == total)
                return "Đủ đúng đơn rồi! Bấm Trộn món (Space).";
        }
        return selectedTopping_ ? "Giữ chuột trên tô rồi thả để thêm 1-3 muỗng • Phím 1-6: chọn khay"
                                : "Chọn một khay topping (bấm chuột hoặc phím 1-6).";
    }
    case Stage::Mixing:
        return "Bấm Space hoặc bấm vào thanh khi vạch chạy vào vùng xanh.";
    case Stage::Ready:
        return "Kéo tô đến khách hoặc bấm Giao món (Space).";
    }
    return {};
}
int PlayingState::tutorialStep() const {
    if (!tutorial_) return -1;
    switch (stage_) {
    case Stage::Paper: return tutorialPressed_ ? 1 : 0;
    case Stage::Toppings: return bowl_.portions.empty() ? 2 : 3;
    case Stage::Mixing: return 4;
    case Stage::Ready: return 5;
    }
    return -1;
}
bool PlayingState::handleEvent(const sf::Event &event, Pointer mouse) {
    pointer_ = mouse;
    if ((event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) ||
        event.type == sf::Event::LostFocus) {
        onCovered();
        stack_.push(StateID::Pause);
        return false;
    }
    if (briefing_) {
        if (event.type == sf::Event::KeyPressed &&
            (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space))
            startDay();
        else startButton_->handleEvent(event, mouse);
        return false;
    }
    if (day_.ended()) return false;
    if (event.type == sf::Event::MouseLeft) {
        cancelDrag();
        if (paper_)
            paper_->cancel();
        pointer_.reset();
    }
    if (event.type == sf::Event::KeyPressed) {
        const auto key = event.key.code;
        if (key == sf::Keyboard::Space) {
            if (stage_ == Stage::Mixing) hitMix();
            else if (buttons_[3].enabled() && !charge_.active() && !bowlDrag_) primaryAction();
            return false;
        }
        if (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num6) { selectTray(static_cast<std::size_t>(key - sf::Keyboard::Num1)); return false; }
        if (key >= sf::Keyboard::Numpad1 && key <= sf::Keyboard::Numpad6) { selectTray(static_cast<std::size_t>(key - sf::Keyboard::Numpad1)); return false; }
        if (key == sf::Keyboard::Left || key == sf::Keyboard::A) { cycleCustomer(-1); return false; }
        if (key == sf::Keyboard::Right || key == sf::Keyboard::D || key == sf::Keyboard::Tab) { cycleCustomer(1); return false; }
        if (key == sf::Keyboard::BackSpace || key == sf::Keyboard::Z) { undoPortion(); return false; }
        if ((key == sf::Keyboard::Q || key == sf::Keyboard::PageUp) && page_ > 0 && !charge_.active()) { --page_; refreshButtons(); return false; }
        if ((key == sf::Keyboard::E || key == sf::Keyboard::PageDown) && (page_ + 1) * 6 < available_.size() && !charge_.active()) { ++page_; refreshButtons(); return false; }
    }
    if (event.type == sf::Event::MouseMoved && charge_.active() && (!mouse || !overBowl(*mouse))) {
        cancelCharge();
        say("Đã hủy lượt thêm vì chuột rời khỏi tô.");
        refreshButtons();
    }
    if (event.type == sf::Event::MouseMoved && stage_ == Stage::Paper) {
        if (mouse) paper_->move(*mouse);
        else paper_->cancel();
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
        undoPortion();
        return false;
    }
    if (tutorial_ && !(charge_.active() || bowlDrag_)) skipTutorial_->handleEvent(event, mouse);
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && mouse) {
        if (tutorial_ && tutorialCard.contains(*mouse)) return false;
        for (std::size_t i = 0; i < day_.customers().size(); ++i)
            if (customerArea(i).contains(*mouse)) {
                cancelCharge();
                if (selectedId_ != day_.customers()[i].id) sound("ui_click");
                selectedId_ = day_.customers()[i].id;
                tutorialPressed_ = tutorialPressed_ || tutorialStep() == 0;
                return false;
            }
        if (stage_ == Stage::Paper) {
            paper_->begin(*mouse);
            if (paper_->active()) tutorialPressed_ = true;
        }
        if (stage_ == Stage::Toppings && drops_.empty()) {
            for (std::size_t slot = 0; slot < 6; ++slot) {
                const auto index = page_ * 6 + slot;
                if (index < available_.size() && tray(slot).contains(*mouse)) {
                    cancelCharge();
                    selectTray(slot);
                    return false;
                }
            }
            if (overBowl(*mouse)) {
                if (selectedTopping_ && static_cast<int>(bowl_.portions.size()) < day_.capacity()) {
                    charge_.begin();
                    if (context_.audio) context_.audio->charging(true, game().data.number("topping_charge_seconds"));
                    refreshButtons();
                } else say(selectedTopping_ ? "Tô đã đầy, hãy bấm Trộn món hoặc bỏ bớt muỗng." : "Chọn một khay topping trước nhé!");
                return false;
            }
        }
        if (stage_ == Stage::Mixing && mixBar.contains(*mouse)) {
            hitMix();
            return false;
        }
        if (stage_ == Stage::Ready && overBowl(*mouse)) {
            bowlDrag_ = true;
            return false;
        }
    }
    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (stage_ == Stage::Paper) {
            if (mouse)
                paper_->move(*mouse);
            if (const auto quality = paper_->release()) {
                bowl_.paperQuality = *quality;
                stage_ = Stage::Toppings;
                say("Đã cho bánh tráng vừa xé vào tô. Chọn topping, giữ trên tô rồi thả để thêm.");
                refreshButtons();
            }
        }

        if (charge_.active()) {
            if (stage_ == Stage::Toppings && selectedTopping_ && mouse && overBowl(*mouse)) {
                const int count = charge_.release();
                if (context_.audio) context_.audio->charging(false);
                if (count > 0) addPortions(game().data.toppings[*selectedTopping_].id, count);
                else say("Giữ lâu hơn một chút để lấy topping.");
            } else cancelCharge();
            refreshButtons();
            return false;
        }
        if (bowlDrag_) {
            bowlDrag_ = false;
            if (mouse)
                for (std::size_t i = 0; i < day_.customers().size(); ++i)
                    if (customerArea(i).contains(*mouse)) {
                        serve(i);
                        break;
                    }
            return false;
        }
    }
    if (!charge_.active() && !bowlDrag_)
        handleButtons(event, mouse);
    return false;
}
bool PlayingState::update(float dt) {
    animation_ += dt;
    messageAge_ += dt;
    walletBump_ = std::max(0.f, walletBump_ - dt * 2.5f);
    {
        const auto target = static_cast<float>(game().progress.money);
        walletShown_ += (target - walletShown_) * (1.f - std::exp(-5.f * std::max(0.f, dt)));
        if (std::abs(target - walletShown_) < 1.f) walletShown_ = target;
    }
    for (auto &banner : banners_) banner.age += dt;
    while (!banners_.empty() && banners_.front().age >= banners_.front().life) banners_.pop_front();
    for (auto &toast : toasts_) toast.age += dt;
    while (!toasts_.empty() && toasts_.front().age >= 4.2f) toasts_.pop_front();
    for (auto &coin : coins_) {
        coin.age += dt;
        if (coin.age - coin.delay >= .75f) walletBump_ = 1.f;
    }
    coins_.erase(std::remove_if(coins_.begin(), coins_.end(),
                                [](const auto &c) { return c.age - c.delay >= .75f; }), coins_.end());
    cookingFx_.update(dt);
    effects_.update(dt);
    game().liveDay = day_.result();
    game().liveCombo = day_.combo();
    if (briefing_) {
        briefingAge_ += dt;
        startButton_->update(dt);
        return UiState::update(dt);
    }
    for (auto &pending : happySounds_) pending.delay -= std::max(0.f, dt);
    while (!happySounds_.empty() && happySounds_.front().delay <= 0) {
        sound(happySounds_.front().event);
        happySounds_.pop_front();
    }
    selectionAge_ += dt;
    // Cap visual time so a slow frame cannot skip an icon or collapse all pops.
    for (auto &drop : drops_) {
        drop.age += std::min(std::max(dt, 0.f), .05f);
        if (drop.age >= 0 && !drop.popped) {
            sound("ingredient_pop");
            drop.popped = true;
        }
        if (drop.age >= .48f && !drop.landed) {
            const float spread = (static_cast<float>((drop.index * 7) % 5) - 2.f) * 13.f;
            cookingFx_.pulse(ui::CookingEffects::Glint, bowlCenter + sf::Vector2f(spread, -5), 28.f, .3f);
            drop.landed = true;
        }
    }
    drops_.erase(std::remove_if(drops_.begin(), drops_.end(),
        [](const auto &drop) { return drop.age >= .55f; }), drops_.end());
    if (!reviews_.empty()) {
        reviews_.front().remaining -= dt;
        if (reviews_.front().remaining <= 0) reviews_.pop_front();
    }
    if (paper_ && stage_ == Stage::Paper) {
        const int previousStage = paper_->completedStages();
        const float previousElapsed = paper_->elapsed();
        paper_->update(dt);
        if (paper_->elapsed() > previousElapsed && pointer_) {
            tearFxClock_ += dt;
            if (tearFxClock_ >= .045f) {
                cookingFx_.tear(*pointer_);
                tearFxClock_ = 0;
            }
        } else tearFxClock_ = 0;
        if (paper_->completedStages() > previousStage) {
            sound("paper_tear");
            cookingFx_.tear({737, 501}, true);
            say(paper_->done() ? "Bánh đã xé nhỏ! Thả chuột để chuyển sang thêm topping."
                               : "Đã xé thành dải! Tiếp tục giữ và rê để xé thành miếng nhỏ.");
        }
    }
    const int oldLeft = day_.result().left;
    // Only impatient departures have an angry voice; closing time is a separate sound.
    std::vector<bool> leavingVoices;
    if (!day_.ended() && day_.remaining() > dt)
        for (const auto &customer : day_.customers())
            if (customer.remaining <= dt)
                leavingVoices.push_back(game().data.customers[customer.kind].femaleVoice);
    day_.update(dt);
    if (day_.result().left > oldLeft && !day_.ended()) {
        for (bool female : leavingVoices)
            sound(female ? "female_customer_leave" : "male_customer_leave");
        say("Một khách đã rời quán. Hãy xem lại đơn của khách đang chọn.");
    }
    if (!day_.ended()) {
        if (day_.rush() && !lastRush_) {
            banner("GIỜ CAO ĐIỂM!", "Khách đông hơn • Tip thêm 20%", {255, 190, 92}, 2.8f);
            sound("rush_start");
        } else if (!day_.rush() && lastRush_)
            banner("Hết giờ cao điểm", "Bạn làm tốt lắm!", {200, 236, 180}, 2.0f);
        lastRush_ = day_.rush();
        if (day_.combo() < lastCombo_ && lastCombo_ >= 2) effects_.say("Mất combo", {737, 372}, ui::Theme{}.red, 1.4f, 26, true);
        lastCombo_ = day_.combo();
        const int whole = static_cast<int>(std::ceil(day_.remaining()));
        if (!warned20_ && day_.remaining() <= 20.f) {
            warned20_ = true;
            banner("Sắp đóng cửa!", "Còn 20 giây", {255, 150, 120}, 2.2f);
            sound("warning_tick");
        } else if (whole != lastSecond_ && whole <= 5 && whole > 0) sound("warning_tick");
        lastSecond_ = whole;
        collectAchievements();
    }
    game().liveDay = day_.result();
    game().liveCombo = day_.combo();
    if (day_.ended() && !reviews_.empty()) {
        cancelDrag();
        for (auto &button : buttons_) button.setEnabled(false);
        return false;
    }
    if (day_.ended() && !finished_) {
        cancelCharge();
        finished_ = true;
        game().lastDay = day_.result();
        game().hasResult = true;
        game().save();
        sound("day_end");
        stack_.replace(StateID::DayResult);
        return false;
    }
    selectedIndex();
    if (stage_ == Stage::Mixing) {
        mixTrail_.push_back(mix_->position());
        if (mixTrail_.size() > 7) mixTrail_.pop_front();
        mix_->update(dt);
    }
    if (charge_.active()) {
        if (stage_ == Stage::Toppings && pointer_ && overBowl(*pointer_)) {
            const int previousPortions = charge_.portions();
            charge_.update(dt);
            if (charge_.portions() > previousPortions)
                cookingFx_.pulse(ui::CookingEffects::Glint, {610 + 254 * charge_.power(), 644},
                                  28.f + charge_.portions() * 8.f, .4f);
            if (context_.audio) context_.audio->charging(charge_.power() < 1.f, game().data.number("topping_charge_seconds"));
        }
        else {
            cancelCharge();
            say("Đã hủy lượt thêm vì chuột rời khỏi tô.");
        }
    }
    refreshButtons();
    if (tutorial_) skipTutorial_->update(dt);
    UiState::update(dt);
    return false;
}
void PlayingState::drawCustomers(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const bool decorated = hasDecorations();
    if (day_.customers().empty()) {
        ui::outlinedText(target, context_.resources.titleFont(), "Khách đang đến… Chuẩn bị một phần bánh tráng nhé!", 22,
                         {640, 228}, {255, 246, 214}, {94, 53, 34});
        return;
    }
    const auto selectedNow = selectedIndex();
    for (std::size_t i = 0; i < day_.customers().size(); ++i) {
        const auto &c = day_.customers()[i];
        const auto &kind = game().data.customers[c.kind];
        auto r = customerArea(i);
        const float center = r.left + r.width / 2.f - (1.f - ui::easeOut(c.arrival)) * 10.f;
        const int chairLevel = UpgradeSystem::level(game().progress, "patience");
        if (chairLevel > 0) {
            ui::image(target, context_.resources, "props/plastic_chair_blue", {center - 93, 286, 70, 77});
        }
        const bool selected = c.id == selectedId_;
        const bool hovered = pointer_ && r.contains(*pointer_);
        const float mood = c.remaining / c.patience;
        // Bóng dưới chân và vầng sáng khi được chọn giúp khách "đứng" trên quầy.
        ui::radial(target, {center, 357}, {82, 11}, {46, 28, 14, 70}, {46, 28, 14, 0});
        if (selected) ui::radial(target, {center, 350}, {96, 26}, {255, 226, 140, 120}, {255, 226, 140, 0}, true);
        else if (hovered) ui::radial(target, {center, 350}, {84, 20}, {255, 240, 200, 60}, {255, 240, 200, 0}, true);
        const int extraRows = std::max(0, static_cast<int>((c.order.recipe.size() + 1) / 2) - 2);
        const float extraHeight = extraRows * (decorated ? 21.f : 25.f);
        const sf::FloatRect bubble{r.left + 9, r.top + 3, r.width - 18, (decorated ? 88.f : 115.f) + extraHeight};
        if (selected) ui::panelOutline(target, {bubble.left + 3, bubble.top + 3, bubble.width - 6, bubble.height + 10}, {255, 214, 98, 150}, 18, 3);
        if (!ui::frame(target, context_.resources, "ui/cozy_frames/chat_bubble",
                       {bubble.left, bubble.top, bubble.width, bubble.height + 16}, {48, 64}, {15, 20}))
            ui::panel(target, bubble, theme.paper, 18);
        ui::text(target, context_.resources.dialogFont(), kind.special ? "Chị Dung • Tip 100% giá món" : "Cho mình một phần bánh tráng!", r.width < 280 ? 12 : r.width < 320 ? 14 : 16,
                 {bubble.left + 13, bubble.top + 12}, theme.ink);
        int itemIndex = 0;
        for (const auto &item : c.order.recipe) {
            const auto &t = game().data.topping(item.first);
            const float x = bubble.left + 13 + (itemIndex % 2) * (bubble.width - 24) / 2;
            const float y = bubble.top + (decorated ? 32.f : 42.f) + (itemIndex / 2) * (decorated ? 21.f : 25.f);
            if (!ui::image(target, context_.resources, "toppings/" + t.id + "_icon", {x, y - 2, 21, 21}))
                ui::ingredient(target, context_.resources, t, {x + 10, y + 8}, .5f);
            // Khách đang chọn: hiện số muỗng đã có/cần để đối chiếu tức thì.
            const bool tracking = selected && stage_ != Stage::Paper;
            const int have = bowl_.count(item.first);
            const bool exact = tracking && have == item.second;
            const bool over = tracking && have > item.second;
            const std::string amount = tracking ? std::to_string(have) + "/" + std::to_string(item.second)
                                                : "×" + std::to_string(item.second);
            ui::textFit(target, font, t.name + " " + amount, r.width < 280 ? 11 : r.width < 320 ? 12 : 14,
                        {x + 25, y + 2}, over ? theme.red : exact ? theme.green : theme.ink,
                        (bubble.width - 24) / 2 - 42);
            if (exact) ui::checkMark(target, {x + (bubble.width - 24) / 2 - 22, y + 9}, 7, theme.green);
            ++itemIndex;
        }
        ui::text(target, font, moneyText(c.order.price) + (selected ? "  •  Đang làm" : "  •  Bấm chọn"), 12,
                 {center, bubble.top + bubble.height - (decorated ? 10.f : 14.f)}, selected ? theme.green : theme.muted, true);
        // The original artwork is a bust: keep its aspect ratio and let the counter hide its lower edge.
        const std::string emotion = mood < .3f ? "angry" : mood > .7f ? "happy" : "normal";
        const float bob = std::sin(animation_ * 2.1f + static_cast<float>(c.id) * 1.7f) * 2.2f;
        const float shake = mood < .3f ? std::sin(animation_ * 38.f + static_cast<float>(c.id)) * 1.6f : 0.f;
        if (!ui::image(target, context_.resources, "customers/" + kind.style + "_" + emotion,
                       {center - 95 + shake, (decorated ? 237.f : 202.f) + extraHeight + bob * (selected ? 1.4f : 1.f), 190, (decorated ? 125.f : 164.f) - extraHeight}))
            ui::customer(target, context_.resources, kind, {center, 290}, mood, decorated ? 1.9f : 2.5f);
        const std::string nameFrame = selected ? "customer_name" : "customer_name_unselected";
        if (!ui::frame(target, context_.resources, "ui/cozy_frames/" + nameFrame,
                       {center - 110, 337, 220, 28}, {54, 48}, {14, 13}))
            ui::panel(target, {center - 110, 337, 220, 28}, theme.paper, 12);
        ui::text(target, font, kind.name, 14, {center, 351}, theme.ink, true);
        if (kind.special) ui::star(target, {center - 89, 351}, 7, {210, 156, 48});
        // Thanh kiên nhẫn đổi màu và nhấp nháy khi sắp bỏ về, kèm số giây còn lại.
        const bool critical = mood < .18f;
        sf::Color barColor = mood < .3f ? sf::Color(203, 92, 66) : mood < .55f ? sf::Color(224, 160, 62) : theme.green;
        if (critical && std::fmod(animation_, .5f) < .25f) barColor = {235, 70, 50};
        ui::ProgressBar::draw(target, {center - 100, 361, 200, 6}, mood, barColor);
        ui::text(target, context_.resources.smallFont(), std::to_string(static_cast<int>(std::ceil(c.remaining))) + "s", 11,
                 {center + 92, 351}, mood < .3f ? theme.red : theme.muted, true);
        (void)selectedNow;
    }
}
void PlayingState::drawBowlPanel(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    if (stage_ != Stage::Toppings && stage_ != Stage::Ready) return;
    ui::shadowPanel(target, {466, 440, 168, 140}, {255, 250, 235, 225}, 16, 4, 45);
    ui::text(target, font, "TRONG TÔ", 12, {550, 456}, theme.muted, true);
    const int count = static_cast<int>(bowl_.portions.size());
    const bool full = count >= day_.capacity();
    ui::text(target, context_.resources.titleFont(), std::to_string(count) + " / " + std::to_string(day_.capacity()), 28,
             {550, 484}, full ? theme.red : theme.ink, true);
    ui::ProgressBar::draw(target, {484, 508, 132, 8}, static_cast<float>(count) / static_cast<float>(std::max(1, day_.capacity())),
                          full ? theme.red : theme.green);
    int need = 0;
    bool known = false;
    const auto &customers = day_.customers();
    for (const auto &c : customers)
        if (c.id == selectedId_) {
            known = true;
            for (const auto &item : c.order.recipe) need += item.second;
        }
    if (known) {
        const bool match = need == count;
        ui::text(target, font, "Đơn cần " + std::to_string(need) + " muỗng", 14, {550, 536}, match ? theme.green : theme.muted, true);
        if (match) ui::checkMark(target, {622, 536}, 7, theme.green);
        ui::text(target, font, count < need ? "Còn thiếu " + std::to_string(need - count)
                              : count > need ? "Thừa " + std::to_string(count - need) : "Vừa đủ!", 13,
                 {550, 558}, count > need ? theme.red : match ? theme.green : theme.muted, true);
    }
}
void PlayingState::drawWorktop(sf::RenderTarget &target) {
    const auto &font = context_.resources.uiFont();
    const ui::Theme theme;
    ui::panel(target, {998, 375, 252, 280}, {255, 250, 235, 210}, 20);
    const auto selected = selectedIndex();
    for (std::size_t slot = 0; slot < 6; ++slot) {
        const auto n = page_ * 6 + slot;
        if (n >= available_.size())
            break;
        const auto &t = game().data.toppings[available_[n]];
        const auto r = tray(slot);
        const bool chosen = stage_ == Stage::Toppings && selectedTopping_ == available_[n];
        const bool hovered = pointer_ && r.contains(*pointer_) && stage_ == Stage::Toppings && drops_.empty();
        // Transparent hit areas: bowls sit directly on the painted wooden counter.
        sf::CircleShape shadow(51.f);
        shadow.setOrigin(51.f, 51.f);
        shadow.setScale(1.f, .28f);
        shadow.setPosition(r.left + r.width / 2, r.top + 80);
        shadow.setFillColor({61, 36, 20, 35});
        target.draw(shadow);
        if (chosen) {
            shadow.setFillColor(sf::Color::Transparent);
            shadow.setOutlineColor(theme.green);
            shadow.setOutlineThickness(3.f);
            shadow.setScale(1.12f, .45f);
            shadow.setPosition(r.left + r.width / 2, r.top + 65);
            target.draw(shadow);
        } else if (hovered) ui::radial(target, {r.left + r.width / 2, r.top + 70}, {62, 22}, {255, 240, 190, 90}, {255, 240, 190, 0}, true);
        const std::string artwork = t.type == "sauce" ? "tools/bottle_" + t.id : "toppings/" + t.id + "_tray";
        const float lift = chosen ? 7.f + 9.f * std::sin(std::min(selectionAge_ / .28f, 1.f) * 3.14159f) : (hovered ? 4.f : 0.f);
        if (!ui::image(target, context_.resources, artwork, {r.left + 1, r.top - lift, r.width - 2, 106}))
            ui::ingredient(target, context_.resources, t, {r.left + r.width / 2, r.top + 48}, 2.3f);
        int required = 0;
        if (selected) {
            const auto &recipe = day_.customers()[*selected].order.recipe;
            const auto it = recipe.find(t.id);
            if (it != recipe.end())
                required = it->second;
        }
        const int have = bowl_.count(t.id);
        ui::text(target, context_.resources.smallFont(),
                 t.name + "  " + std::to_string(have) + "/" + std::to_string(required), 14,
                 {r.left + r.width / 2, r.top + 111}, have > required ? theme.red : chosen || required > 0 ? theme.green : theme.ink, true);
        if (stage_ == Stage::Toppings)
            ui::keycap(target, font, std::to_string(slot + 1), {r.left + 14, r.top + 12});
    }
    if (stage_ == Stage::Paper) {
        const int completed = paper_->completedStages();
        const char *artwork = completed == 0 ? "rice_paper/paper_whole"
                              : completed == 1 ? "paper_tearing/rice_paper/paper_torn_stage_01"
                                               : "paper_tearing/rice_paper/paper_torn_stage_02";
        const int boardLevel = UpgradeSystem::level(game().progress, "paper");
        if (boardLevel > 0) {
            ui::image(target, context_.resources, "tools/cutting_board", {610, 427, 254, 190});
        }
        ui::radial(target, {737, 612}, {104, 14}, {61, 36, 20, 60}, {61, 36, 20, 0});
        ui::image(target, context_.resources, artwork, paperBounds);
        ui::ProgressBar::draw(target, {625, 640, 224, 8}, paper_->progress(), theme.green);
        ui::panel(target, {736, 638, 2, 12}, theme.ink, 0);
        ui::text(target, font, "Xé: " + std::to_string(static_cast<int>(paper_->progress() * 100)) + "%", 13,
                 {737, 657}, theme.muted, true);
    } else if (stage_ == Stage::Mixing) {
        ui::bowl(target, context_.resources, game().data, bowl_, {737, 582}, .40f, false, toppingsComplete());
        const float width = 360;
        ui::panel(target, {551, 465, width + 12, 48}, {130, 80, 41}, 14);
        ui::panel(target, {553, 467, width + 8, 44}, {245, 199, 106}, 12);
        ui::panel(target, {557, 471, width, 36}, {102, 73, 49}, 9);
        ui::panel(target, {557 + width * (mix_->target() - mix_->width() / 2), 471, width * mix_->width(), 36},
                  {100, 170, 106}, 5);
        ui::panel(target, {557 + width * (mix_->target() - mix_->width() * .2f), 471, width * mix_->width() * .4f, 36},
                  {213, 236, 150}, 4);
        ui::panel(target, {557 + width * (mix_->target() - mix_->width() / 2) + 3, 474,
                            width * mix_->width() - 6, 3}, {239, 249, 192}, 1);
        cookingFx_.sprite(target, ui::CookingEffects::Glint, {557 + width * mix_->target(), 489},
                           24.f + 3.f * std::sin(animation_ * 5.f), 0, .65f);
        for (std::size_t i = 0; i < mixTrail_.size(); ++i)
            ui::panel(target, {557 + width * mixTrail_[i] - 2, 474, 4, 30},
                       {255, 235, 159, static_cast<sf::Uint8>(18 + i * 9)}, 2);
        ui::panel(target, {557 + width * mix_->position() - 5, 460, 10, 58}, {255, 242, 198}, 4);
        ui::panel(target, {557 + width * mix_->position() - 3, 462, 6, 54}, theme.ink, 3);
        cookingFx_.sprite(target, ui::CookingEffects::Glint,
                           {557 + width * mix_->position(), 462}, 22.f);
        // Chấm tròn thể hiện số nhịp đã trúng/trượt.
        const int total = game().data.integer("mix_hits");
        for (int i = 0; i < total; ++i) {
            sf::CircleShape dot(7.f);
            dot.setOrigin(7, 7);
            dot.setPosition(737.f + (static_cast<float>(i) - (total - 1) / 2.f) * 22.f, 443.f);
            dot.setFillColor(i < mix_->hits() ? theme.green : sf::Color(224, 217, 197));
            dot.setOutlineColor(theme.wood);
            dot.setOutlineThickness(2.f);
            target.draw(dot);
        }
        ui::keycap(target, font, "Space", {737, 540});
    } else {
        if (!bowlDrag_) {
            ui::radial(target, {bowlCenter.x, bowlCenter.y + 66 * bowlScale() / .62f}, {120 * bowlScale() / .62f, 16}, {61, 36, 20, 70}, {61, 36, 20, 0});
            ui::bowl(target, context_.resources, game().data, bowl_, bowlCenter, bowlScale(),
                     charge_.active(), toppingsComplete());
        }
        drawBowlPanel(target);
        if (stage_ == Stage::Toppings && selectedTopping_) {
            const auto &chosen = game().data.toppings[*selectedTopping_];
            const int space = std::max(0, day_.capacity() - static_cast<int>(bowl_.portions.size()));
            ui::text(target, font, chosen.name + (charge_.active()
                         ? " • Thả: +" + std::to_string(std::min(space, charge_.portions())) + " muỗng"
                         : " • Giữ trên tô để lấy"), 14, {737, 619}, theme.green, true);
            // Three spoon thresholds share the exact ratio used when releasing the charge.
            ui::panel(target, {606, 632, 262, 24}, {139, 89, 49}, 10);
            ui::panel(target, {608, 634, 258, 20}, {255, 224, 162}, 8);
            ui::panel(target, {610, 636, 254, 16}, {105, 82, 55}, 6);
            const sf::Color colors[] = {{131, 192, 112}, {242, 191, 72}, {245, 139, 65}};
            for (int tier = 0; tier < 3; ++tier) {
                const float start = tier / 3.f;
                const float fill = std::clamp(charge_.power() - start, 0.f, 1.f / 3);
                if (fill > 0) {
                    ui::panel(target, {610 + 254 * start, 636, 254 * fill, 16}, colors[tier], 3);
                    ui::panel(target, {610 + 254 * start, 638, 254 * fill, 3}, {255, 249, 208, 170}, 1);
                }
                ui::text(target, context_.resources.smallFont(), std::to_string(tier + 1), 11,
                         {610 + 254 * (tier + .5f) / 3, 644}, {255, 251, 226}, true);
                if (tier > 0) ui::panel(target, {610 + 254 * start, 635, 2, 18}, {255, 230, 175}, 0);
            }
            if (charge_.active()) {
                const float x = 610 + 254 * charge_.power();
                ui::panel(target, {x - 1, 633, 2, 22}, {255, 253, 215}, 1);
                cookingFx_.sprite(target, ui::CookingEffects::Glint, {x, 644},
                    23.f + 7.f * charge_.power() + 3.f * std::sin(animation_ * 12.f));
            }
        } else if (stage_ == Stage::Ready) {
            // Gợi ý thao tác giao món: nhấp nháy nhẹ để người chơi không bỏ sót.
            const float pulse = .5f + .5f * std::sin(animation_ * 4.f);
            ui::text(target, font, "Kéo tô tới khách", 15, {737, 640 + pulse * 3}, theme.green, true);
        }
    }
    const int spoonLevel = UpgradeSystem::level(game().progress, "mix");
    if (spoonLevel > 0) {
        const float sway = stage_ == Stage::Mixing ? std::sin(animation_ * 7.f) * 12.f : 0.f;
        ui::image(target, context_.resources, "tools/spoon", {923, 438, 44.f + spoonLevel * 5.f, 140}, -18.f + sway);
    }
    ui::text(target, font, "MÓN CỦA BẠN", 16, {1125, 391}, theme.muted, true);
    const char *steps[] = {"Xé bánh tráng", "Thêm topping", "Trộn đều", "Giao cho khách"};
    for (int i = 0; i < 4; ++i) {
        const bool current = static_cast<int>(stage_) == i;
        const bool done = static_cast<int>(stage_) > i;
        const float y = 418 + static_cast<float>(i) * 27;
        if (done) ui::checkMark(target, {1006, y + 11}, 7, theme.green);
        else if (current) {
            sf::CircleShape dot(4.f);
            dot.setOrigin(4, 4);
            dot.setPosition(1006, y + 11);
            dot.setFillColor(theme.orange);
            target.draw(dot);
        }
        ui::text(target, font, std::to_string(i + 1) + ". " + steps[i], 17, {1020, y}, current ? theme.green : done ? theme.ink : theme.muted);
    }
    if (stage_ != Stage::Paper)
        ui::text(target, font,
                 "Chất lượng xé: " + std::to_string(static_cast<int>(bowl_.paperQuality * 100)) + "%", 15,
                 {1016, 521}, theme.muted);
    ui::text(target, font, "Space: tiếp tục  •  Chuột phải: bỏ muỗng", 12, {1124, 667}, {255, 244, 214}, true);
}
void PlayingState::drawHint(sf::RenderTarget &target) {
    const ui::Theme theme;
    const std::string shown = messageAge_ < 6.f ? message_ : stageHint();
    ui::shadowPanel(target, hintPill, {255, 250, 232, 238}, 17, 3, 40);
    if (messageAge_ < .35f)
        ui::panelOutline(target, {hintPill.left + 2, hintPill.top + 2, hintPill.width - 4, hintPill.height - 4}, {255, 214, 98, static_cast<sf::Uint8>(220 * (1.f - messageAge_ / .35f))}, 17, 2);
    ui::textFit(target, context_.resources.uiFont(), shown, 15,
                {hintPill.left + hintPill.width / 2, hintPill.top + hintPill.height / 2 - 1}, theme.ink, hintPill.width - 28, true, 11);
}
void PlayingState::drawTutorial(sf::RenderTarget &target) {
    const int step = tutorialStep();
    if (step < 0 || briefing_) return;
    const ui::Theme theme;
    const char *texts[] = {
        "Chọn một khách (bấm vào) và xem đơn: mỗi dòng là một topping cùng số muỗng cần làm. Rồi giữ chuột và rê lên bánh tráng.",
        "Giữ chuột trái và rê qua lại trên bánh để xé. Thả chuột để nghỉ, tiến độ vẫn được giữ.",
        "Bấm một khay topping (hoặc phím 1-6), giữ chuột trên tô rồi thả: giữ càng lâu càng nhiều muỗng (1-3).",
        "Làm đúng số muỗng trong đơn. Lỡ tay thì bấm Bỏ muỗng cuối (hoặc chuột phải). Đủ rồi bấm Trộn món.",
        "Bấm Space hoặc bấm vào thanh khi vạch chạy vào vùng xanh. Cần đủ 3 nhịp.",
        "Kéo tô đến khách hoặc bấm Giao món. Giao đúng đơn và nhanh để có nhiều sao, tip và combo!"};
    sf::FloatRect focus;
    const auto selected = const_cast<PlayingState *>(this)->selectedIndex();
    switch (step) {
    case 0: case 5: focus = selected ? customerArea(*selected) : sf::FloatRect(28, 92, 300, 270); break;
    case 1: focus = paperBounds; break;
    case 2: focus = {40, 414, 414, 246}; break;
    case 3: focus = mainButtonRect; break;
    default: focus = {551, 465, 372, 48}; break;
    }
    ring(target, focus, animation_, theme.gold);
    ui::shadowPanel(target, tutorialCard, {255, 247, 220, 245}, 14, 4, 50);
    ui::panelOutline(target, tutorialCard, {226, 168, 58, 200}, 14, 2);
    ui::panel(target, {tutorialCard.left + 10, tutorialCard.top + 8, 98, 22}, theme.gold, 11);
    ui::text(target, context_.resources.uiFont(), "Hướng dẫn " + std::to_string(step + 1) + "/6", 13,
             {tutorialCard.left + 59, tutorialCard.top + 19}, {255, 250, 232}, true);
    ui::paragraph(target, context_.resources.uiFont(), texts[step], 13,
                  {tutorialCard.left + 12, tutorialCard.top + 38}, tutorialCard.width - 24, 16, theme.ink);
    skipTutorial_->render(target);
}
void PlayingState::drawHud(sf::RenderTarget &target) {
    const ui::Theme theme;
    const bool rush = day_.rush();
    const float remaining = day_.remaining();
    const bool closing = remaining <= 20.f && !day_.ended();
    if (!ui::frame(target, context_.resources, "ui/cozy_frames/top_panel",
                   {12, 8, 1256, 76}, {80, 48}, {40, 24}))
        ui::panel(target, {12, 8, 1256, 76}, theme.paper, 16);
    if (rush) {
        const float pulse = .5f + .5f * std::sin(animation_ * 6.f);
        ui::radial(target, {640, 46}, {620, 40}, {255, 170, 60, static_cast<sf::Uint8>(40 + 40 * pulse)}, {255, 170, 60, 0}, true);
    }
    ui::text(target, context_.resources.titleFont(), "Ngày " + std::to_string(game().progress.day),
             24, {640, 30}, theme.ink, true);
    if (rush)
        ui::text(target, context_.resources.titleFont(), "GIỜ CAO ĐIỂM  " + std::to_string(static_cast<int>(std::ceil(day_.rushRemaining()))) + "s", 15,
                 {640, 52}, theme.orange, true);
    else
        ui::text(target, context_.resources.uiFont(),
                 "Đã phục vụ " + std::to_string(day_.result().served) + "  •  " + moneyText(day_.result().revenue + day_.result().tips), 13,
                 {640, 52}, theme.muted, true);
    const int time = static_cast<int>(std::ceil(remaining));
    ui::image(target, context_.resources, "ui/hourglass_icon", {65, 25, 26, 30});
    sf::Color timeColor = theme.ink;
    if (closing) timeColor = std::fmod(animation_, 1.f) < .5f ? theme.red : sf::Color(235, 90, 60);
    ui::text(target, context_.resources.smallFont(),
             std::to_string(time / 60) + ":" + (time % 60 < 10 ? "0" : "") + std::to_string(time % 60),
             25, {108, 26}, timeColor);
    ui::ProgressBar::draw(target, {260, 65, 760, 5}, remaining / game().data.number("day_seconds"),
                          closing ? theme.red : rush ? theme.orange : theme.green);
    // Combo: chip vàng khi đang có chuỗi, mờ khi chưa có.
    const int combo = day_.combo();
    const sf::FloatRect chip{210, 18, 170, 40};
    if (combo >= 2) {
        ui::panel(target, {chip.left, chip.top + 2, chip.width, chip.height}, {170, 110, 30, 90}, 14);
        ui::panel(target, chip, theme.gold, 14);
        ui::panel(target, {chip.left + 2, chip.top + 2, chip.width - 4, chip.height / 2 - 2}, {255, 220, 130, 150}, 12);
        ui::text(target, context_.resources.titleFont(), "COMBO x" + std::to_string(combo), 19,
                 {chip.left + chip.width / 2, chip.top + 15}, {255, 252, 238}, true);
        const int bonus = static_cast<int>(std::round(game().data.numberOr("combo_step", .04f) * 100.f *
                              static_cast<float>(std::min(combo - 1, static_cast<int>(game().data.numberOr("combo_max", 8.f))))));
        ui::text(target, context_.resources.uiFont(), "Tip thêm +" + std::to_string(bonus) + "%", 12,
                 {chip.left + chip.width / 2, chip.top + 31}, {255, 246, 220}, true);
    } else {
        ui::panel(target, chip, {236, 224, 196, 190}, 14);
        ui::text(target, context_.resources.uiFont(), "Combo x0", 14, {chip.left + chip.width / 2, chip.top + 14}, theme.muted, true);
        ui::text(target, context_.resources.uiFont(), "Giao liên tiếp đơn 4 sao+", 11, {chip.left + chip.width / 2, chip.top + 29}, theme.muted, true);
    }
    // Ba nhiệm vụ của ngày.
    const auto &goals = day_.goals();
    for (std::size_t i = 0; i < goals.size() && i < 3; ++i) {
        const auto &goal = goals[i];
        const float x = 716 + static_cast<float>(i) * 122;
        const sf::FloatRect box{x, 16, 116, 44};
        const bool failed = goal.id == "nolose" && day_.result().left > 0;
        const bool done = goal.done();
        ui::panel(target, box, done ? sf::Color(214, 236, 196, 240) : failed ? sf::Color(240, 208, 196, 230) : sf::Color(255, 250, 232, 225), 11);
        ui::textFit(target, context_.resources.uiFont(), goalShort(goal), 12, {x + 8, box.top + 5}, done ? theme.green : theme.ink, 82);
        const std::string progress = failed ? "Hỏng" : goalProgressText(goal);
        ui::text(target, context_.resources.uiFont(), progress, 11, {x + box.width - 8 - ui::textWidth(context_.resources.uiFont(), progress, 11), box.top + 6}, failed ? theme.red : theme.muted);
        const float ratio = goal.id == "nolose" ? (done ? 1.f : 0.f) : static_cast<float>(goal.progress) / static_cast<float>(std::max(1, goal.target));
        ui::ProgressBar::draw(target, {x + 8, box.top + 30, box.width - 16, 6}, ratio, done ? theme.green : theme.orange);
        if (done) ui::checkMark(target, {x + box.width - 14, box.top + 33}, 7, theme.green);
    }
}
void PlayingState::drawReviewCard(sf::RenderTarget &target) {
    if (reviews_.empty()) return;
    const ui::Theme theme;
    const auto &review = reviews_.front();
    const float appear = std::min(1.f, (3.5f - review.remaining) / .25f);
    const float fade = std::min(1.f, std::max(0.f, review.remaining) / .5f);
    const auto alpha = static_cast<sf::Uint8>(255.f * fade);
    const sf::FloatRect card{440, 362.f - 8.f * (1.f - appear), 400, 70};
    ui::shadowPanel(target, card, {255, 249, 228, alpha}, 14, 4, static_cast<sf::Uint8>(50 * fade));
    ui::panelOutline(target, card, {226, 168, 58, static_cast<sf::Uint8>(190 * fade)}, 14, 2);
    ui::text(target, context_.resources.titleFont(), review.customer, 17, {card.left + 14, card.top + 7}, {theme.ink.r, theme.ink.g, theme.ink.b, alpha});
    for (int i = 0; i < 5; ++i) {
        const auto *texture = context_.resources.texture(i < review.receipt.stars()
            ? "assets/images/cozy/ui/star_full.png" : "assets/images/cozy/ui/star_empty.png");
        if (!texture) continue;
        const auto size = texture->getSize();
        // Sao nảy lần lượt để tạo cảm giác thưởng.
        const float pop = easeOutBack((3.5f - review.remaining - static_cast<float>(i) * .08f) / .3f);
        sf::Sprite star(*texture);
        star.setOrigin(size.x / 2.f, size.y / 2.f);
        star.setScale(24.f / size.x * pop, 24.f / size.y * pop);
        star.setPosition(card.left + card.width - 128 + static_cast<float>(i) * 25.f, card.top + 19);
        star.setColor({255, 255, 255, alpha});
        target.draw(star);
    }
    ui::textFit(target, context_.resources.uiFont(), review.receipt.review(), 14, {card.left + 14, card.top + 30},
                {theme.muted.r, theme.muted.g, theme.muted.b, alpha}, card.width - 28);
    std::string money = "Món +" + moneyText(review.receipt.base);
    const int plainTip = review.receipt.tip - review.receipt.comboBonus - review.receipt.rushBonus;
    if (plainTip > 0) money += "  •  Tip +" + moneyText(plainTip);
    if (review.receipt.comboBonus > 0) money += "  •  Combo +" + moneyText(review.receipt.comboBonus);
    if (review.receipt.rushBonus > 0) money += "  •  Cao điểm +" + moneyText(review.receipt.rushBonus);
    ui::textFit(target, context_.resources.uiFont(), money, 13, {card.left + 14, card.top + 50},
                {theme.green.r, theme.green.g, theme.green.b, alpha}, card.width - 28);
}
void PlayingState::drawOverlays(sf::RenderTarget &target) {
    const ui::Theme theme;
    // Xu bay về ví.
    for (const auto &coin : coins_) {
        const float t = (coin.age - coin.delay) / .75f;
        if (t < 0) continue;
        const float e = ui::easeOut(t);
        const sf::Vector2f control{(coin.from.x + walletPos.x) / 2.f, std::min(coin.from.y, walletPos.y) - 90.f};
        const sf::Vector2f p = (1 - e) * (1 - e) * coin.from + 2 * (1 - e) * e * control + e * e * walletPos;
        ui::image(target, context_.resources, "ui/coin_icon", {p.x - 11, p.y - 11, 22, 22}, e * 360.f);
    }
    // Tooltip khay topping khi rê chuột.
    if (pointer_ && stage_ == Stage::Toppings && drops_.empty() && !charge_.active() && !bowlDrag_) {
        for (std::size_t slot = 0; slot < 6; ++slot) {
            const auto n = page_ * 6 + slot;
            if (n >= available_.size() || !tray(slot).contains(*pointer_)) continue;
            const auto &t = game().data.toppings[available_[n]];
            const std::string line = t.name + "  •  " + typeName(t.type) + "  •  +" + moneyText(t.unitPrice) + "/muỗng";
            const float w = ui::textWidth(context_.resources.uiFont(), line, 14) + 24;
            const sf::FloatRect box{std::clamp(pointer_->x + 14, 8.f, 1272.f - w), pointer_->y - 44, w, 28};
            ui::shadowPanel(target, box, {255, 250, 232, 245}, 10, 3, 50);
            ui::text(target, context_.resources.uiFont(), line, 14, {box.left + 12, box.top + 6}, theme.ink);
            break;
        }
    }
    // Thông báo thành tựu.
    if (!toasts_.empty()) {
        const auto &toast = toasts_.front();
        const float slide = ui::easeOut(std::min(1.f, toast.age / .4f)) * (toast.age > 3.7f ? 1.f - (toast.age - 3.7f) / .5f : 1.f);
        const sf::FloatRect box{1290.f - 372.f * std::max(0.f, slide), 96, 360, 62};
        ui::shadowPanel(target, box, {255, 248, 222, 250}, 14, 4, 55);
        ui::panelOutline(target, box, theme.gold, 14, 2);
        ui::star(target, {box.left + 34, box.top + 31}, 20, theme.gold);
        ui::text(target, context_.resources.uiFont(), "Thành tựu mới!", 13, {box.left + 66, box.top + 9}, theme.orange);
        for (const auto &info : AchievementSystem::all())
            if (info.id == toast.id) {
                ui::text(target, context_.resources.titleFont(), info.name, 19, {box.left + 66, box.top + 24}, theme.ink);
                break;
            }
    }
    // Banner giữa màn hình (mở cửa, giờ cao điểm, sắp đóng cửa).
    if (!banners_.empty()) {
        const auto &b = banners_.front();
        const float inT = std::min(1.f, b.age / .35f);
        const float outT = std::min(1.f, std::max(0.f, (b.life - b.age) / .45f));
        const auto alpha = static_cast<sf::Uint8>(255.f * std::min(inT, outT));
        const float scale = .75f + .25f * easeOutBack(inT);
        const sf::Vector2f center{640, 262};
        ui::radial(target, center, {360, 70}, {40, 24, 12, static_cast<sf::Uint8>(alpha * .55f)}, {40, 24, 12, 0});
        sf::Text title(utf8(b.title), context_.resources.titleFont(), static_cast<unsigned>(56 * scale));
        const auto tb = title.getLocalBounds();
        title.setOrigin(tb.left + tb.width / 2, tb.top + tb.height / 2);
        title.setPosition(center);
        title.setFillColor({b.color.r, b.color.g, b.color.b, alpha});
        title.setOutlineColor({94, 53, 34, alpha});
        title.setOutlineThickness(3.f);
        target.draw(title);
        ui::outlinedText(target, context_.resources.titleFont(), b.sub, 22, {640, 306}, {255, 246, 224, alpha}, {94, 53, 34, alpha});
    }
}
void PlayingState::drawBriefing(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    sf::RectangleShape dim({1280, 720});
    dim.setFillColor({38, 26, 18, static_cast<sf::Uint8>(150 * std::min(1.f, briefingAge_ / .3f))});
    target.draw(dim);
    const float t = ui::easeOut(std::min(1.f, briefingAge_ / .4f));
    const sf::FloatRect card{320, 86 + (1.f - t) * 30.f, 640, 520};
    ui::shadowPanel(target, card, theme.paper, 26, 8, 70);
    ui::panelOutline(target, card, {226, 168, 58, 160}, 26, 2);
    ui::text(target, context_.resources.titleFont(), "Ngày " + std::to_string(game().progress.day), 44, {640, card.top + 52}, theme.ink, true);
    ui::text(target, font, "Chuẩn bị mở quán", 18, {640, card.top + 92}, theme.muted, true);
    ui::text(target, context_.resources.titleFont(), "Nhiệm vụ hôm nay", 22, {card.left + 40, card.top + 122}, theme.green);
    const auto &goals = day_.goals();
    for (std::size_t i = 0; i < goals.size(); ++i) {
        const float y = card.top + 160 + static_cast<float>(i) * 52;
        ui::panel(target, {card.left + 36, y, card.width - 72, 44}, {250, 241, 214}, 14);
        sf::CircleShape box(10.f);
        box.setOrigin(10, 10);
        box.setPosition(card.left + 62, y + 22);
        box.setFillColor(sf::Color::Transparent);
        box.setOutlineColor(theme.gold);
        box.setOutlineThickness(3.f);
        target.draw(box);
        ui::textFit(target, font, goalText(goals[i]), 17, {card.left + 84, y + 11}, theme.ink, 360);
        const std::string reward = "+" + moneyText(goals[i].reward);
        ui::text(target, font, reward, 17, {card.left + card.width - 52 - ui::textWidth(font, reward, 17), y + 11}, theme.green);
    }
    ui::text(target, context_.resources.titleFont(), "Mẹo nhỏ", 18, {card.left + 40, card.top + 326}, theme.orange);
    ui::paragraph(target, font, "Giao liên tiếp các đơn từ 4 sao để nhân combo (tip tăng dần). Giờ cao điểm sẽ đến bất ngờ: khách đông hơn nhưng tip thêm 20%.",
                  14, {card.left + 40, card.top + 352}, card.width - 80, 20, theme.muted);
    ui::text(target, font, "Ví: " + moneyText(game().progress.money), 16, {640, card.top + 428}, theme.green, true);
    ui::text(target, font, "Enter / Space để bắt đầu  •  ESC để tạm dừng", 13, {640, card.top + 496}, theme.muted, true);
    startButton_->render(target);
}
void PlayingState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    ui::panel(target, {0, 0, 1280, 720}, theme.cream, 0);
    // Offset the source by 70px so its painted counter edge meets the customer baseline.
    // The extra lower strip is plain wood and is clipped by the logical game viewport.
    const bool scene = ui::background(target, context_.resources, "background/playing", {0, 70, 1280, 720});
    if (!scene) {
        ui::shopBackground(target, game().data, game().progress, animation_);
        ui::panel(target, {0, 91, 1280, 275}, {222, 230, 205}, 0);
    }
    // Purchased decorations are part of the scene, behind customers and controls.
    // Draw the flags first so the fabric covers their rope and ends.
    if (UpgradeSystem::effect(game().data, game().progress, "decor") > 0)
        ui::image(target, context_.resources, "purchased_decor/decor/bunting", {104, 78, 1072, 80});
    if (UpgradeSystem::effect(game().data, game().progress, "curtain") > 0) {
        // Preserve the fabric's proportions and repeat the central valance across the shop.
        if (const auto *texture = context_.resources.texture("assets/images/cozy/purchased_decor/decor/curtain.png")) {
            const auto size = texture->getSize();
            const int edge = static_cast<int>(size.x * .21875f);
            const float scale = 192.f / size.y;
            const float edgeWidth = edge * scale;
            const auto part = [&](int left, int width, float x, float displayWidth) {
                sf::Sprite sprite(*texture, {left, 0, width, static_cast<int>(size.y)});
                sprite.setPosition(x, 84);
                sprite.setScale(displayWidth / width, scale);
                target.draw(sprite);
            };
            part(0, edge, 0, edgeWidth);
            const int middle = static_cast<int>(size.x) - 2 * edge;
            for (float x = edgeWidth; x < 1280 - edgeWidth;) {
                const float width = std::min(middle * scale, 1280 - edgeWidth - x);
                part(edge, std::max(1, static_cast<int>(std::ceil(width / scale))), x, width);
                x += width;
            }
            part(static_cast<int>(size.x) - edge, edge, 1280 - edgeWidth, edgeWidth);
        }
    }
    if (UpgradeSystem::effect(game().data, game().progress, "lantern") > 0) {
        // Đèn lồng đung đưa nhẹ và tỏa sáng ấm.
        const float sway = std::sin(animation_ * 1.6f) * 2.5f;
        for (float x : {43.f, 1177.f}) {
            ui::radial(target, {x + 30 + sway, 168}, {58, 64}, {255, 170, 70, 60}, {255, 170, 70, 0}, true);
            ui::image(target, context_.resources, "purchased_decor/decor/lantern", {x + sway, 113, 60, 110});
        }
    }
    // Ánh nắng và bụi bay trên phố tạo chiều sâu cho cảnh.
    ui::lightShafts(target, {0, 92, 1280, 270}, animation_, 26);
    ui::dust(target, {0, 92, 1280, 270}, animation_, 22, 90);
    ui::vignette(target, 46);
    drawHud(target);
    drawCustomers(target);
    // The new scene supplies the wooden worktop; retain a fallback for missing art.
    if (!scene) {
        ui::panel(target, {15, 366, 1250, 291}, {193, 149, 105}, 14);
        ui::panel(target, {15, 366, 1250, 8}, {151, 107, 72}, 3);
    }
    ui::image(target, context_.resources, "ui/coin_icon", {29 - walletBump_ * 3, 372 - walletBump_ * 3, 24 + walletBump_ * 6, 24 + walletBump_ * 6});
    ui::text(target, context_.resources.smallFont(), moneyText(static_cast<int>(std::lround(walletShown_))), 19 + static_cast<unsigned>(walletBump_ * 3),
             {61, 373}, {255, 248, 220});
    drawWorktop(target);
    for (std::size_t i = 0; i < buttons_.size(); ++i)
        if (i != 5 || stage_ == Stage::Toppings) buttons_[i].render(target);
    drawHint(target);
    if (!game().saves.error().empty())
        ui::text(target, font, game().saves.error(), 13, {30, 660}, {171, 65, 43});
    if (charge_.active() && selectedTopping_) {
        const auto &topping = game().data.toppings[*selectedTopping_];
        if (topping.type == "sauce")
            ui::image(target, context_.resources, "tools/bottle_" + topping.id + "_tilt", {790, 444, 62, 75});
        else ui::ingredient(target, context_.resources, topping, {812, 474}, 1.2f);
    }
    for (const auto &drop : drops_) {
        if (drop.age < 0) continue;
        const float progress = std::clamp(drop.age / .55f, 0.f, 1.f);
        const float spread = (static_cast<float>((drop.index * 7) % 5) - 2.f) * 13.f;
        const sf::Vector2f start{805.f + spread * .25f, 457.f};
        const sf::Vector2f end{bowlCenter.x + spread, bowlCenter.y - 5.f};
        const auto position = start + (end - start) * progress + sf::Vector2f(0, -25.f * std::sin(progress * 3.14159f));
        const float scale = .95f * (1.f - .85f * std::max(0.f, (progress - .78f) / .22f));
        ui::ingredient(target, context_.resources, game().data.topping(drop.id), position, scale,
                       (drop.index % 2 ? -1.f : 1.f) * progress * 135.f);
    }
    if (bowlDrag_ && pointer_)
        ui::bowl(target, context_.resources, game().data, bowl_, *pointer_, .5f, true, toppingsComplete());
    // Cảnh báo sắp hết giờ: viền đỏ nhịp thở.
    if (!briefing_ && day_.remaining() <= 20.f && !day_.ended()) {
        const float pulse = .5f + .5f * std::sin(animation_ * 5.f);
        ui::vignette(target, static_cast<sf::Uint8>(30 + 40 * pulse));
        ui::radial(target, {640, 360}, {900, 600}, {0, 0, 0, 0}, {200, 40, 20, static_cast<sf::Uint8>(40 + 40 * pulse)});
    }
    drawTutorial(target);
    cookingFx_.render(target);
    effects_.render(target, font);
    drawReviewCard(target);
    drawOverlays(target);
    if (briefing_) drawBriefing(target);
}
} // namespace tiem

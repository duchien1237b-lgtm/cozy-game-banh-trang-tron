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
    addButton("Giữ và rê bánh", {1010, 541, 227, 44}, [this] {
        if (stage_ == Stage::Toppings)
            startMix();
        else if (stage_ == Stage::Mixing)
            hitMix();
        else if (stage_ == Stage::Ready) {
            if (const auto i = selectedIndex())
                serve(*i);
        }
    });
    addButton("Làm lại tô", {1010, 606, 227, 40}, [this] {
        resetBowl();
        message_ = "Tô mới đã sẵn sàng. Xé bánh tráng trước nhé!";
    });
    resetBowl();
    refreshButtons();
}
PlayingState::~PlayingState() {
    if (context_.audio) context_.audio->charging(false);
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
    selectedId_ = customers.front().id;
    return 0;
}
void PlayingState::refreshButtons() {
    if (buttons_.size() < 5)
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
}
void PlayingState::startMix() {
    if (stage_ != Stage::Toppings || bowl_.portions.empty() || !drops_.empty())
        return;
    cancelDrag();
    stage_ = Stage::Mixing;
    mixTrail_.clear();
    mix_ = std::make_unique<MixingMinigame>(
        game().data, UpgradeSystem::effect(game().data, game().progress, "mix"), game().progress.day);
    message_ = "Bấm khi vạch chạy vào vùng xanh. Cần đủ " + std::to_string(game().data.integer("mix_hits")) +
               " nhịp.";
    refreshButtons();
}
void PlayingState::hitMix() {
    if (!mix_)
        return;
    const float hitPosition = 557 + 360 * mix_->position();
    const auto value = mix_->hit();
    if (!value)
        return;
    cookingFx_.pulse(*value > .9f ? ui::CookingEffects::Perfect :
                     *value > .5f ? ui::CookingEffects::Good : ui::CookingEffects::Miss,
                     {hitPosition, 489}, *value > .9f ? 108.f : 78.f, .65f);
    effects_.say(*value > .9f   ? "Perfect!"
                 : *value > .5f ? "Good!"
                                : "Miss!",
                 {737, 450}, *value > .5f ? sf::Color(60, 118, 70) : sf::Color(182, 90, 56),
                 game().data.number("feedback_seconds"));
    sound(*value > .9f ? "mix_perfect" : *value > .5f ? "mix_good" : "mix_miss");
    if (mix_->done()) {
        bowl_.mixQuality = mix_->score();
        bowl_.mixed = true;
        stage_ = Stage::Ready;
        message_ = "Chọn khách rồi Giao món, hoặc kéo tô đến khách.";
    }
    refreshButtons();
}
void PlayingState::addPortions(const std::string &id, int count) {
    int added = 0;
    while (added < count && bowl_.add(id, day_.capacity())) ++added;
    if (added == 0) {
        message_ = "Tô đã đầy. Hãy trộn hoặc làm lại.";
        return;
    }
    const auto &t = game().data.topping(id);
    message_ = "Đã thêm " + std::to_string(added) + " muỗng " + t.name + ".";
    if (added < count) message_ += " Tô đã đầy!";
    // Two icons per actual portion: 2 / 4 / 6, each with its own pop.
    for (int i = 0; i < added * 2; ++i)
        drops_.push_back({id, -static_cast<float>(i) * .13f, i});
    cookingFx_.pulse(ui::CookingEffects::Glint, {610 + 254 * added / 3.f, 644}, 38.f + added * 8.f);
    effects_.say("+" + std::to_string(added) + " muỗng", {737, 408}, ui::Theme{}.green);
    refreshButtons();
}
void PlayingState::serve(std::size_t index) {
    if (stage_ != Stage::Ready || index >= day_.customers().size())
        return;
    const auto customerName = game().data.customers[day_.customers()[index].kind].name;
    const auto receipt = day_.serve(index, bowl_);
    reviews_.push_back({customerName, receipt});
    effects_.say("+" + moneyText(receipt.base + receipt.tip), {737, 425}, {137, 105, 45}, 2.4f);
    effects_.burst({737, 487}, {228, 177, 66});
    sound("serve_order");
    if (receipt.stars() >= 4) happySounds_.push_back(.55f);
    std::ostringstream detail;
    detail << "Công thức " << static_cast<int>(receipt.recipe * 100) << "% • Xé "
           << static_cast<int>(receipt.paper * 100) << "% • Trộn " << static_cast<int>(receipt.mix * 100)
           << "% • Tip " << moneyText(receipt.tip);
    message_ = detail.str();
    game().save();
    resetBowl();
    selectedIndex();
}
bool PlayingState::handleEvent(const sf::Event &event, Pointer mouse) {
    pointer_ = mouse;
    if ((event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) ||
        event.type == sf::Event::LostFocus) {
        onCovered();
        stack_.push(StateID::Pause);
        return false;
    }
    if (day_.ended()) return false;
    if (event.type == sf::Event::MouseLeft) {
        cancelDrag();
        if (paper_)
            paper_->cancel();
        pointer_.reset();
    }
    if (event.type == sf::Event::MouseMoved && charge_.active() && (!mouse || !overBowl(*mouse))) {
        cancelCharge();
        message_ = "Đã hủy lượt thêm vì chuột rời khỏi tô.";
        refreshButtons();
    }
    if (event.type == sf::Event::MouseMoved && stage_ == Stage::Paper) {
        if (mouse) paper_->move(*mouse);
        else paper_->cancel();
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && mouse) {
        for (std::size_t i = 0; i < day_.customers().size(); ++i)
            if (customerArea(i).contains(*mouse)) {
                cancelCharge();
                selectedId_ = day_.customers()[i].id;
                return false;
            }
        if (stage_ == Stage::Paper)
            paper_->begin(*mouse);
        if (stage_ == Stage::Toppings && drops_.empty()) {
            for (std::size_t slot = 0; slot < 6; ++slot) {
                const auto index = page_ * 6 + slot;
                if (index < available_.size() && tray(slot).contains(*mouse)) {
                    cancelCharge();
                    selectedTopping_ = available_[index];
                    selectionAge_ = 0;
                    message_ = "Đã chọn " + game().data.toppings[*selectedTopping_].name +
                               ". Giữ chuột trên tô, xem thanh lực rồi thả để thêm.";
                    sound("topping_select");
                    return false;
                }
            }
            if (overBowl(*mouse)) {
                if (selectedTopping_ && static_cast<int>(bowl_.portions.size()) < day_.capacity()) {
                    charge_.begin();
                    if (context_.audio) context_.audio->charging(true, game().data.number("topping_charge_seconds"));
                    refreshButtons();
                } else message_ = selectedTopping_ ? "Tô đã đầy, hãy bấm Trộn món." : "Chọn một khay topping trước nhé!";
                return false;
            }
        }
        if (stage_ == Stage::Mixing && sf::FloatRect(557, 466, 360, 58).contains(*mouse)) {
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
                message_ = "Đã cho bánh tráng vừa xé vào tô. Chọn topping → giữ trên tô → thả để thêm.";
                refreshButtons();
            }
        }

        if (charge_.active()) {
            if (stage_ == Stage::Toppings && selectedTopping_ && mouse && overBowl(*mouse)) {
                const int count = charge_.release();
                if (context_.audio) context_.audio->charging(false);
                if (count > 0) addPortions(game().data.toppings[*selectedTopping_].id, count);
                else message_ = "Giữ lâu hơn một chút để lấy topping.";
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
    cookingFx_.update(dt);
    for (auto &delay : happySounds_) delay -= std::max(0.f, dt);
    while (!happySounds_.empty() && happySounds_.front() <= 0) {
        sound("customer_happy");
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
    effects_.update(dt);
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
            message_ = paper_->done() ? "Bánh đã xé nhỏ! Thả chuột để chuyển sang thêm topping."
                                      : "Đã xé thành dải! Tiếp tục giữ và rê để xé thành miếng nhỏ.";
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
        message_ = "Một khách đã rời quán. Hãy xem lại đơn của khách đang chọn.";
    }
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
            message_ = "Đã hủy lượt thêm vì chuột rời khỏi tô.";
        }
    }
    refreshButtons();
    UiState::update(dt);
    return false;
}
void PlayingState::drawCustomers(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const bool decorated = hasDecorations();
    if (day_.customers().empty()) {
        ui::text(target, font, "Khách đang đến… Chuẩn bị một phần bánh tráng nhé!", 20,
                 {640, 228}, theme.green, true);
        return;
    }
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
        const float mood = c.remaining / c.patience;
        const int extraRows = std::max(0, static_cast<int>((c.order.recipe.size() + 1) / 2) - 2);
        const float extraHeight = extraRows * (decorated ? 21.f : 25.f);
        const sf::FloatRect bubble{r.left + 9, r.top + 3, r.width - 18, (decorated ? 88.f : 115.f) + extraHeight};
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
            ui::text(target, font, t.name + " ×" + std::to_string(item.second), r.width < 280 ? 11 : r.width < 320 ? 12 : 14,
                     {x + 25, y + 2}, theme.ink);
            ++itemIndex;
        }
        ui::text(target, font, moneyText(c.order.price) + (selected ? "  •  Đang làm" : "  •  Bấm chọn"), 12,
                 {center, bubble.top + bubble.height - (decorated ? 10.f : 14.f)}, selected ? theme.green : theme.muted, true);
        // The original artwork is a bust: keep its aspect ratio and let the counter hide its lower edge.
        const std::string emotion = mood < .3f ? "angry" : mood > .7f ? "happy" : "normal";
        if (!ui::image(target, context_.resources, "customers/" + kind.style + "_" + emotion,
                       {center - 95, (decorated ? 237.f : 202.f) + extraHeight, 190, (decorated ? 125.f : 164.f) - extraHeight}))
            ui::customer(target, context_.resources, kind, {center, 290}, mood, decorated ? 1.9f : 2.5f);
        const std::string nameFrame = selected ? "customer_name" : "customer_name_unselected";
        if (!ui::frame(target, context_.resources, "ui/cozy_frames/" + nameFrame,
                       {center - 110, 337, 220, 28}, {54, 48}, {14, 13}))
            ui::panel(target, {center - 110, 337, 220, 28}, theme.paper, 12);
        ui::text(target, font, kind.name, 14, {center, 351}, theme.ink, true);
        if (kind.special) ui::star(target, {center - 89, 351}, 7, {210, 156, 48});
        ui::ProgressBar::draw(target, {center - 100, 361, 200, 6}, mood,
                              mood < .3f ? sf::Color(203, 108, 75) : theme.green);
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
        }
        const std::string artwork = t.type == "sauce" ? "tools/bottle_" + t.id : "toppings/" + t.id + "_tray";
        const float lift = chosen ? 7.f + 9.f * std::sin(std::min(selectionAge_ / .28f, 1.f) * 3.14159f) : 0.f;
        if (!ui::image(target, context_.resources, artwork, {r.left + 1, r.top - lift, r.width - 2, 106}))
            ui::ingredient(target, context_.resources, t, {r.left + r.width / 2, r.top + 48}, 2.3f);
        int required = 0;
        if (selected) {
            const auto &recipe = day_.customers()[*selected].order.recipe;
            const auto it = recipe.find(t.id);
            if (it != recipe.end())
                required = it->second;
        }
        ui::text(target, context_.resources.smallFont(),
                 t.name + "  " + std::to_string(bowl_.count(t.id)) + "/" + std::to_string(required), 14,
                 {r.left + r.width / 2, r.top + 111}, chosen || required > 0 ? theme.green : theme.ink, true);
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
        ui::image(target, context_.resources, artwork, paperBounds);
        ui::ProgressBar::draw(target, {625, 640, 224, 8}, paper_->progress(), theme.green);
        ui::panel(target, {736, 638, 2, 12}, theme.ink, 0);
        ui::text(target, font, "Giữ chuột và rê qua lại trên bánh • Thả để nghỉ", 14,
                 {737, 687}, theme.ink, true);
    } else if (stage_ == Stage::Mixing) {
        ui::bowl(target, context_.resources, game().data, bowl_, {737, 582}, .40f);
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
        ui::text(target, font,
                 "Nhịp " + std::to_string(mix_->hits()) + " / " +
                     std::to_string(game().data.integer("mix_hits")),
                 18, {737, 425}, theme.ink, true);
        ui::text(target, font, "Canh vùng xanh mới sau mỗi nhịp rồi bấm", 15, {737, 673}, theme.muted,
                 true);
    } else {
        if (!bowlDrag_)
            ui::bowl(target, context_.resources, game().data, bowl_, bowlCenter, bowlScale(),
                     charge_.active());
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
        }
        ui::text(target, font,
                 std::to_string(bowl_.portions.size()) + " / " + std::to_string(day_.capacity()) + " muỗng",
                 16, {737, 663}, theme.ink, true);
        ui::text(target, font,
                 stage_ == Stage::Ready ? "Kéo tô đến khách hoặc bấm Giao món"
                                        : "Chọn khay → Giữ trên tô → Thả (1–3 muỗng)",
                 13, {737, 687}, theme.muted, true);
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
        ui::text(target, font, std::to_string(i + 1) + ". " + steps[i], 17,
                 {1016, 418 + static_cast<float>(i) * 27}, current ? theme.green : theme.muted);
    }
    if (stage_ != Stage::Paper)
        ui::text(target, font,
                 "Chất lượng xé: " + std::to_string(static_cast<int>(bowl_.paperQuality * 100)) + "%", 15,
                 {1016, 521}, theme.muted);
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
        ui::image(target, context_.resources, "purchased_decor/decor/lantern", {43, 113, 60, 110});
        ui::image(target, context_.resources, "purchased_decor/decor/lantern", {1177, 113, 60, 110});
    }
    if (!ui::frame(target, context_.resources, "ui/cozy_frames/top_panel",
                   {12, 8, 1256, 76}, {80, 48}, {40, 24}))
        ui::panel(target, {12, 8, 1256, 76}, theme.paper, 16);
    ui::text(target, context_.resources.titleFont(), "Ngày " + std::to_string(game().progress.day),
             26, {640, 38}, theme.ink, true);
    const int time = static_cast<int>(std::ceil(day_.remaining()));
    ui::image(target, context_.resources, "ui/hourglass_icon", {65, 25, 26, 30});
    ui::text(target, context_.resources.smallFont(),
             std::to_string(time / 60) + ":" + (time % 60 < 10 ? "0" : "") + std::to_string(time % 60),
             25, {108, 26}, theme.ink);
    ui::ProgressBar::draw(target, {260, 65, 760, 5}, day_.remaining() / game().data.number("day_seconds"), theme.green);
    drawCustomers(target);
    // The new scene supplies the wooden worktop; retain a fallback for missing art.
    if (!scene) {
        ui::panel(target, {15, 366, 1250, 291}, {193, 149, 105}, 14);
        ui::panel(target, {15, 366, 1250, 8}, {151, 107, 72}, 3);
    }
    ui::image(target, context_.resources, "ui/coin_icon", {29, 372, 24, 24});
    ui::text(target, context_.resources.smallFont(), moneyText(game().progress.money), 19,
             {61, 373}, {255, 248, 220});
    drawWorktop(target);
    renderButtons(target);
    if (!game().saves.error().empty())
        ui::text(target, font, game().saves.error(), 13, {30, 710}, {171, 65, 43});
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
        ui::bowl(target, context_.resources, game().data, bowl_, *pointer_, .5f, true);
    cookingFx_.render(target);
    effects_.render(target, font);
    if (!reviews_.empty()) {
        const auto &review = reviews_.front();
        const auto alpha = static_cast<sf::Uint8>(255.f * std::min(1.f, std::max(0.f, review.remaining) / .6f));
        const auto outlined = [&](const std::string &label, unsigned size, float y) {
            sf::Text value(utf8(label), context_.resources.titleFont(), size);
            const auto bounds = value.getLocalBounds();
            value.setOrigin(bounds.left + bounds.width / 2, bounds.top + bounds.height / 2);
            value.setPosition(640, y);
            value.setFillColor({255, 244, 207, alpha});
            value.setOutlineColor({94, 53, 34, alpha});
            value.setOutlineThickness(2.f);
            target.draw(value);
        };
        outlined(review.customer + ": " + review.receipt.review(), 29, 341);
        outlined(std::to_string(static_cast<int>(std::lround(review.receipt.score * 100))) + "/100  •  " +
                     std::to_string(review.receipt.stars()) + "/5 sao", 20, 377);
        for (int i = 0; i < 5; ++i) {
            const auto *texture = context_.resources.texture(i < review.receipt.stars()
                ? "assets/images/cozy/ui/star_full.png" : "assets/images/cozy/ui/star_empty.png");
            if (texture) {
                const auto size = texture->getSize();
                sf::Sprite star(*texture);
                star.setScale(30.f / size.x, 30.f / size.y);
                star.setPosition(557.f + i * 34.f, 395.f);
                star.setColor({255, 255, 255, alpha});
                target.draw(star);
            }
        }
    }
}
} // namespace tiem

#include "states/PlayingState.hpp"
#include "ui/Art.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace tiem {
PlayingState::PlayingState(StateStack &stack, AppContext &context)
    : UiState(stack, context),
      day_(game().data, game().progress,
           static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count())) {
    if (context_.audio)
        context_.audio->music("cozy_shop");
    for (std::size_t i = 0; i < game().data.toppings.size(); ++i)
        if (game().progress.unlocked.count(game().data.toppings[i].id))
            available_.push_back(i);
    addButton("Tạm dừng", {1085, 29, 162, 44}, [this] { stack_.push(StateID::Pause); });
    addButton("Trước", {40, 591, 195, 38}, [this] {
        if (page_ > 0)
            --page_;
        refreshButtons();
    });
    addButton("Sau", {255, 591, 195, 38}, [this] {
        if ((page_ + 1) * 6 < available_.size())
            ++page_;
        refreshButtons();
    });
    addButton("Xé theo vạch", {1010, 513, 227, 49}, [this] {
        if (stage_ == Stage::Toppings)
            startMix();
        else if (stage_ == Stage::Mixing)
            hitMix();
        else if (stage_ == Stage::Ready) {
            if (const auto i = selectedIndex())
                serve(*i);
        }
    });
    addButton("Làm lại tô", {1010, 580, 227, 43}, [this] {
        resetBowl();
        message_ = "Tô mới đã sẵn sàng. Xé bánh tráng trước nhé!";
    });
    resetBowl();
    refreshButtons();
}
void PlayingState::resetBowl() {
    bowl_.clear();
    stage_ = Stage::Paper;
    cancelDrag();
    paper_ = std::make_unique<PaperTearing>(game().data,
                                            UpgradeSystem::effect(game().data, game().progress, "paper"));
    mix_.reset();
    refreshButtons();
}
sf::FloatRect PlayingState::tray(std::size_t slot) const {
    return {40.f + static_cast<float>(slot % 3) * 142, 315.f + static_cast<float>(slot / 3) * 132, 130, 120};
}
sf::FloatRect PlayingState::customerCard(std::size_t index) const {
    const int slots =
        std::min(game().data.integer("queue_max"),
                 game().data.integer("queue_slots") +
                     static_cast<int>(UpgradeSystem::effect(game().data, game().progress, "queue")));
    const float width = (1224.f - static_cast<float>(slots - 1) * 12) / static_cast<float>(slots);
    return {28.f + static_cast<float>(index) * (width + 12), 105, width, 140};
}
bool PlayingState::overBowl(sf::Vector2f p) const {
    const float x = (p.x - 737) / 120, y = (p.y - 437) / 94;
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
    buttons_[1].setEnabled(page_ > 0 && !dragging_);
    buttons_[2].setEnabled((page_ + 1) * 6 < available_.size() && !dragging_);
    const char *name = stage_ == Stage::Paper      ? "Xé theo vạch"
                       : stage_ == Stage::Toppings ? "Trộn món"
                       : stage_ == Stage::Mixing   ? "Đánh nhịp"
                                                   : "Giao món";
    buttons_[3].setLabel(name);
    buttons_[3].setEnabled(stage_ != Stage::Paper && (stage_ != Stage::Toppings || !bowl_.portions.empty()) &&
                           (stage_ != Stage::Ready || !day_.customers().empty()));
}
void PlayingState::cancelDrag() {
    dragging_.reset();
    bowlDrag_ = false;
    pourTime_ = 0;
    poured_ = false;
}
void PlayingState::onCovered() {
    UiState::onCovered();
    cancelDrag();
    pointer_.reset();
    if (paper_)
        paper_->cancel();
}
void PlayingState::startMix() {
    if (stage_ != Stage::Toppings || bowl_.portions.empty())
        return;
    cancelDrag();
    stage_ = Stage::Mixing;
    mix_ = std::make_unique<MixingMinigame>(
        game().data, UpgradeSystem::effect(game().data, game().progress, "mix"), game().progress.day);
    message_ = "Bấm khi vạch chạy vào vùng xanh. Cần đủ " + std::to_string(game().data.integer("mix_hits")) +
               " nhịp.";
    refreshButtons();
}
void PlayingState::hitMix() {
    if (!mix_)
        return;
    const auto value = mix_->hit();
    if (!value)
        return;
    effects_.say(*value > .9f   ? "Perfect!"
                 : *value > .5f ? "Good!"
                                : "Miss!",
                 {737, 390}, *value > .5f ? sf::Color(60, 118, 70) : sf::Color(182, 90, 56),
                 game().data.number("feedback_seconds"));
    sound(*value > .9f ? "perfect" : "mix");
    if (mix_->done()) {
        bowl_.mixQuality = mix_->score();
        bowl_.mixed = true;
        stage_ = Stage::Ready;
        message_ = "Chọn khách rồi Giao món, hoặc kéo tô đến khách.";
    }
    refreshButtons();
}
void PlayingState::addPortion(const std::string &id) {
    if (!bowl_.add(id, day_.capacity())) {
        message_ = "Tô đã đầy. Hãy trộn hoặc làm lại.";
        return;
    }
    const auto &t = game().data.topping(id);
    message_ = "Đã thêm " + t.name + ". Kiểm tra số muỗng trên đơn nhé.";
    effects_.burst({737, 437}, t.color);
    sound(t.type == "sauce" ? "sauce_pour" : "topping_drop");
    refreshButtons();
}
void PlayingState::serve(std::size_t index) {
    if (stage_ != Stage::Ready || index >= day_.customers().size())
        return;
    const auto receipt = day_.serve(index, bowl_);
    const auto card = customerCard(index);
    effects_.say(receipt.score >= .85f  ? "Ngon tuyệt!"
                 : receipt.score >= .6f ? "Cảm ơn nhé!"
                                        : "Chưa đúng vị...",
                 {card.left + card.width / 2, 170}, {67, 110, 68}, 2.4f);
    effects_.say("+" + moneyText(receipt.base + receipt.tip), {737, 365}, {137, 105, 45}, 2.4f);
    effects_.burst({737, 427}, {228, 177, 66});
    sound("coin_receive");
    sound(receipt.score >= .6f ? "customer_happy" : "customer_leave");
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
    if (event.type == sf::Event::MouseLeft) {
        cancelDrag();
        if (paper_)
            paper_->cancel();
        pointer_.reset();
    }
    if (event.type == sf::Event::MouseMoved && mouse && stage_ == Stage::Paper)
        paper_->move(*mouse);
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left && mouse) {
        for (std::size_t i = 0; i < day_.customers().size(); ++i)
            if (customerCard(i).contains(*mouse)) {
                selectedId_ = day_.customers()[i].id;
                return false;
            }
        if (stage_ == Stage::Paper)
            paper_->begin(*mouse);
        if (stage_ == Stage::Toppings) {
            for (std::size_t slot = 0; slot < 6; ++slot) {
                const auto index = page_ * 6 + slot;
                if (index < available_.size() && tray(slot).contains(*mouse)) {
                    dragging_ = available_[index];
                    poured_ = false;
                    pourTime_ = 0;
                    return false;
                }
            }
            if (overBowl(*mouse) && !bowl_.portions.empty()) {
                startMix();
                return false;
            }
        }
        if (stage_ == Stage::Mixing && sf::FloatRect(557, 406, 360, 58).contains(*mouse)) {
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
                sound("paper_tear");
                effects_.say(*quality > .85f  ? "Perfect!"
                             : *quality > .6f ? "Good!"
                                              : "Ok!",
                             {737, 360}, {67, 118, 66}, game().data.number("feedback_seconds"));
                effects_.burst({737, 437}, {233, 189, 120});
                if (paper_->done()) {
                    bowl_.paperQuality = paper_->quality();
                    stage_ = Stage::Toppings;
                    message_ = "Kéo topping vào tô theo đơn. Sốt: giữ chai trên tô để rưới.";
                    refreshButtons();
                }
            }
        }
        if (dragging_) {
            const auto &t = game().data.toppings[*dragging_];
            if (t.type != "sauce" && mouse && overBowl(*mouse))
                addPortion(t.id);
            else if (t.type == "sauce" && !poured_)
                message_ = "Giữ chai trên tô cho đủ một muỗng rồi thả chuột.";
            else if (!poured_)
                message_ = "Đã hủy lượt kéo ngoài tô.";
            cancelDrag();
            return false;
        }
        if (bowlDrag_) {
            bowlDrag_ = false;
            if (mouse)
                for (std::size_t i = 0; i < day_.customers().size(); ++i)
                    if (customerCard(i).contains(*mouse)) {
                        serve(i);
                        break;
                    }
            return false;
        }
    }
    if (!dragging_ && !bowlDrag_)
        handleButtons(event, mouse);
    return false;
}
bool PlayingState::update(float dt) {
    animation_ += dt;
    effects_.update(dt);
    if (paper_)
        paper_->update(dt);
    const int oldLeft = day_.result().left;
    day_.update(dt);
    if (day_.result().left > oldLeft && !day_.ended()) {
        sound("customer_leave");
        message_ = "Một khách đã rời quán. Hãy xem lại đơn của khách đang chọn.";
    }
    if (day_.ended() && !finished_) {
        finished_ = true;
        game().lastDay = day_.result();
        game().hasResult = true;
        game().save();
        sound("day_end");
        stack_.replace(StateID::DayResult);
        return false;
    }
    selectedIndex();
    if (stage_ == Stage::Mixing)
        mix_->update(dt);
    if (dragging_ && stage_ == Stage::Toppings && game().data.toppings[*dragging_].type == "sauce") {
        if (pointer_ && overBowl(*pointer_)) {
            pourTime_ += dt;
            if (pourTime_ >= game().data.number("sauce_seconds_per_spoon")) {
                pourTime_ = 0;
                addPortion(game().data.toppings[*dragging_].id);
                poured_ = true;
            }
        } else
            pourTime_ = 0;
    }
    refreshButtons();
    UiState::update(dt);
    return false;
}
void PlayingState::drawCustomers(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    if (day_.customers().empty())
        ui::text(target, font, "Khách đang trên đường đến… Bạn có thể chuẩn bị bánh tráng trước.", 22,
                 {640, 169}, theme.green, true);
    for (std::size_t i = 0; i < day_.customers().size(); ++i) {
        const auto &c = day_.customers()[i];
        auto r = customerCard(i);
        r.top += (1 - ui::easeOut(c.arrival)) * 12;
        const bool selected = c.id == selectedId_;
        ui::panel(target, {r.left - 3, r.top - 3, r.width + 6, r.height + 6},
                  selected ? theme.green : sf::Color(207, 216, 187), 16);
        ui::panel(target, r, theme.paper, 14);
        ui::customer(target, game().data.customers[c.kind], {r.left + 34, r.top + 33},
                     c.remaining / c.patience, .7f);
        ui::text(target, font, game().data.customers[c.kind].name, 18, {r.left + 66, r.top + 10}, theme.ink);
        ui::text(target, font, moneyText(c.order.price), 14, {r.left + 66, r.top + 34}, theme.muted);
        int row = 0;
        for (const auto &item : c.order.recipe) {
            const auto &t = game().data.topping(item.first);
            const int col = row % 2, line = row / 2;
            const float x = r.left + 16 + static_cast<float>(col) * (r.width - 24) / 2,
                        y = r.top + 74 + static_cast<float>(line) * 25;
            ui::ingredient(target, t, {x + 8, y}, .55f);
            ui::text(target, font, t.name + " ×" + std::to_string(item.second), r.width < 320 ? 12 : 14,
                     {x + 22, y - 8}, theme.ink);
            ++row;
        }
        ui::ProgressBar::draw(target, {r.left + 13, r.top + 122, r.width - 26, 8}, c.remaining / c.patience,
                              c.remaining / c.patience < .3f ? sf::Color(203, 108, 75) : theme.green);
    }
}
void PlayingState::drawWorktop(sf::RenderTarget &target) {
    const auto &font = context_.resources.uiFont();
    const ui::Theme theme;
    ui::panel(target, {27, 281, 440, 359}, {251, 241, 218}, 20);
    ui::panel(target, {486, 281, 500, 359}, {247, 233, 206}, 20);
    ui::panel(target, {998, 281, 252, 359}, {255, 249, 233}, 20);
    ui::text(target, font, "KHAY NGUYÊN LIỆU", 16, {43, 294}, theme.muted);
    const auto selected = selectedIndex();
    for (std::size_t slot = 0; slot < 6; ++slot) {
        const auto n = page_ * 6 + slot;
        if (n >= available_.size())
            break;
        const auto &t = game().data.toppings[available_[n]];
        const auto r = tray(slot);
        ui::panel(target, r, theme.paper, 12);
        sf::Color fill = t.color;
        fill.a = 85;
        ui::panel(target, {r.left + 8, r.top + 8, r.width - 16, 58}, fill, 10);
        if (t.type == "sauce") {
            ui::panel(target, {r.left + 49, r.top + 19, 30, 42}, t.color, 7);
            ui::panel(target, {r.left + 55, r.top + 9, 18, 13}, theme.ink, 3);
        } else
            for (int k = 0; k < 3; ++k)
                ui::ingredient(target, t, {r.left + 39 + static_cast<float>(k) * 25, r.top + 38}, .9f,
                               static_cast<float>(k) * 35);
        ui::text(target, font, t.name, 16, {r.left + r.width / 2, r.top + 80}, theme.ink, true);
        int required = 0;
        if (selected) {
            const auto &recipe = day_.customers()[*selected].order.recipe;
            const auto it = recipe.find(t.id);
            if (it != recipe.end())
                required = it->second;
        }
        ui::text(target, font,
                 std::to_string(bowl_.count(t.id)) + " / " + std::to_string(required) + " muỗng", 14,
                 {r.left + r.width / 2, r.top + 105}, required > 0 ? theme.green : theme.muted, true);
    }
    ui::text(target, font, "Trong tô / Đơn đang chọn", 13, {247, 579}, theme.muted, true);
    const char *title = stage_ == Stage::Paper      ? "1. XÉ BÁNH TRÁNG"
                        : stage_ == Stage::Toppings ? "2. THÊM TOPPING"
                        : stage_ == Stage::Mixing   ? "3. TRỘN ĐỀU"
                                                    : "4. GIAO MÓN";
    ui::text(target, font, title, 19, {737, 310}, theme.ink, true);
    if (stage_ == Stage::Paper) {
        ui::panel(target, {588, 345, 296, 192}, {179, 130, 83}, 17);
        ui::panel(target, {600, 351, 272, 177}, {245, 218, 167}, 10);
        for (int i = 0; i < 3; ++i) {
            const float y = PaperTearing::lineY(i);
            if (paper_->cut(i)) {
                ui::panel(target, {619, y - 8, 236, 3}, {179, 130, 83}, 0);
                ui::text(target, font, "Đã xé", 13, {737, y + 7}, theme.green, true);
            } else {
                for (int k = 0; k < 13; ++k)
                    ui::panel(target, {640 + static_cast<float>(k) * 16, y, 9, 3}, theme.ink, 0);
                sf::CircleShape point(7);
                point.setOrigin(7, 7);
                point.setPosition(640, y);
                point.setFillColor(theme.green);
                target.draw(point);
                point.setPosition(840, y);
                target.draw(point);
            }
        }
        ui::text(target, font, "Giữ ở chấm xanh → vuốt hết vạch → thả", 16, {737, 563}, theme.muted, true);
    } else if (stage_ == Stage::Mixing) {
        ui::bowl(target, game().data, bowl_, {737, 528}, .52f);
        const float width = 360;
        ui::panel(target, {557, 411, width, 42}, {214, 184, 145}, 9);
        ui::panel(target, {557 + width * (.5f - mix_->width() / 2), 411, width * mix_->width(), 42},
                  {124, 170, 110}, 5);
        ui::panel(target, {557 + width * (.5f - mix_->width() * .2f), 411, width * mix_->width() * .4f, 42},
                  {178, 204, 133}, 4);
        ui::panel(target, {557 + width * mix_->position() - 3, 402, 6, 60}, theme.ink, 3);
        ui::text(target, font,
                 "Nhịp " + std::to_string(mix_->hits()) + " / " +
                     std::to_string(game().data.integer("mix_hits")),
                 18, {737, 365}, theme.ink, true);
        ui::text(target, font, "Bấm thanh hoặc nút Đánh nhịp khi vạch vào xanh", 15, {737, 602}, theme.muted,
                 true);
    } else {
        if (!bowlDrag_)
            ui::bowl(target, game().data, bowl_, {737, 437}, 1.f,
                     dragging_ && pointer_ && overBowl(*pointer_));
        ui::text(target, font,
                 std::to_string(bowl_.portions.size()) + " / " + std::to_string(day_.capacity()) + " muỗng",
                 18, {737, 566}, theme.ink, true);
        ui::text(target, font,
                 stage_ == Stage::Ready ? "Kéo tô đến khách hoặc bấm Giao món"
                                        : "Thêm đủ topping rồi bấm Trộn món",
                 15, {737, 604}, theme.muted, true);
    }
    ui::text(target, font, "MÓN CỦA BẠN", 16, {1125, 312}, theme.muted, true);
    const char *steps[] = {"Xé bánh tráng", "Thêm topping", "Trộn đều", "Giao cho khách"};
    for (int i = 0; i < 4; ++i) {
        const bool current = static_cast<int>(stage_) == i;
        ui::text(target, font, std::to_string(i + 1) + ". " + steps[i], 17,
                 {1016, 349 + static_cast<float>(i) * 30}, current ? theme.green : theme.muted);
    }
    if (stage_ != Stage::Paper)
        ui::text(target, font,
                 "Chất lượng xé: " + std::to_string(static_cast<int>(bowl_.paperQuality * 100)) + "%", 15,
                 {1016, 479}, theme.muted);
}
void PlayingState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    ui::shopBackground(target, game().data, game().progress, animation_);
    ui::text(target, font, "Bánh tráng trộn tiệm", 27, {30, 29}, theme.ink);
    ui::text(target, font, "Ngày " + std::to_string(game().progress.day), 18, {391, 30}, theme.green);
    const int time = static_cast<int>(std::ceil(day_.remaining()));
    ui::text(target, font,
             std::to_string(time / 60) + ":" + (time % 60 < 10 ? "0" : "") + std::to_string(time % 60), 25,
             {566, 26}, theme.ink);
    ui::text(target, font, moneyText(game().progress.money), 24, {762, 27}, theme.ink);
    ui::text(target, font, "Chọn một khách để xem lượng topping cần làm", 14, {30, 69}, theme.muted);
    ui::ProgressBar::draw(target, {390, 66, 650, 6}, day_.remaining() / game().data.number("day_seconds"),
                          theme.green);
    drawCustomers(target);
    drawWorktop(target);
    renderButtons(target);
    ui::text(target, font, message_, 16, {32, 661}, theme.ink);
    ui::text(target, font, "ESC: tạm dừng  •  Thả ngoài: hủy kéo  •  Hết ngày tự lưu", 13, {32, 694},
             theme.muted);
    if (!game().saves.error().empty())
        ui::text(target, font, game().saves.error(), 13, {590, 694}, {171, 65, 43});
    if (dragging_ && pointer_) {
        const auto &topping = game().data.toppings[*dragging_];
        if (topping.type == "sauce") {
            sf::RectangleShape bottle({24, 48});
            bottle.setOrigin(12, 24);
            bottle.setPosition(*pointer_);
            bottle.setRotation(overBowl(*pointer_) ? -60.f : 0.f);
            bottle.setFillColor(topping.color);
            target.draw(bottle);
            ui::ProgressBar::draw(target, {pointer_->x - 25, pointer_->y + 33, 50, 6},
                                  pourTime_ / game().data.number("sauce_seconds_per_spoon"), theme.green);
        } else
            ui::ingredient(target, topping, *pointer_, 1.4f);
        ui::Tooltip::draw(target, font, topping.name, pointer_.value() + sf::Vector2f(23, -42));
    }
    if (bowlDrag_ && pointer_)
        ui::bowl(target, game().data, bowl_, *pointer_, .5f, true);
    effects_.render(target, font);
}
} // namespace tiem

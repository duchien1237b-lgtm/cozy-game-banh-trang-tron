#include "ui/Button.hpp"
#include "ui/Draw.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace tiem::ui {
Button::Button(const sf::Font &font, std::string label, sf::FloatRect bounds, Callback onClick)
    : font_(font), label_(std::move(label)), bounds_(bounds), onClick_(std::move(onClick)) {}
void Button::setEnabled(bool enabled) {
    if (enabled_ != enabled) {
        enabled_ = enabled;
        cancelInteraction();
    }
}
void Button::cancelInteraction() {
    hovered_ = false;
    pressed_ = false;
}

void Button::handleEvent(const sf::Event &event, Pointer mouse) {
    if (event.type == sf::Event::LostFocus || event.type == sf::Event::MouseLeft) {
        cancelInteraction();
        return;
    }
    if (!enabled_)
        return;
    const bool inside = mouse && bounds_.contains(*mouse);
    if (event.type == sf::Event::MouseMoved) {
        hovered_ = inside;
        if (!inside)
            pressed_ = false;
    } else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        hovered_ = inside;
        pressed_ = inside;
    } else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        const bool activate = pressed_ && inside;
        pressed_ = false;
        hovered_ = inside;
        // Chỉ nhận click nếu nhấn rồi thả trong nút; kéo từ ngoài vào không tính.
        if (activate) {
            if (clickSound_)
                clickSound_();
            if (onClick_)
                onClick_();
        }
    }
}

void Button::update(float dt) {
    const auto goal = enabled_ && hovered_ && !pressed_ ? -3.f : 0.f;
    lift_ += (goal - lift_) * (1.f - std::exp(-16.f * std::max(0.f, dt)));
}
void Button::render(sf::RenderTarget &target) const {
    const Theme theme;
    auto rect = bounds_;
    if (!iconPath_.empty()) {
        rect.top += pressed_ ? 3.f : lift_;
        const float side = std::min(rect.width, rect.height - 22.f);
        const sf::Vector2f center{rect.left + rect.width * .5f, rect.top + side * .5f};
        const auto *icon = resources_ ? resources_->texture(iconPath_) : nullptr;
        if (icon) {
            const auto size = icon->getSize();
            const float scale = (side - 10.f) / static_cast<float>(std::max(size.x, size.y));
            sf::Sprite sprite(*icon);
            sprite.setOrigin(size.x * .5f, size.y * .5f);
            sprite.setScale(scale, scale);
            sprite.setPosition(center);
            sprite.setColor({255, 255, 255, static_cast<sf::Uint8>(enabled_ ? 255 : 130)});
            target.draw(sprite);
        }
        // The caption is inside the hit area and remains legible over the street background.
        panel(target, {rect.left, rect.top + side + 3, rect.width, 20}, {255, 250, 229, 235}, 9);
        text(target, font_, label_, 13, {center.x, rect.top + side + 13},
             enabled_ ? theme.ink : theme.muted, true);
        return;
    }
    panel(target, {rect.left, rect.top + 5.f, rect.width, rect.height}, {199, 177, 147}, 12.f);
    rect.top += pressed_ ? 3.f : lift_;
    auto color = theme.green;
    if (!enabled_)
        color = {205, 202, 186};
    else if (pressed_)
        color = {57, 85, 55};
    else if (hovered_)
        color = {99, 132, 91};
    const std::string state = !enabled_ ? "disabled" : pressed_ ? "pressed" : hovered_ ? "hover" : "normal";
    const auto *texture = resources_ ? resources_->texture("assets/images/cozy/ui/button_" + state + ".png") : nullptr;
    if (texture) {
        sf::Sprite sprite(*texture);
        // The supplied button PNGs share a 256x64 canvas with transparent padding.
        // Crop to their common painted bounds so the art matches the hit rectangle.
        sprite.setTextureRect({48, 11, 160, 39});
        sprite.setPosition(rect.left, rect.top);
        sprite.setScale(rect.width / 160.f, rect.height / 39.f);
        target.draw(sprite);
    } else panel(target, rect, color, 12.f);
    text(target, font_, label_, 19, {rect.left + rect.width / 2.f, rect.top + rect.height / 2.f},
         enabled_ ? (texture ? theme.ink : theme.paper) : theme.muted, true);
}
} // namespace tiem::ui

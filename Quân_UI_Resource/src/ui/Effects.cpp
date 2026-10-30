#include "ui/Effects.hpp"
#include "core/Utf8.hpp"
#include <algorithm>
#include <cmath>
namespace tiem::ui {
void ProgressBar::draw(sf::RenderTarget &target, sf::FloatRect r, float ratio, sf::Color color) {
    panel(target, r, {224, 217, 197}, r.height / 2);
    r.width *= std::clamp(ratio, 0.f, 1.f);
    if (r.width > 0)
        panel(target, r, color, r.height / 2);
}
void Effects::say(std::string text, sf::Vector2f position, sf::Color color, float life, unsigned size, bool outlined) {
    // Giữ phản hồi mới nhất tại cùng một vị trí, tránh chữ chồng khi xé liên tiếp.
    texts_.erase(std::remove_if(texts_.begin(), texts_.end(), [&](const auto& item) {
        return std::abs(item.position.x - position.x) < 20 && std::abs(item.position.y - position.y) < 20;
    }), texts_.end());
    texts_.push_back({std::move(text), position, color, 0, life, size, outlined});
}
void Effects::burst(sf::Vector2f p, sf::Color color, int count) {
    for (int i = 0; i < count; ++i) {
        float a = static_cast<float>(i) * 6.2831853f / static_cast<float>(std::max(1, count));
        particles_.push_back({p, {std::cos(a) * 95, std::sin(a) * 70 - 50}, color, 0, .65f});
    }
}
void Effects::sparkle(sf::Vector2f p, sf::Color color, int count) {
    for (int i = 0; i < count; ++i) {
        const float a = static_cast<float>(i) * 2.39996f;
        const float speed = 60.f + static_cast<float>((i * 37) % 90);
        particles_.push_back({p, {std::cos(a) * speed, std::sin(a) * speed - 90.f}, color, 0, .9f + static_cast<float>(i % 5) * .08f});
    }
}
void Effects::update(float dt) {
    for (auto &t : texts_)
        t.age += dt;
    for (auto &p : particles_) {
        p.age += dt;
        p.position += p.velocity * dt;
        p.velocity.y += 160 * dt;
    }
    texts_.erase(std::remove_if(texts_.begin(), texts_.end(), [](const auto &t) { return t.age >= t.life; }),
                 texts_.end());
    particles_.erase(
        std::remove_if(particles_.begin(), particles_.end(), [](const auto &p) { return p.age >= p.life; }),
        particles_.end());
}
void Effects::render(sf::RenderTarget &target, const sf::Font &font) const {
    for (const auto &p : particles_) {
        sf::CircleShape s(3);
        s.setOrigin(3, 3);
        s.setPosition(p.position);
        auto c = p.color;
        c.a = static_cast<sf::Uint8>(255 * (1 - p.age / p.life));
        s.setFillColor(c);
        target.draw(s);
    }
    for (const auto &t : texts_) {
        auto c = t.color;
        c.a = static_cast<sf::Uint8>(255 * (1 - t.age / t.life));
        const auto position = t.position + sf::Vector2f(0, -35 * easeOut(t.age / t.life));
        if (t.outlined) {
            auto outline = sf::Color(94, 53, 34, c.a);
            outlinedText(target, font, t.text, t.size, position, c, outline);
        } else text(target, font, t.text, t.size, position, c, true);
    }
}
void Tooltip::draw(sf::RenderTarget &target, const sf::Font &font, const std::string &message,
                   sf::Vector2f p) {
    p.x = std::clamp(p.x, 10.f, 1000.f);
    p.y = std::clamp(p.y, 20.f, 670.f);
    panel(target, {p.x, p.y, 260, 36}, {255, 252, 236}, 8);
    text(target, font, message, 15, p + sf::Vector2f(10, 9), {81, 54, 44});
}
} // namespace tiem::ui

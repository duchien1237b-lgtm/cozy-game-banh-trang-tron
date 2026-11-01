#include "ui/CookingEffects.hpp"
#include <algorithm>
#include <cmath>

namespace tiem::ui {
CookingEffects::CookingEffects(ResourceManager &resources)
    : atlas_(resources.texture("assets/images/cozy/effects/cooking_fx.png")) {
    if (!atlas_) return;
    const auto pixels = atlas_->copyToImage();
    const auto size = pixels.getSize();
    for (int i = 0; i < 6; ++i) {
        const int x0 = (i % 3) * size.x / 3, x1 = (i % 3 + 1) * size.x / 3;
        const int y0 = (i / 3) * size.y / 2, y1 = (i / 3 + 1) * size.y / 2;
        int left = x1, top = y1, right = x0, bottom = y0;
        for (int y = y0; y < y1; ++y)
            for (int x = x0; x < x1; ++x)
                if (pixels.getPixel(x, y).a > 16) {
                    left = std::min(left, x); right = std::max(right, x + 1);
                    top = std::min(top, y); bottom = std::max(bottom, y + 1);
                }
        cells_[i] = right > left && bottom > top ? sf::IntRect(left, top, right - left, bottom - top)
                                                  : sf::IntRect(x0, y0, x1 - x0, y1 - y0);
    }
}
void CookingEffects::clear() { motifs_.clear(); }
void CookingEffects::add(Motif motif) {
    // Bound work even when pointer events arrive at a very high rate.
    if (motifs_.size() >= 96) motifs_.erase(motifs_.begin());
    motifs_.push_back(motif);
}
void CookingEffects::tear(sf::Vector2f p, bool complete) {
    const int count = complete ? 10 : 2;
    for (int i = 0; i < count; ++i) {
        const unsigned n = sequence_++;
        const float angle = (n * 137 % 360) * 3.14159265f / 180.f;
        const float speed = complete ? 115.f : 52.f;
        add({Shard, p, {std::cos(angle) * speed, std::sin(angle) * speed - 65.f},
             12.f + (n % 4) * 3.f, 0, .55f + (n % 3) * .06f,
             static_cast<float>(n * 53 % 360), n % 2 ? 150.f : -150.f});
    }
    pulse(Tear, p, complete ? 92.f : 44.f, .26f);
}
void CookingEffects::pulse(Model model, sf::Vector2f p, float size, float life) {
    add({model, p, {}, size, 0, life, 0, model == Good ? 110.f : 0.f});
}
void CookingEffects::update(float dt) {
    dt = std::max(0.f, dt);
    for (auto &m : motifs_) {
        m.age += dt;
        m.position += m.velocity * dt;
        if (m.model == Shard) m.velocity.y += 180.f * dt;
        m.angle += m.spin * dt;
    }
    motifs_.erase(std::remove_if(motifs_.begin(), motifs_.end(),
        [](const auto &m) { return m.age >= m.life; }), motifs_.end());
}
void CookingEffects::sprite(sf::RenderTarget &target, Model model, sf::Vector2f p,
                            float size, float rotation, float opacity) const {
    if (!atlas_) return;
    const auto &cell = cells_[model];
    sf::Sprite s(*atlas_, cell);
    s.setOrigin(cell.width * .5f, cell.height * .5f);
    const float scale = size / std::max(cell.width, cell.height);
    s.setScale(scale, scale);
    s.setPosition(p);
    s.setRotation(rotation);
    s.setColor({255, 255, 255, static_cast<sf::Uint8>(255 * std::clamp(opacity, 0.f, 1.f))});
    target.draw(s);
}
void CookingEffects::render(sf::RenderTarget &target) const {
    for (const auto &m : motifs_) {
        const float t = std::clamp(m.age / m.life, 0.f, 1.f);
        const float scale = m.model == Shard ? 1.f : .6f + .6f * std::sin(t * 1.5707963f);
        sprite(target, m.model, m.position, m.size * scale, m.angle,
               std::min(1.f, (1.f - t) * 2.5f));
    }
}
} // namespace tiem::ui

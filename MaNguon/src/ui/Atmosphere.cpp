#include "ui/Atmosphere.hpp"
#include "core/Utf8.hpp"
#include <algorithm>
#include <cmath>
namespace tiem::ui {
namespace {
constexpr float pi = 3.14159265f;
sf::Color alpha(sf::Color c, float a) {
    c.a = static_cast<sf::Uint8>(std::clamp(a, 0.f, 1.f) * static_cast<float>(c.a));
    return c;
}
void circle(sf::RenderTarget &target, sf::Vector2f center, float radius, sf::Color color) {
    sf::CircleShape c(radius, 28);
    c.setOrigin(radius, radius);
    c.setPosition(center);
    c.setFillColor(color);
    target.draw(c);
}
void cloudShape(sf::RenderTarget &target, sf::Vector2f c, float s, sf::Color fill) {
    circle(target, c + sf::Vector2f(-s * .28f, s * .06f), s * .26f, fill);
    circle(target, c + sf::Vector2f(s * .02f, -s * .1f), s * .34f, fill);
    circle(target, c + sf::Vector2f(s * .32f, s * .08f), s * .24f, fill);
    panel(target, {c.x - s * .5f, c.y + s * .02f, s * 1.02f, s * .3f}, fill, s * .15f);
}
void drop(sf::RenderTarget &target, sf::Vector2f tip, float size, sf::Color color) {
    sf::ConvexShape shape(3);
    shape.setPoint(0, tip);
    shape.setPoint(1, tip + sf::Vector2f(-size * .55f, size * 1.1f));
    shape.setPoint(2, tip + sf::Vector2f(size * .55f, size * 1.1f));
    shape.setFillColor(color);
    target.draw(shape);
    circle(target, tip + sf::Vector2f(0, size * 1.25f), size * .62f, color);
}
void heart(sf::RenderTarget &target, sf::Vector2f c, float s, sf::Color color) {
    circle(target, c + sf::Vector2f(-s * .28f, -s * .1f), s * .32f, color);
    circle(target, c + sf::Vector2f(s * .28f, -s * .1f), s * .32f, color);
    sf::ConvexShape tip(3);
    tip.setPoint(0, c + sf::Vector2f(-s * .58f, 0));
    tip.setPoint(1, c + sf::Vector2f(s * .58f, 0));
    tip.setPoint(2, c + sf::Vector2f(0, s * .62f));
    tip.setFillColor(color);
    target.draw(tip);
}
void line(sf::RenderTarget &target, sf::Vector2f a, sf::Vector2f b, float width, sf::Color color) {
    const sf::Vector2f d = b - a;
    const float length = std::hypot(d.x, d.y);
    sf::RectangleShape r({length, width});
    r.setOrigin(0, width / 2);
    r.setPosition(a);
    r.setRotation(std::atan2(d.y, d.x) * 180.f / pi);
    r.setFillColor(color);
    target.draw(r);
}
} // namespace
float Atmosphere::random() {
    seed_ = seed_ * 1664525u + 1013904223u;
    return static_cast<float>(seed_ >> 8) / static_cast<float>(1u << 24);
}
void Atmosphere::update(float dt, Weather weather, float hour) {
    dt = std::clamp(dt, 0.f, .1f);
    hour_ = hour;
    const float rainTarget = weather == Weather::Rainy ? 1.f : 0.f;
    const float cloudTarget = weather == Weather::Sunny ? 0.f : 1.f;
    rain_ += (rainTarget - rain_) * std::min(1.f, dt * .9f);
    cloud_ += (cloudTarget - cloud_) * std::min(1.f, dt * .9f);
    if (std::abs(rain_ - rainTarget) < .002f) rain_ = rainTarget;
    const std::size_t wanted = static_cast<std::size_t>(170.f * rain_);
    while (drops_.size() < wanted)
        drops_.push_back({{random() * 1400.f - 60.f, random() * 380.f - 20.f}, 620.f + random() * 260.f, 14.f + random() * 12.f});
    if (drops_.size() > wanted) drops_.resize(wanted);
    for (auto &d : drops_) {
        d.position += sf::Vector2f(-.18f, 1.f) * d.speed * dt;
        if (d.position.y > 366.f) {
            if (random() < .35f) splashes_.push_back({{d.position.x, 360.f + random() * 8.f}, 0.f});
            d.position = {random() * 1400.f - 60.f, -20.f - random() * 40.f};
        }
    }
    for (auto &s : splashes_) s.age += dt;
    splashes_.erase(std::remove_if(splashes_.begin(), splashes_.end(), [](const Splash &s) { return s.age > .32f; }),
                    splashes_.end());
}
void Atmosphere::shade(sf::RenderTarget &target, sf::FloatRect area, float strength) const {
    const auto state = light();
    const auto mixed = [&](sf::Uint8 v) { return static_cast<sf::Uint8>(255.f - (255.f - v) * std::clamp(strength, 0.f, 1.f)); };
    sf::RectangleShape r({area.width, area.height});
    r.setPosition(area.left, area.top);
    r.setFillColor({mixed(state.tint.r), mixed(state.tint.g), mixed(state.tint.b)});
    target.draw(r, sf::RenderStates(sf::BlendMultiply));
    // Trời tối: phủ thêm một lớp xanh đêm mỏng để cảnh dịu, không đen kịt.
    if (state.night > .02f) {
        r.setFillColor({28, 30, 64, static_cast<sf::Uint8>(90.f * state.night * strength)});
        target.draw(r);
    }
}
void Atmosphere::nightLights(sf::RenderTarget &target, float time, bool lanterns) const {
    const float night = light().night;
    if (night < .12f) return;
    const float a = std::clamp((night - .12f) / .5f, 0.f, 1.f);
    // Cửa sổ và biển hiệu các căn nhà sáng đèn (tọa độ theo ảnh nền đường phố).
    const sf::Vector2f windows[] = {{608, 238}, {905, 236}, {1060, 236}, {420, 248}, {275, 260}, {1170, 240}};
    for (std::size_t i = 0; i < std::size(windows); ++i) {
        const float flicker = .9f + .1f * std::sin(time * 3.f + static_cast<float>(i) * 2.1f);
        radial(target, windows[i], {70, 46}, {255, 196, 110, static_cast<sf::Uint8>(120 * a * flicker)}, {255, 196, 110, 0}, true);
    }
    // Đèn đường ấm hắt xuống quầy.
    for (float x : {170.f, 640.f, 1110.f}) {
        radial(target, {x, 150}, {150, 120}, {255, 214, 150, static_cast<sf::Uint8>(70 * a)}, {255, 214, 150, 0}, true);
    }
    if (lanterns)
        for (float x : {73.f, 1207.f})
            radial(target, {x, 170}, {110, 120}, {255, 150, 60, static_cast<sf::Uint8>(110 * a)}, {255, 150, 60, 0}, true);
}
void Atmosphere::drawRain(sf::RenderTarget &target, sf::FloatRect area) const {
    if (rain_ <= .01f) return;
    sf::VertexArray streaks(sf::Lines, drops_.size() * 2);
    const sf::Color head{225, 236, 255, static_cast<sf::Uint8>(200 * rain_)}, tail{220, 232, 255, 0};
    for (std::size_t i = 0; i < drops_.size(); ++i) {
        const auto &d = drops_[i];
        if (!area.contains(d.position)) continue;
        streaks[i * 2] = {d.position, head};
        streaks[i * 2 + 1] = {d.position - sf::Vector2f(-.18f, 1.f) * d.length, tail};
    }
    target.draw(streaks);
    // Màn mưa mờ phía xa.
    sf::RectangleShape haze({area.width, area.height});
    haze.setPosition(area.left, area.top);
    haze.setFillColor({170, 186, 210, static_cast<sf::Uint8>(38 * rain_)});
    target.draw(haze);
}
void Atmosphere::drawSplashes(sf::RenderTarget &target) const {
    for (const auto &s : splashes_) {
        const float t = s.age / .32f;
        sf::CircleShape ring(4.f + 10.f * t, 18);
        ring.setOrigin(ring.getRadius(), ring.getRadius());
        ring.setScale(1.f, .3f);
        ring.setPosition(s.position);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(1.2f);
        ring.setOutlineColor({225, 236, 255, static_cast<sf::Uint8>(160 * (1.f - t) * rain_)});
        target.draw(ring);
    }
}
void weatherIcon(sf::RenderTarget &target, Weather weather, sf::Vector2f c, float s, float time) {
    if (weather == Weather::Sunny) {
        radial(target, c, {s * .9f, s * .9f}, {255, 214, 98, 120}, {255, 214, 98, 0}, true);
        for (int i = 0; i < 8; ++i) {
            const float a = static_cast<float>(i) / 8.f * 2.f * pi + time * .5f;
            line(target, c + sf::Vector2f(std::cos(a), std::sin(a)) * (s * .42f),
                 c + sf::Vector2f(std::cos(a), std::sin(a)) * (s * .6f), s * .08f, {244, 172, 46});
        }
        circle(target, c, s * .32f, {250, 190, 60});
        circle(target, c + sf::Vector2f(-s * .07f, -s * .08f), s * .18f, {255, 226, 130});
        return;
    }
    if (weather == Weather::Cloudy) {
        circle(target, c + sf::Vector2f(s * .22f, -s * .18f), s * .22f, {250, 190, 60});
        cloudShape(target, c + sf::Vector2f(0, s * .08f), s * .95f, {236, 238, 244});
        return;
    }
    cloudShape(target, c + sf::Vector2f(0, -s * .1f), s * .95f, {178, 190, 210});
    for (int i = 0; i < 3; ++i) {
        const float fall = std::fmod(time * 1.6f + static_cast<float>(i) * .33f, 1.f);
        drop(target, c + sf::Vector2f((static_cast<float>(i) - 1.f) * s * .28f, s * (.2f + .25f * fall)), s * .09f,
             {90, 150, 220, static_cast<sf::Uint8>(255 * (1.f - fall))});
    }
}
void clockIcon(sf::RenderTarget &target, sf::Vector2f c, float radius, float hour, sf::Color face, sf::Color ink) {
    sf::CircleShape dial(radius, 32);
    dial.setOrigin(radius, radius);
    dial.setPosition(c);
    dial.setFillColor(face);
    dial.setOutlineColor(ink);
    dial.setOutlineThickness(2.f);
    target.draw(dial);
    for (int i = 0; i < 12; ++i) {
        const float a = static_cast<float>(i) / 12.f * 2.f * pi;
        circle(target, c + sf::Vector2f(std::sin(a), -std::cos(a)) * (radius * .78f), i % 3 ? .8f : 1.4f, ink);
    }
    const float h = std::fmod(hour, 12.f) / 12.f * 2.f * pi;
    const float m = std::fmod(hour, 1.f) * 2.f * pi;
    line(target, c, c + sf::Vector2f(std::sin(h), -std::cos(h)) * (radius * .5f), 2.6f, ink);
    line(target, c, c + sf::Vector2f(std::sin(m), -std::cos(m)) * (radius * .75f), 1.6f, ink);
    circle(target, c, 2.f, ink);
}
void emote(sf::RenderTarget &target, const sf::Font &font, Expression e, sf::Vector2f a, float time, float k) {
    const float bob = std::sin(time * 4.f) * 2.f * k;
    switch (e) {
    case Expression::Delighted:
        for (int i = 0; i < 3; ++i) {
            const float rise = std::fmod(time * .8f + static_cast<float>(i) * .33f, 1.f);
            heart(target, a + sf::Vector2f((static_cast<float>(i) - 1.f) * 16.f * k + std::sin(time * 3.f + i) * 4.f, -rise * 30.f * k),
                  (8.f + 3.f * static_cast<float>(i % 2)) * k, {232, 84, 104, static_cast<sf::Uint8>(255 * (1.f - rise))});
        }
        break;
    case Expression::Happy: {
        // Nốt nhạc nhỏ nhún nhảy.
        const sf::Vector2f n = a + sf::Vector2f(10.f * k, bob - 4.f * k);
        circle(target, n, 4.6f * k, {236, 126, 52});
        panel(target, {n.x + 3.2f * k, n.y - 16.f * k, 2.2f * k, 16.f * k}, {236, 126, 52}, 0);
        panel(target, {n.x + 3.2f * k, n.y - 16.f * k, 7.f * k, 3.f * k}, {236, 126, 52}, 1);
        break;
    }
    case Expression::Bored: {
        panel(target, {a.x - 18.f * k, a.y - 12.f * k + bob, 38.f * k, 18.f * k}, {255, 252, 240, 235}, 9.f * k);
        for (int i = 0; i < 3; ++i) {
            const float on = std::fmod(time * 1.5f, 1.5f) > static_cast<float>(i) * .4f ? 1.f : .25f;
            circle(target, {a.x - 9.f * k + static_cast<float>(i) * 9.f * k, a.y - 3.f * k + bob}, 2.4f * k, alpha({133, 104, 86}, on));
        }
        text(target, font, "z", static_cast<unsigned>(14 * k), a + sf::Vector2f(22.f * k, -26.f * k + bob), {120, 130, 170});
        break;
    }
    case Expression::Worried:
        drop(target, a + sf::Vector2f(14.f * k, -14.f * k + std::fmod(time * 12.f, 10.f) * .6f * k), 5.f * k, {120, 186, 236});
        break;
    case Expression::Angry: {
        // Gân tức: bốn vệt đỏ quanh tâm, nhịp phập phồng; hơi nóng bốc lên.
        const float pulse = 1.f + .18f * std::sin(time * 9.f);
        const sf::Vector2f c = a + sf::Vector2f(12.f * k, -10.f * k);
        for (int i = 0; i < 4; ++i) {
            const float ang = static_cast<float>(i) * pi / 2.f + pi / 4.f;
            const sf::Vector2f dir{std::cos(ang), std::sin(ang)};
            const sf::Vector2f side{-dir.y, dir.x};
            line(target, c + dir * (4.f * k * pulse) + side * (3.f * k), c + dir * (9.f * k * pulse), 2.6f * k, {214, 54, 48});
            line(target, c + dir * (4.f * k * pulse) - side * (3.f * k), c + dir * (9.f * k * pulse), 2.6f * k, {214, 54, 48});
        }
        for (int i = 0; i < 2; ++i) {
            const float rise = std::fmod(time * 1.3f + static_cast<float>(i) * .5f, 1.f);
            radial(target, a + sf::Vector2f((static_cast<float>(i) * 2.f - 1.f) * 22.f * k, -8.f * k - rise * 22.f * k),
                   {7.f * k, 7.f * k}, {255, 255, 255, static_cast<sf::Uint8>(150 * (1.f - rise))}, {255, 255, 255, 0});
        }
        break;
    }
    case Expression::Sad:
        // Vạch u ám và giọt nước mắt.
        for (int i = 0; i < 3; ++i)
            line(target, a + sf::Vector2f(-14.f * k + static_cast<float>(i) * 9.f * k, -18.f * k),
                 a + sf::Vector2f(-14.f * k + static_cast<float>(i) * 9.f * k, -6.f * k), 2.f * k, {110, 120, 170, 200});
        drop(target, a + sf::Vector2f(18.f * k, -6.f * k + std::fmod(time * 10.f, 8.f) * .8f * k), 4.f * k, {120, 176, 236});
        break;
    case Expression::Surprised:
        outlinedText(target, font, "!", static_cast<unsigned>(26 * k), a + sf::Vector2f(10.f * k, -10.f * k + bob), {255, 214, 98}, {120, 70, 30}, 2.f);
        break;
    case Expression::Neutral:
        break;
    }
}
sf::Vector2f expressionMotion(Expression e, float time, unsigned seed) {
    const float phase = static_cast<float>(seed % 97) * .37f;
    switch (e) {
    case Expression::Delighted: return {0.f, -std::abs(std::sin(time * 5.5f + phase)) * 7.f};
    case Expression::Happy: return {0.f, std::sin(time * 2.6f + phase) * 2.6f};
    case Expression::Bored: return {std::sin(time * 1.1f + phase) * 3.5f, 2.f + std::sin(time * .9f + phase) * 1.5f};
    case Expression::Worried: return {std::sin(time * 14.f + phase) * 1.1f, std::abs(std::sin(time * 7.f + phase)) * -2.f};
    case Expression::Angry: return {std::sin(time * 38.f + phase) * 1.8f, 0.f};
    case Expression::Sad: return {0.f, 5.f + std::sin(time * 1.4f + phase) * 1.f};
    case Expression::Surprised: return {0.f, -4.f};
    case Expression::Neutral: return {0.f, std::sin(time * 2.1f + phase) * 2.2f};
    }
    return {};
}
} // namespace tiem::ui

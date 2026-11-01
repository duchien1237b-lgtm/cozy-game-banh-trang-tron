#include "ui/Draw.hpp"
#include "core/Utf8.hpp"
#include <algorithm>
#include <cmath>

namespace tiem::ui {
void panel(sf::RenderTarget &target, sf::FloatRect rect, sf::Color color, float radius) {
    radius = std::max(0.f, std::min(radius, std::min(rect.width, rect.height) / 2.f));
    if (rect.width <= 0 || rect.height <= 0)
        return;
    // Một đa giác duy nhất: tránh chồng alpha ở các góc bo tròn.
    if (radius == 0) {
        sf::RectangleShape shape({rect.width, rect.height});
        shape.setPosition(rect.left, rect.top);
        shape.setFillColor(color);
        target.draw(shape);
        return;
    }
    constexpr int segments = 8;
    sf::ConvexShape shape(4 * (segments + 1));
    const sf::Vector2f centers[] = {{rect.left + rect.width - radius, rect.top + radius},
                                    {rect.left + rect.width - radius, rect.top + rect.height - radius},
                                    {rect.left + radius, rect.top + rect.height - radius},
                                    {rect.left + radius, rect.top + radius}};
    for (int corner = 0; corner < 4; ++corner) {
        for (int i = 0; i <= segments; ++i) {
            const float angle =
                (static_cast<float>(corner - 1) + static_cast<float>(i) / segments) * 1.5707963f;
            shape.setPoint(static_cast<std::size_t>(corner * (segments + 1) + i),
                           centers[corner] + sf::Vector2f(std::cos(angle), std::sin(angle)) * radius);
        }
    }
    shape.setFillColor(color);
    target.draw(shape);
}

void text(sf::RenderTarget &target, const sf::Font &font, std::string_view label, unsigned int size,
          sf::Vector2f position, sf::Color color, bool centered) {
    sf::Text value(utf8(label), font, size);
    value.setFillColor(color);
    const auto bounds = value.getLocalBounds();
    if (centered)
        value.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    else
        value.setOrigin(bounds.left, bounds.top);
    value.setPosition(position);
    target.draw(value);
}

void backdrop(sf::RenderTarget &target) {
    const Theme theme;
    panel(target, {0.f, 0.f, 1280.f, 720.f}, theme.cream, 0.f);
    panel(target, {0.f, 0.f, 1280.f, 16.f}, theme.green, 0.f);
    panel(target, {0.f, 636.f, 1280.f, 84.f}, {232, 209, 176}, 0.f);
    for (int i = 0; i < 8; ++i)
        panel(target, {static_cast<float>(i) * 180.f, 637.f, 2.f, 83.f}, {219, 192, 155}, 0.f);
}
} // namespace tiem::ui

namespace tiem::ui {
namespace {
float hash01(int n) {
    unsigned x = static_cast<unsigned>(n) * 2654435761u + 0x9e3779b9u;
    x ^= x >> 15; x *= 2246822519u; x ^= x >> 13; x *= 3266489917u; x ^= x >> 16;
    return static_cast<float>(x & 0xffffff) / static_cast<float>(0x1000000);
}
} // namespace
float textWidth(const sf::Font &font, std::string_view label, unsigned int size) {
    sf::Text value(utf8(label), font, size);
    return value.getLocalBounds().width;
}
void textFit(sf::RenderTarget &target, const sf::Font &font, std::string_view label, unsigned int size,
             sf::Vector2f position, sf::Color color, float maxWidth, bool centered, unsigned int minSize) {
    while (size > minSize && textWidth(font, label, size) > maxWidth) --size;
    text(target, font, label, size, position, color, centered);
}
void outlinedText(sf::RenderTarget &target, const sf::Font &font, std::string_view label, unsigned int size,
                  sf::Vector2f center, sf::Color fill, sf::Color outline, float thickness) {
    sf::Text value(utf8(label), font, size);
    const auto bounds = value.getLocalBounds();
    value.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    value.setPosition(center);
    value.setFillColor(fill);
    value.setOutlineColor(outline);
    value.setOutlineThickness(thickness);
    target.draw(value);
}
std::size_t paragraph(sf::RenderTarget &target, const sf::Font &font, std::string_view label, unsigned int size,
                      sf::Vector2f position, float width, float lineHeight, sf::Color color, bool centered) {
    std::vector<std::string> lines;
    std::string line;
    std::size_t i = 0;
    const std::string source(label);
    while (i <= source.size()) {
        const auto space = source.find(' ', i);
        const auto end = space == std::string::npos ? source.size() : space;
        const std::string word = source.substr(i, end - i);
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && textWidth(font, candidate, size) > width) {
            lines.push_back(line);
            line = word;
        } else line = candidate;
        if (space == std::string::npos) break;
        i = space + 1;
    }
    if (!line.empty()) lines.push_back(line);
    for (std::size_t n = 0; n < lines.size(); ++n)
        text(target, font, lines[n], size, {position.x, position.y + static_cast<float>(n) * lineHeight}, color, centered);
    return lines.size();
}
void shadowPanel(sf::RenderTarget &target, sf::FloatRect rect, sf::Color color, float radius, float offset,
                 sf::Uint8 shadowAlpha) {
    // Hai lớp bóng chồng nhẹ cho mép mềm thay vì một khối đặc.
    panel(target, {rect.left - 2, rect.top + offset + 2, rect.width + 4, rect.height + 2}, {60, 38, 22, static_cast<sf::Uint8>(shadowAlpha / 2)}, radius + 3);
    panel(target, {rect.left, rect.top + offset, rect.width, rect.height}, {60, 38, 22, shadowAlpha}, radius);
    panel(target, rect, color, radius);
}
void panelOutline(sf::RenderTarget &target, sf::FloatRect rect, sf::Color color, float radius, float thickness) {
    // Viền rỗng bo góc, vẽ bên ngoài rect: không che nội dung bên trong.
    radius = std::max(0.f, std::min(radius, std::min(rect.width, rect.height) / 2.f));
    constexpr int segments = 8;
    sf::ConvexShape shape(4 * (segments + 1));
    const sf::Vector2f centers[] = {{rect.left + rect.width - radius, rect.top + radius},
                                    {rect.left + rect.width - radius, rect.top + rect.height - radius},
                                    {rect.left + radius, rect.top + rect.height - radius},
                                    {rect.left + radius, rect.top + radius}};
    for (int corner = 0; corner < 4; ++corner)
        for (int i = 0; i <= segments; ++i) {
            const float angle = (static_cast<float>(corner - 1) + static_cast<float>(i) / segments) * 1.5707963f;
            shape.setPoint(static_cast<std::size_t>(corner * (segments + 1) + i),
                           centers[corner] + sf::Vector2f(std::cos(angle), std::sin(angle)) * radius);
        }
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(color);
    shape.setOutlineThickness(thickness);
    target.draw(shape);
}
void gradientRect(sf::RenderTarget &target, sf::FloatRect r, sf::Color top, sf::Color bottom) {
    sf::VertexArray quad(sf::Quads, 4);
    quad[0] = {{r.left, r.top}, top};
    quad[1] = {{r.left + r.width, r.top}, top};
    quad[2] = {{r.left + r.width, r.top + r.height}, bottom};
    quad[3] = {{r.left, r.top + r.height}, bottom};
    target.draw(quad);
}
void radial(sf::RenderTarget &target, sf::Vector2f center, sf::Vector2f radius, sf::Color inner, sf::Color outer,
            bool additive) {
    constexpr int segments = 40;
    sf::VertexArray fan(sf::TriangleFan, segments + 2);
    fan[0] = {center, inner};
    for (int i = 0; i <= segments; ++i) {
        const float angle = static_cast<float>(i) / segments * 6.2831853f;
        fan[static_cast<std::size_t>(i + 1)] = {center + sf::Vector2f(std::cos(angle) * radius.x, std::sin(angle) * radius.y), outer};
    }
    target.draw(fan, additive ? sf::RenderStates(sf::BlendAdd) : sf::RenderStates::Default);
}
void vignette(sf::RenderTarget &target, sf::Uint8 strength) {
    const sf::Color dark{30, 18, 10, strength}, clear{30, 18, 10, 0};
    gradientRect(target, {0, 0, 1280, 110}, dark, clear);
    gradientRect(target, {0, 610, 1280, 110}, clear, dark);
    sf::VertexArray left(sf::Quads, 4), right(sf::Quads, 4);
    left[0] = {{0, 0}, dark}; left[1] = {{150, 0}, clear}; left[2] = {{150, 720}, clear}; left[3] = {{0, 720}, dark};
    right[0] = {{1130, 0}, clear}; right[1] = {{1280, 0}, dark}; right[2] = {{1280, 720}, dark}; right[3] = {{1130, 720}, clear};
    target.draw(left);
    target.draw(right);
}
void keycap(sf::RenderTarget &target, const sf::Font &font, std::string_view label, sf::Vector2f center) {
    const float w = std::max(20.f, textWidth(font, label, 12) + 12.f);
    panel(target, {center.x - w / 2, center.y - 9, w, 20}, {96, 68, 48, 90}, 6);
    panel(target, {center.x - w / 2, center.y - 11, w, 20}, {255, 247, 224}, 6);
    panel(target, {center.x - w / 2 + 1.5f, center.y - 9.5f, w - 3, 15}, {248, 232, 196}, 5);
    text(target, font, label, 12, {center.x, center.y - 2}, {89, 60, 42}, true);
}
void checkMark(sf::RenderTarget &target, sf::Vector2f c, float r, sf::Color background, sf::Color mark) {
    sf::CircleShape disc(r, 24);
    disc.setOrigin(r, r);
    disc.setPosition(c);
    disc.setFillColor(background);
    target.draw(disc);
    sf::VertexArray tick(sf::LineStrip, 3);
    tick[0] = {c + sf::Vector2f(-r * .45f, r * .02f), mark};
    tick[1] = {c + sf::Vector2f(-r * .1f, r * .38f), mark};
    tick[2] = {c + sf::Vector2f(r * .5f, -r * .34f), mark};
    // Vẽ ba lần lệch nhau để nét dày hơn 1px.
    for (float o : {-1.f, 0.f, 1.f}) {
        sf::VertexArray shifted = tick;
        for (std::size_t i = 0; i < 3; ++i) shifted[i].position.y += o * .8f;
        target.draw(shifted);
    }
}
void dust(sf::RenderTarget &target, sf::FloatRect area, float time, int count, sf::Uint8 alpha) {
    for (int i = 0; i < count; ++i) {
        const float speed = .012f + hash01(i * 3 + 1) * .02f;
        const float phase = std::fmod(time * speed + hash01(i * 5 + 2), 1.f);
        const float x = area.left + std::fmod(hash01(i * 7 + 3) + phase * .6f, 1.f) * area.width;
        const float y = area.top + (1.f - phase) * area.height + std::sin(time * .7f + static_cast<float>(i)) * 8.f;
        const float twinkle = .45f + .55f * std::sin(time * (.8f + hash01(i)) + static_cast<float>(i) * 1.7f);
        const float r = 1.2f + hash01(i * 11 + 4) * 2.2f;
        const auto a = static_cast<sf::Uint8>(alpha * std::max(0.f, twinkle) * std::sin(phase * 3.14159f));
        radial(target, {x, y}, {r * 2.4f, r * 2.4f}, {255, 240, 200, a}, {255, 240, 200, 0}, true);
    }
}
void lightShafts(sf::RenderTarget &target, sf::FloatRect area, float time, sf::Uint8 alpha) {
    for (int i = 0; i < 4; ++i) {
        const float sway = std::sin(time * .25f + static_cast<float>(i) * 1.3f) * 14.f;
        const float x = area.left + (static_cast<float>(i) + .3f) * area.width / 4.f + sway;
        const float widthTop = 36.f + 18.f * static_cast<float>(i % 2);
        const float a = alpha * (.55f + .45f * std::sin(time * .4f + static_cast<float>(i) * 2.1f));
        sf::VertexArray shaft(sf::Quads, 4);
        const sf::Color strong{255, 232, 170, static_cast<sf::Uint8>(std::max(0.f, a))}, none{255, 232, 170, 0};
        shaft[0] = {{x, area.top}, strong};
        shaft[1] = {{x + widthTop, area.top}, strong};
        shaft[2] = {{x - 130 + widthTop * 2, area.top + area.height}, none};
        shaft[3] = {{x - 130, area.top + area.height}, none};
        target.draw(shaft, sf::RenderStates(sf::BlendAdd));
    }
}
void steam(sf::RenderTarget &target, sf::Vector2f base, float time, float intensity, float height) {
    if (intensity <= 0.01f) return;
    for (int i = 0; i < 9; ++i) {
        const float phase = std::fmod(time * (.22f + .05f * hash01(i)) + static_cast<float>(i) / 9.f, 1.f);
        const float sway = std::sin(time * 1.4f + static_cast<float>(i) * 2.3f + phase * 5.f) * (8.f + phase * 14.f);
        const float x = base.x + (hash01(i + 40) - .5f) * 70.f + sway;
        const float y = base.y - phase * height;
        const float r = 10.f + phase * 26.f;
        const auto a = static_cast<sf::Uint8>(70.f * intensity * std::sin(phase * 3.14159f) * (1.f - phase * .4f));
        radial(target, {x, y}, {r, r * .8f}, {255, 255, 255, a}, {255, 255, 255, 0});
    }
}
} // namespace tiem::ui

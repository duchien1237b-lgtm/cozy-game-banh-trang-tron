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

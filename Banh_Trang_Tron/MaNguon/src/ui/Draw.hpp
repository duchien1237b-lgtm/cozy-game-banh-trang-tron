#pragma once
#include <SFML/Graphics.hpp>
#include <string_view>

namespace tiem::ui {
struct Theme {
    sf::Color cream{255, 248, 230};
    sf::Color ink{81, 54, 44};
    sf::Color muted{133, 104, 86};
    sf::Color green{79, 111, 76};
    sf::Color peach{239, 166, 124};
    sf::Color paper{255, 254, 246};
};
void panel(sf::RenderTarget &target, sf::FloatRect rect, sf::Color color, float radius = 16.f);
void text(sf::RenderTarget &target, const sf::Font &font, std::string_view label, unsigned int size,
          sf::Vector2f position, sf::Color color, bool centered = false);
void backdrop(sf::RenderTarget &target);
} // namespace tiem::ui

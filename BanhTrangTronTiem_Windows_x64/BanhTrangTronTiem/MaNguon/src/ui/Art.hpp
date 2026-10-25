#pragma once
#include "gameplay/Model.hpp"
#include "ui/Draw.hpp"
namespace tiem::ui {
void ingredient(sf::RenderTarget &, const Topping &, sf::Vector2f, float scale = 1.f, float angle = 0.f);
void customer(sf::RenderTarget &, const CustomerKind &, sf::Vector2f, float mood = 1.f, float scale = 1.f);
void bowl(sf::RenderTarget &, const GameData &, const Bowl &, sf::Vector2f, float scale,
          bool highlight = false);
void star(sf::RenderTarget &, sf::Vector2f, float radius, sf::Color);
void shopBackground(sf::RenderTarget &, const GameData &, const Progress &, float time);
} // namespace tiem::ui

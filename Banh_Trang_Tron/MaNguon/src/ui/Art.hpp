#pragma once
#include "gameplay/Model.hpp"
#include "ui/Draw.hpp"
#include "core/ResourceManager.hpp"
namespace tiem::ui {
// Fit a sprite inside bounds without distorting its aspect ratio.
bool image(sf::RenderTarget &, ResourceManager &, const std::string &, sf::FloatRect, float angle = 0.f);
// Stretch only the middle of an ornamental frame, preserving its corners.
bool frame(sf::RenderTarget &, ResourceManager &, const std::string &, sf::FloatRect,
           sf::Vector2f sourceCorner, sf::Vector2f displayCorner);
// Fill the scene with a centered crop, preserving the original image proportions.
bool background(sf::RenderTarget &, ResourceManager &, const std::string &, sf::FloatRect);
void ingredient(sf::RenderTarget &, ResourceManager &, const Topping &, sf::Vector2f, float scale = 1.f, float angle = 0.f);
void customer(sf::RenderTarget &, ResourceManager &, const CustomerKind &, sf::Vector2f, float mood = 1.f, float scale = 1.f);
void bowl(sf::RenderTarget &, ResourceManager &, const GameData &, const Bowl &, sf::Vector2f, float scale,
          bool highlight = false);
void star(sf::RenderTarget &, sf::Vector2f, float radius, sf::Color);
void shopBackground(sf::RenderTarget &, const GameData &, const Progress &, float time);
} // namespace tiem::ui

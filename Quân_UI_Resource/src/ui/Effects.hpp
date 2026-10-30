#pragma once
#include "ui/Draw.hpp"
#include <vector>
namespace tiem::ui {
inline float easeOut(float t) {
    return 1.f - (1.f - t) * (1.f - t) * (1.f - t);
}
struct ProgressBar {
    static void draw(sf::RenderTarget &, sf::FloatRect, float ratio, sf::Color color);
};
struct FloatingText {
    std::string text;
    sf::Vector2f position;
    sf::Color color;
    float age{}, life{};
    unsigned size{25};
    bool outlined{false};
};
struct Particle {
    sf::Vector2f position, velocity;
    sf::Color color;
    float age{}, life{};
};
class Effects {
  public:
    void say(std::string text, sf::Vector2f position, sf::Color color, float life = 1.6f,
             unsigned size = 25, bool outlined = false);
    void burst(sf::Vector2f position, sf::Color color, int count = 12);
    // Mưa tia sáng nhỏ rơi quanh một điểm (thưởng, mở khóa).
    void sparkle(sf::Vector2f position, sf::Color color, int count = 18);
    void update(float dt);
    void render(sf::RenderTarget &, const sf::Font &) const;

  private:
    std::vector<FloatingText> texts_;
    std::vector<Particle> particles_;
};
struct Tooltip {
    static void draw(sf::RenderTarget &, const sf::Font &, const std::string &, sf::Vector2f);
};
} // namespace tiem::ui

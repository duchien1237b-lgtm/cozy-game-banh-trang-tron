#include "ui/Art.hpp"
#include <cmath>
namespace tiem::ui {
namespace {
void circle(sf::RenderTarget &t, sf::Vector2f p, float r, sf::Color c, sf::Vector2f scale = {1, 1}) {
    sf::CircleShape s(r, 40);
    s.setOrigin(r, r);
    s.setPosition(p);
    s.setFillColor(c);
    s.setScale(scale);
    t.draw(s);
}
void stroke(sf::RenderTarget &t, sf::Vector2f p, sf::Vector2f size, float angle, sf::Color c) {
    sf::RectangleShape s(size);
    s.setOrigin(size * .5f);
    s.setPosition(p);
    s.setRotation(angle);
    s.setFillColor(c);
    t.draw(s);
}
} // namespace
void ingredient(sf::RenderTarget &target, const Topping &item, sf::Vector2f p, float scale, float angle) {
    const auto c = item.color;
    if (item.id == "trung_cut") {
        circle(target, p, 12 * scale, {113, 80, 50}, {.8f, 1});
        circle(target, p, 11 * scale, c, {.8f, 1});
        circle(target, p + sf::Vector2f(0, 2 * scale), 5 * scale, {239, 179, 62});
    } else if (item.id == "rau_ram") {
        for (int i = -1; i <= 1; ++i) {
            sf::ConvexShape leaf(4);
            leaf.setPoint(0, {0, -15 * scale});
            leaf.setPoint(1, {7 * scale, 0});
            leaf.setPoint(2, {0, 15 * scale});
            leaf.setPoint(3, {-7 * scale, 0});
            leaf.setPosition(p);
            leaf.setRotation(angle + static_cast<float>(i) * 48);
            leaf.setFillColor(c);
            target.draw(leaf);
        }
    } else if (item.type == "sauce") {
        circle(target, p, 10 * scale, c, {1.35f, .45f});
        circle(target, p + sf::Vector2f(12 * scale, 5 * scale), 3 * scale, c);
    } else if (item.id == "xuc_xich" || item.id == "dau_phong" || item.id == "top_mo") {
        circle(target, p, 9 * scale, {135, 87, 50}, {1, .75f});
        circle(target, p, 8 * scale, c, {1, .75f});
        stroke(target, p, {10 * scale, 2 * scale}, angle, {243, 208, 148});
    } else {
        for (int i = 0; i < 3; ++i) {
            const auto q = p + sf::Vector2f(static_cast<float>(i - 1) * 5 * scale,
                                            static_cast<float>(i % 2) * 5 * scale);
            stroke(target, q, {(item.id == "muoi_tom" ? 7.f : 25.f) * scale, 4 * scale},
                   angle + static_cast<float>(i) * 18, c);
        }
    }
}
void customer(sf::RenderTarget &target, const CustomerKind &k, sf::Vector2f p, float mood, float scale) {
    circle(target, p + sf::Vector2f(0, 27 * scale), 25 * scale, k.color, {1.1f, .7f});
    circle(target, p, 24 * scale, {81, 54, 44});
    circle(target, p + sf::Vector2f(0, 3 * scale), 21 * scale, {237, 185, 141});
    circle(target, p + sf::Vector2f(-19 * scale, 4 * scale), 5 * scale, {237, 185, 141});
    circle(target, p + sf::Vector2f(19 * scale, 4 * scale), 5 * scale, {237, 185, 141});
    if (k.style == "courier") {
        circle(target, p + sf::Vector2f(0, -13 * scale), 24 * scale, k.color, {1, .55f});
        stroke(target, p + sf::Vector2f(5 * scale, -7 * scale), {45 * scale, 5 * scale}, 0, k.color);
    } else if (k.style == "auntie" || k.style == "office") {
        circle(target, p + sf::Vector2f(15 * scale, -17 * scale), 12 * scale, {76, 50, 43});
        circle(target, p + sf::Vector2f(-8 * scale, -17 * scale), 18 * scale, {76, 50, 43}, {1, .5f});
    } else {
        circle(target, p + sf::Vector2f(-4 * scale, -16 * scale), 20 * scale, {76, 50, 43}, {1, .4f});
    }
    for (float x : {-8.f, 8.f})
        circle(target, p + sf::Vector2f(x * scale, 3 * scale), 2.2f * scale, {58, 43, 35});
    if (mood < .3f) {
        stroke(target, p + sf::Vector2f(-8 * scale, -3 * scale), {10 * scale, 2 * scale}, 25, {76, 50, 43});
        stroke(target, p + sf::Vector2f(8 * scale, -3 * scale), {10 * scale, 2 * scale}, -25, {76, 50, 43});
    }
    sf::VertexArray smile(sf::LineStrip, 13);
    for (std::size_t i = 0; i < 13; ++i) {
        float x = static_cast<float>(i) - 6;
        smile[i].position =
            p + sf::Vector2f(x * scale, (13.f + (mood < .3f ? -1.f : 1.f) * (3.f - x * x / 12.f)) * scale);
        smile[i].color = {100, 54, 43};
    }
    target.draw(smile);
}
void bowl(sf::RenderTarget &target, const GameData &data, const Bowl &bowlData, sf::Vector2f p, float scale,
          bool highlight) {
    circle(target, p + sf::Vector2f(0, 8 * scale), 125 * scale, {194, 164, 125}, {1, .78f});
    circle(target, p, 126 * scale, highlight ? sf::Color(109, 154, 95) : sf::Color(218, 196, 163), {1, .78f});
    circle(target, p, 120 * scale, {255, 253, 239}, {1, .78f});
    circle(target, p, 106 * scale, {245, 226, 190}, {1, .78f});
    for (int i = 0; i < 26; ++i) {
        const float a = static_cast<float>(i) * 2.39996f, radius = 20.f + static_cast<float>(i % 7) * 11;
        stroke(target, p + sf::Vector2f(std::cos(a) * radius * scale, std::sin(a) * radius * .75f * scale),
               {33 * scale, 6 * scale}, a * 57.3f, {234, 183, 115});
    }
    for (std::size_t i = 0; i < bowlData.portions.size(); ++i) {
        const float a = static_cast<float>(i) * 2.39996f, r = 17.f + std::sqrt(static_cast<float>(i)) * 15;
        ingredient(target, data.topping(bowlData.portions[i]),
                   p + sf::Vector2f(std::cos(a) * std::min(r, 87.f) * scale,
                                    std::sin(a) * std::min(r, 87.f) * .7f * scale),
                   scale * .85f, a * 57.3f);
    }
}
void star(sf::RenderTarget &target, sf::Vector2f p, float r, sf::Color c) {
    sf::ConvexShape s(10);
    for (int i = 0; i < 10; ++i) {
        float a = static_cast<float>(i) * .6283185f - 1.5708f;
        float radius = i % 2 ? r * .45f : r;
        s.setPoint(static_cast<std::size_t>(i), {std::cos(a) * radius, std::sin(a) * radius});
    }
    s.setPosition(p);
    s.setFillColor(c);
    target.draw(s);
}
void shopBackground(sf::RenderTarget &target, const GameData &data, const Progress &p, float time) {
    const Theme theme;
    panel(target, {0, 0, 1280, 720}, theme.cream, 0);
    panel(target, {0, 91, 1280, 166}, {222, 230, 205}, 0);
    panel(target, {0, 263, 1280, 457}, {226, 202, 165}, 0);
    for (int i = 0; i < 10; ++i)
        panel(target, {static_cast<float>(i) * 142, 263, 2, 457}, {216, 189, 150}, 0);
    panel(target, {0, 263, 1280, 5}, {180, 149, 115}, 0);
    if (UpgradeSystem::effect(data, p, "curtain") > 0) {
        for (int i = 0; i < 16; ++i)
            panel(target, {static_cast<float>(i) * 80, 0, 80, 19},
                  i % 2 ? sf::Color(235, 190, 151) : sf::Color(91, 127, 94), 7);
    }
    if (UpgradeSystem::effect(data, p, "lantern") > 0) {
        for (float x : {18.f, 1262.f}) {
            const float y = 70 + std::sin(time * 1.5f) * 3;
            stroke(target, {x, y - 25}, {2, 48}, 0, {100, 71, 47});
            circle(target, {x, y}, 15, {234, 159, 83}, {.8f, 1});
            stroke(target, {x, y + 22}, {2, 14}, 0, {168, 94, 54});
        }
    }
}
} // namespace tiem::ui

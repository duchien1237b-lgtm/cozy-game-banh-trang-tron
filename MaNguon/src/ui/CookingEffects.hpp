#pragma once
#include "core/ResourceManager.hpp"
#include <array>
#include <vector>

namespace tiem::ui {
// The six cells are trimmed once on load; animation never reads pixels from the GPU.
class CookingEffects {
  public:
    enum Model { Shard, Tear, Glint, Miss, Good, Perfect };
    explicit CookingEffects(ResourceManager &);
    void clear();
    void update(float dt);
    void tear(sf::Vector2f position, bool phaseComplete = false);
    void pulse(Model model, sf::Vector2f position, float size, float life = .55f);
    void sprite(sf::RenderTarget &, Model, sf::Vector2f, float size,
                float rotation = 0, float opacity = 1) const;
    void render(sf::RenderTarget &) const;
  private:
    struct Motif {
        Model model;
        sf::Vector2f position, velocity;
        float size, age, life, angle, spin;
    };
    void add(Motif);
    const sf::Texture *atlas_{};
    std::array<sf::IntRect, 6> cells_{};
    std::vector<Motif> motifs_;
    unsigned sequence_{};
};
} // namespace tiem::ui

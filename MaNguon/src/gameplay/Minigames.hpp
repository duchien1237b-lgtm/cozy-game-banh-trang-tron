#pragma once
#include "gameplay/Model.hpp"
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <optional>
namespace tiem {
class PaperTearing {
  public:
    PaperTearing(const GameData &, sf::FloatRect area, float bonus = 0.f, int upgradeLevel = 0);
    void begin(sf::Vector2f);
    void move(sf::Vector2f);
    std::optional<float> release();
    // Cancel only the current gesture; completed tearing time is kept.
    void cancel();
    void update(float dt);
    bool done() const;
    bool active() const { return held_ && inside_; }
    int completedStages() const;
    float progress() const;
    float elapsed() const { return static_cast<float>(elapsed_); }
    float stageSeconds() const { return stageSeconds_; }
    float quality() const { return done() ? 1.f : 0.f; }
    bool contains(sf::Vector2f) const;
  private:
    sf::FloatRect area_;
    float stageSeconds_, bonus_;
    double elapsed_{};
    bool held_{}, inside_{}, moved_{};
    sf::Vector2f previous_;
};
// One press/release commits one batch. Holding at maximum never auto-adds portions.
class ToppingCharge {
  public:
    explicit ToppingCharge(float seconds = 2.4f);
    void begin();
    void update(float dt);
    void cancel();
    int release();
    int portions() const;
    float power() const { return elapsed_ / seconds_; }
    bool active() const { return active_; }
  private:
    float seconds_, elapsed_{};
    bool active_{};
};
class MixingMinigame {
  public:
    MixingMinigame(const GameData &, float bonus, int day);
    void update(float dt);
    std::optional<float> hit();
    bool done() const {
        return hits_ >= data_.integer("mix_hits");
    }
    float score() const {
        return hits_ ? sum_ / static_cast<float>(hits_) : 0;
    }
    float position() const {
        return position_;
    }
    float width() const {
        return width_;
    }
    float target() const { return target_; }
    int hits() const {
        return hits_;
    }

  private:
    const GameData &data_;
    float position_{0.05f}, direction_{1}, speed_{}, width_{}, cooldown_{}, sum_{}, target_{.5f};
    int hits_{};
};
} // namespace tiem

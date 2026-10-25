#pragma once
#include "gameplay/Model.hpp"
#include <SFML/System/Vector2.hpp>
#include <optional>
namespace tiem {
class PaperTearing {
  public:
    PaperTearing(const GameData &, float bonus);
    void begin(sf::Vector2f);
    void move(sf::Vector2f);
    std::optional<float> release();
    void cancel();
    void update(float dt) {
        if (active_ >= 0)
            elapsed_ += dt;
    }
    bool done() const;
    bool cut(int line) const {
        return scores_[static_cast<std::size_t>(line)] >= 0;
    }
    float quality() const;
    int active() const {
        return active_;
    }
    static float lineY(int line) {
        return 375.f + static_cast<float>(line) * 44.f;
    }

  private:
    const GameData &data_;
    std::vector<float> scores_;
    std::vector<bool> samples_;
    int active_{-1};
    float tolerance_{}, elapsed_{}, deviation_{}, length_{};
    int moves_{};
    sf::Vector2f previous_;
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
    int hits() const {
        return hits_;
    }

  private:
    const GameData &data_;
    float position_{0.05f}, direction_{1}, speed_{}, width_{}, cooldown_{}, sum_{};
    int hits_{};
};
} // namespace tiem

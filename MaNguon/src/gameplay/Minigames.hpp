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
// Nhịp trộn: nhịp đầu luôn đều (dễ làm quen); từ nhịp thứ hai mỗi nhịp bốc ngẫu nhiên một kiểu,
// tốc độ, vị trí và độ rộng vùng xanh. Seed cố định giúp kiểm thử tái lập được.
enum class MixRhythm { Steady, Fast, Slow, Wobble, Reverse };
class MixingMinigame {
  public:
    MixingMinigame(const GameData &, float bonus, int day, unsigned seed = 0x6D1Bu);
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
    MixRhythm rhythm() const { return rhythm_; }
    float speed() const { return speed_ * speedScale_; }
    static const char *rhythmName(MixRhythm);

  private:
    void nextBeat();
    const GameData &data_;
    std::mt19937 random_;
    float position_{0.05f}, direction_{1}, speed_{}, width_{}, baseWidth_{}, cooldown_{}, sum_{}, target_{.5f};
    float speedScale_{1.f}, beatClock_{}, reverseAt_{-1.f};
    MixRhythm rhythm_{MixRhythm::Steady};
    int hits_{};
};
} // namespace tiem

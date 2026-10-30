#include "gameplay/Minigames.hpp"
#include <algorithm>
#include <cmath>
namespace tiem {
PaperTearing::PaperTearing(const GameData &data, sf::FloatRect area, float bonus, int upgradeLevel)
    : area_(area), stageSeconds_(upgradeLevel >= 3 ? .75f : upgradeLevel == 2 ? 1.25f : data.number("paper_stage_seconds")), bonus_(std::max(0.f, bonus)) {}
bool PaperTearing::contains(sf::Vector2f p) const {
    const float radius = std::min(area_.width, area_.height) * .5f + bonus_;
    const auto center = sf::Vector2f(area_.left + area_.width * .5f, area_.top + area_.height * .5f);
    return radius > 0 && std::hypot(p.x - center.x, p.y - center.y) <= radius;
}
void PaperTearing::begin(sf::Vector2f p) {
    cancel();
    if (!contains(p)) return;
    held_ = inside_ = true;
    previous_ = p;
}
void PaperTearing::move(sf::Vector2f p) {
    if (!held_ || done()) return;
    const bool nowInside = contains(p);
    if (!nowInside) moved_ = false;
    else if (inside_ && std::hypot(p.x - previous_.x, p.y - previous_.y) >= .5f) moved_ = true;
    inside_ = nowInside;
    previous_ = p;
}
void PaperTearing::update(float dt) {
    // Only count frames with a held pointer moving over the paper. Idle holds do not tear.
    // Bound a stalled frame's credit, matching the game's maximum simulation step.
    if (held_ && inside_ && moved_ && !done())
        elapsed_ = std::min(static_cast<double>(stageSeconds_) * 2,
                            elapsed_ + std::clamp(dt, 0.f, .25f));
    moved_ = false;
}
int PaperTearing::completedStages() const {
    return std::clamp(static_cast<int>((elapsed_ + .00001) / stageSeconds_), 0, 2);
}
bool PaperTearing::done() const { return completedStages() == 2; }
float PaperTearing::progress() const {
    return std::clamp(static_cast<float>(elapsed_ / (stageSeconds_ * 2)), 0.f, 1.f);
}
std::optional<float> PaperTearing::release() {
    const bool completedGesture = held_ && done();
    cancel();
    return completedGesture ? std::optional<float>(quality()) : std::nullopt;
}
void PaperTearing::cancel() {
    held_ = inside_ = moved_ = false;
}
ToppingCharge::ToppingCharge(float seconds) : seconds_(std::max(.1f, seconds)) {}
void ToppingCharge::begin() { elapsed_ = 0; active_ = true; }
void ToppingCharge::update(float dt) {
    if (active_) elapsed_ = std::min(seconds_, elapsed_ + std::max(0.f, dt));
}
void ToppingCharge::cancel() { elapsed_ = 0; active_ = false; }
int ToppingCharge::portions() const {
    return active_ && power() >= .15f ? std::clamp(static_cast<int>(std::ceil(power() * 3)), 1, 3) : 0;
}
int ToppingCharge::release() {
    const int amount = portions();
    cancel();
    return amount;
}
MixingMinigame::MixingMinigame(const GameData &d, float bonus, int day) : data_(d) {
    speed_ = std::min(d.number("mix_speed_max"),
                      d.number("mix_speed") * difficultyMultiplier(d, day));
    width_ = std::min(d.number("mix_window_max"), d.number("mix_window") + bonus);
}
void MixingMinigame::update(float dt) {
    if (done())
        return;
    cooldown_ = std::max(0.f, cooldown_ - dt);
    position_ += direction_ * speed_ * dt;
    while (position_ < 0 || position_ > 1) {
        if (position_ > 1) {
            position_ = 2 - position_;
            direction_ = -1;
        } else {
            position_ = -position_;
            direction_ = 1;
        }
    }
}
std::optional<float> MixingMinigame::hit() {
    if (done() || cooldown_ > 0)
        return std::nullopt;
    const float distance = std::abs(position_ - target_);
    const auto key = distance <= width_ * .2f ? "perfect" : (distance <= width_ * .5f ? "good" : "miss");
    const float value = data_.balance.at("mix_scores").at(key).get<float>();
    ++hits_;
    sum_ += value;
    cooldown_ = data_.number("mix_hit_cooldown")
    const float margin = width_ * .5f + .02f;
    const float fraction = hits_ % 2 ? .18f : .82f;
    target_ = margin + (1.f - 2.f * margin) * fraction;
    return value;
}
}

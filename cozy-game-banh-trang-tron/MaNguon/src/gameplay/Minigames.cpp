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
MixingMinigame::MixingMinigame(const GameData &d, float bonus, int day, unsigned seed) : data_(d), random_(seed) {
    speed_ = std::min(d.number("mix_speed_max"),
                      d.number("mix_speed") * difficultyMultiplier(d, day));
    width_ = baseWidth_ = std::min(d.number("mix_window_max"), d.number("mix_window") + bonus);
}
const char *MixingMinigame::rhythmName(MixRhythm rhythm) {
    switch (rhythm) {
    case MixRhythm::Steady: return "Nhịp đều";
    case MixRhythm::Fast: return "Nhịp nhanh";
    case MixRhythm::Slow: return "Nhịp chậm";
    case MixRhythm::Wobble: return "Nhịp lắc lư";
    case MixRhythm::Reverse: return "Nhịp đảo chiều";
    }
    return "";
}
void MixingMinigame::update(float dt) {
    if (done())
        return;
    dt = std::max(0.f, dt);
    cooldown_ = std::max(0.f, cooldown_ - dt);
    beatClock_ += dt;
    float scale = speedScale_;
    // Lắc lư: tốc độ dao động theo sóng sin, vạch lúc nhanh lúc chậm.
    if (rhythm_ == MixRhythm::Wobble) scale *= 1.f + .45f * std::sin(beatClock_ * 5.2f);
    // Đảo chiều: tới thời điểm bốc sẵn thì vạch quay đầu bất ngờ (một lần mỗi nhịp).
    if (rhythm_ == MixRhythm::Reverse && reverseAt_ >= 0 && beatClock_ >= reverseAt_) {
        direction_ = -direction_;
        reverseAt_ = -1.f;
    }
    position_ += direction_ * speed_ * scale * dt;
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
void MixingMinigame::nextBeat() {
    std::uniform_real_distribution<float> unit(0.f, 1.f);
    const float roll = unit(random_);
    rhythm_ = roll < .22f ? MixRhythm::Steady : roll < .42f ? MixRhythm::Fast : roll < .58f ? MixRhythm::Slow
            : roll < .80f ? MixRhythm::Wobble : MixRhythm::Reverse;
    // Tốc độ và độ rộng vùng xanh dao động quanh mức chuẩn; nâng cấp muỗng vẫn luôn có tác dụng.
    speedScale_ = rhythm_ == MixRhythm::Fast ? 1.22f + .12f * unit(random_)
                : rhythm_ == MixRhythm::Slow ? .72f + .1f * unit(random_) : .9f + .2f * unit(random_);
    width_ = std::min(data_.number("mix_window_max"), baseWidth_ * (.82f + .3f * unit(random_)));
    beatClock_ = 0;
    reverseAt_ = rhythm_ == MixRhythm::Reverse ? .35f + .5f * unit(random_) : -1.f;
    // Vùng mới luôn nằm trọn trong thanh và cách vùng cũ đủ xa để phải chờ vạch chạy tới.
    const float margin = width_ * .5f + .02f;
    const float old = target_;
    float next = old;
    for (int attempt = 0; attempt < 12 && std::abs(next - old) < .28f; ++attempt)
        next = margin + (1.f - 2.f * margin) * unit(random_);
    if (std::abs(next - old) < .28f) next = old < .5f ? 1.f - margin : margin;
    target_ = next;
}
std::optional<float> MixingMinigame::hit() {
    if (done() || cooldown_ > 0)
        return std::nullopt;
    const float distance = std::abs(position_ - target_);
    const auto key = distance <= width_ * .2f ? "perfect" : (distance <= width_ * .5f ? "good" : "miss");
    const float value = data_.balance.at("mix_scores").at(key).get<float>();
    ++hits_;
    sum_ += value;
    cooldown_ = data_.number("mix_hit_cooldown");
    nextBeat();
    return value;
}
} // namespace tiem

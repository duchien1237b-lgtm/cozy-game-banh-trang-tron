#include "gameplay/Minigames.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
namespace tiem {
PaperTearing::PaperTearing(const GameData &d, float bonus)
    : data_(d), scores_(static_cast<std::size_t>(d.integer("paper_lines")), -1.f),
      samples_(static_cast<std::size_t>(d.integer("paper_samples")), false),
      tolerance_(d.number("paper_tolerance") + bonus) {}
void PaperTearing::begin(sf::Vector2f p) {
    cancel();
    for (int i = 0; i < static_cast<int>(scores_.size()); ++i) {
        if (cut(i))
            continue;
        if (std::abs(p.y - lineY(i)) <= tolerance_ &&
            std::min(std::abs(p.x - 640.f), std::abs(p.x - 840.f)) <= tolerance_) {
            active_ = i;
            previous_ = p;
            move(p);
            return;
        }
    }
}
void PaperTearing::move(sf::Vector2f p) {
    if (active_ < 0)
        return;
    const float distance = std::hypot(p.x - previous_.x, p.y - previous_.y);
    length_ += distance;
    // Lấy mẫu cả đoạn di chuyển để chuột nhanh không bỏ sót những vạch đã đi qua.
    const int steps = std::max(1, static_cast<int>(distance / 4.f));
    for (int i = 0; i <= steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps);
        const auto q = previous_ + (p - previous_) * t;
        const float deviation = std::abs(q.y - lineY(active_));
        deviation_ += deviation;
        ++moves_;
        if (q.x >= 635 && q.x <= 845 && deviation <= tolerance_) {
            const float f = std::clamp((q.x - 640.f) / 200.f, 0.f, .999f);
            samples_[static_cast<std::size_t>(f * static_cast<float>(samples_.size()))] = true;
        }
    }
    previous_ = p;
}
std::optional<float> PaperTearing::release() {
    if (active_ < 0)
        return std::nullopt;
    const float coverage = static_cast<float>(std::count(samples_.begin(), samples_.end(), true)) /
                           static_cast<float>(samples_.size());
    std::optional<float> result;
    if (coverage >= data_.number("paper_min_coverage")) {
        const float alignment =
            std::clamp(1.f - (deviation_ / static_cast<float>(std::max(1, moves_))) / tolerance_, 0.f, 1.f);
        const float timing = std::clamp(1.f - std::abs(elapsed_ - data_.number("paper_ideal_seconds")) /
                                                  (data_.number("paper_ideal_seconds") * 2),
                                        0.f, 1.f);
        const float straight = std::clamp(200.f / std::max(200.f, length_), 0.f, 1.f);
        result = coverage * (.7f * alignment + .2f * timing + .1f * straight);
        scores_[static_cast<std::size_t>(active_)] = *result;
    }
    cancel();
    return result;
}
void PaperTearing::cancel() {
    active_ = -1;
    elapsed_ = deviation_ = length_ = 0;
    moves_ = 0;
    std::fill(samples_.begin(), samples_.end(), false);
}
bool PaperTearing::done() const {
    return std::all_of(scores_.begin(), scores_.end(), [](float x) { return x >= 0; });
}
float PaperTearing::quality() const {
    return done() ? std::accumulate(scores_.begin(), scores_.end(), 0.f) / static_cast<float>(scores_.size())
                  : 0;
}
MixingMinigame::MixingMinigame(const GameData &d, float bonus, int day) : data_(d) {
    speed_ = std::min(d.number("mix_speed_max"),
                      d.number("mix_speed") + static_cast<float>(day - 1) * d.number("mix_speed_per_day"));
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
    const float distance = std::abs(position_ - .5f);
    const auto key = distance <= width_ * .2f ? "perfect" : (distance <= width_ * .5f ? "good" : "miss");
    const float value = data_.balance.at("mix_scores").at(key).get<float>();
    ++hits_;
    sum_ += value;
    cooldown_ = data_.number("mix_hit_cooldown");
    return value;
}
} // namespace tiem

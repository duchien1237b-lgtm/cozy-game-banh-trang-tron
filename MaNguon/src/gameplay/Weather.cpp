#include "gameplay/Weather.hpp"
#include "gameplay/DayClock.hpp"
#include <algorithm>
#include <random>
namespace tiem {
Weather WeatherPlan::at(float dayProgress) const {
    return changes() && dayProgress >= begin && dayProgress < end ? interlude : base;
}
float WeatherPlan::rainShare() const {
    const float span = changes() ? std::clamp(end, 0.f, 1.f) - std::clamp(begin, 0.f, 1.f) : 0.f;
    float share = base == Weather::Rainy ? 1.f - span : 0.f;
    if (interlude == Weather::Rainy) share += span;
    return std::clamp(share, 0.f, 1.f);
}
WeatherPlan WeatherSystem::forecast(std::uint32_t seed, int day) {
    WeatherPlan plan;
    // Ngày đầu luôn nắng đẹp để người chơi mới làm quen.
    if (day <= 1) return plan;
    std::mt19937 rng(seed * 2654435761u ^ static_cast<std::uint32_t>(day) * 40503u ^ 0x5EED0u);
    const float roll = std::uniform_real_distribution<float>(0.f, 1.f)(rng);
    plan.base = roll < .45f ? Weather::Sunny : roll < .70f ? Weather::Cloudy : Weather::Rainy;
    if (std::bernoulli_distribution(.42)(rng)) {
        plan.begin = std::uniform_real_distribution<float>(.18f, .62f)(rng);
        plan.end = std::min(.97f, plan.begin + std::uniform_real_distribution<float>(.14f, .30f)(rng));
        // Trời khô thì có mưa rào; trời mưa thì có lúc tạnh.
        plan.interlude = plan.base == Weather::Rainy
                             ? (std::bernoulli_distribution(.5)(rng) ? Weather::Cloudy : Weather::Sunny)
                             : Weather::Rainy;
    }
    return plan;
}
WeatherEffect WeatherSystem::effect(Weather weather) {
    switch (weather) {
    case Weather::Sunny: return {1.10f, .92f, 0.f, .35f};   // nắng nóng: đông khách, mau sốt ruột, thích đồ tươi mát
    case Weather::Cloudy: return {1.f, 1.f, 0.f, 0.f};
    case Weather::Rainy: return {.72f, 1.18f, .10f, -.30f}; // mưa: vắng khách, khách trú mưa kiên nhẫn và tip thêm
    }
    return {};
}
WeatherEffect WeatherSystem::average(const WeatherPlan &plan) {
    const float span = plan.changes() ? plan.end - plan.begin : 0.f;
    const auto a = effect(plan.base), b = effect(plan.interlude);
    const auto mix = [&](float x, float y) { return x * (1.f - span) + y * span; };
    return {mix(a.spawn, b.spawn), mix(a.patience, b.patience), mix(a.tip, b.tip), mix(a.freshBias, b.freshBias)};
}
const char *WeatherSystem::name(Weather weather) {
    switch (weather) {
    case Weather::Sunny: return "Nắng";
    case Weather::Cloudy: return "Nhiều mây";
    case Weather::Rainy: return "Mưa";
    }
    return "";
}
std::string WeatherSystem::summary(const WeatherPlan &plan) {
    std::string text = name(plan.base);
    if (!plan.changes()) return text + " cả ngày";
    const auto hourAt = [](float fraction) {
        return DayClock::format(DayClock::openHour + fraction * DayClock::hours);
    };
    const std::string span = hourAt(plan.begin) + "–" + hourAt(plan.end);
    if (plan.interlude == Weather::Rainy) return text + ", mưa rào " + span;
    return text + ", tạnh mưa " + span;
}
float WeatherSystem::toppingWeight(const std::string &type, float freshBias) {
    return std::max(.3f, type == "fresh" ? 1.f + freshBias : 1.f - freshBias * .6f);
}
std::string WeatherSystem::advice(const WeatherPlan &plan) {
    const auto avg = average(plan);
    if (avg.freshBias > .2f) return "Trời nóng: khách gọi nhiều xoài, rau, trứng. Nhập thêm đồ tươi.";
    if (avg.freshBias < -.15f) return "Trời mưa: ít khách hơn, chuộng đồ khô và sốt. Đừng nhập quá nhiều đồ tươi.";
    if (plan.changes()) return "Thời tiết đổi giữa ngày: chuẩn bị cân đối cả đồ tươi lẫn đồ khô.";
    return "Trời dịu: lượng khách bình thường, nhập vừa đủ là được.";
}
} // namespace tiem

#include "gameplay/DayClock.hpp"
#include <algorithm>
#include <cmath>
namespace tiem {
DayClock::DayClock(float daySeconds) : daySeconds_(std::max(1.f, daySeconds)) {}
float DayClock::progress(float remaining) const {
    return std::clamp(1.f - remaining / daySeconds_, 0.f, 1.f);
}
float DayClock::hour(float remaining) const {
    return static_cast<float>(openHour) + progress(remaining) * static_cast<float>(hours);
}
int DayClock::slot(float remaining) const {
    return std::clamp(static_cast<int>(progress(remaining) * static_cast<float>(hours)), 0, hours - 1);
}
std::string DayClock::format(float hour) {
    // Làm tròn xuống theo bước 10 phút: đồng hồ chạy đều, không nhảy số liên tục.
    int minutes = static_cast<int>(std::floor(std::clamp(hour, 0.f, 24.f) * 6.f + .0001f)) * 10;
    const int h = minutes / 60, m = minutes % 60;
    return (h < 10 ? "0" : "") + std::to_string(h) + ":" + (m < 10 ? "0" : "") + std::to_string(m);
}
std::string DayClock::slotLabel(int slot) { return std::to_string(openHour + slot) + "h"; }
const char *DayClock::period(float hour) {
    if (hour < 9.f) return "Sáng sớm";
    if (hour < 11.f) return "Buổi sáng";
    if (hour < 13.5f) return "Buổi trưa";
    if (hour < 17.f) return "Buổi chiều";
    if (hour < 18.75f) return "Hoàng hôn";
    return "Buổi tối";
}
float DayClock::trafficFactor(float hour) {
    if (hour < 8.f) return .85f;
    if (hour >= 11.f && hour < 13.f) return 1.2f;  // giờ trưa
    if (hour >= 16.5f && hour < 18.5f) return 1.15f; // tan học, tan ca
    if (hour >= 20.f) return .85f;
    return 1.f;
}
LightState DayClock::light(float hour, float rain) {
    struct Key { float hour; float r, g, b, night, sun; };
    static const Key keys[] = {
        {7.0f, 255, 232, 210, .10f, .55f}, {9.0f, 255, 250, 240, 0.f, .9f}, {12.f, 255, 255, 255, 0.f, 1.f},
        {15.f, 255, 246, 226, 0.f, .9f},   {17.f, 255, 214, 168, .08f, .75f}, {18.5f, 238, 162, 140, .32f, .35f},
        {19.5f, 156, 140, 192, .62f, 0.f}, {21.f, 92, 98, 150, .88f, 0.f}};
    const float h = std::clamp(hour, keys[0].hour, keys[7].hour);
    std::size_t i = 0;
    while (i + 2 < std::size(keys) && h > keys[i + 1].hour) ++i;
    const auto &a = keys[i];
    const auto &b = keys[i + 1];
    const float t = std::clamp((h - a.hour) / (b.hour - a.hour), 0.f, 1.f);
    const auto lerp = [t](float x, float y) { return x + (y - x) * t; };
    float r = lerp(a.r, b.r), g = lerp(a.g, b.g), bl = lerp(a.b, b.b);
    // Mưa làm cảnh xám và tối hơn, che bớt nắng.
    rain = std::clamp(rain, 0.f, 1.f);
    r *= 1.f - .26f * rain;
    g *= 1.f - .20f * rain;
    bl *= 1.f - .12f * rain;
    LightState state;
    state.tint = {static_cast<sf::Uint8>(std::clamp(r, 0.f, 255.f)), static_cast<sf::Uint8>(std::clamp(g, 0.f, 255.f)),
                  static_cast<sf::Uint8>(std::clamp(bl, 0.f, 255.f))};
    state.night = std::clamp(lerp(a.night, b.night) + .12f * rain, 0.f, 1.f);
    state.sun = std::clamp(lerp(a.sun, b.sun) * (1.f - .9f * rain), 0.f, 1.f);
    return state;
}
} // namespace tiem

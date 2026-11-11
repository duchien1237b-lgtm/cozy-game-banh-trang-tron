#pragma once
#include <SFML/Graphics/Color.hpp>
#include <string>
namespace tiem {
// Ánh sáng môi trường tại một thời điểm: màu nhân lên cảnh và độ tối (0 = ban ngày, 1 = đêm).
struct LightState {
    sf::Color tint{255, 255, 255};
    float night{0.f};
    float sun{1.f}; // độ mạnh của nắng (dải nắng, bụi sáng)
};
// Quy đổi thời gian thật của một ngày chơi sang giờ trong game, mở cửa 7:00 và đóng cửa 21:00.
class DayClock {
  public:
    static constexpr int openHour = 7, closeHour = 21, hours = closeHour - openHour;
    explicit DayClock(float daySeconds);
    float progress(float remaining) const; // 0 lúc mở cửa, 1 lúc đóng cửa
    float hour(float remaining) const;     // 7.0 .. 21.0
    int slot(float remaining) const;       // 0 .. hours-1: khung giờ dùng cho thống kê
    static std::string format(float hour); // "07:30"
    static std::string slotLabel(int slot); // "7h"
    static const char *period(float hour); // Sáng sớm, Buổi sáng, Buổi trưa, ...
    // Hệ số lượng khách theo giờ: trưa và tan tầm đông hơn, sáng sớm và tối muộn vắng hơn.
    static float trafficFactor(float hour);
    static LightState light(float hour, float rain);

  private:
    float daySeconds_;
};
} // namespace tiem

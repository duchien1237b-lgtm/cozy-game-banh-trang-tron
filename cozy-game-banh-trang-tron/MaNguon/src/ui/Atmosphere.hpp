#pragma once
#include "gameplay/CustomerMood.hpp"
#include "gameplay/DayClock.hpp"
#include "gameplay/Weather.hpp"
#include "ui/Draw.hpp"
#include <vector>
namespace tiem::ui {
// Khí quyển của cảnh: ánh sáng theo giờ, đèn đêm, mưa rơi và vũng nước.
// Lớp giữ hạt mưa (có trạng thái) nên mỗi màn chơi sở hữu một đối tượng riêng.
class Atmosphere {
  public:
    // rain: 0..1, tăng/giảm mượt khi thời tiết đổi để mưa không bật/tắt đột ngột.
    void update(float dt, Weather weather, float hour);
    float rain() const { return rain_; }
    float cloud() const { return cloud_; }
    LightState light() const { return DayClock::light(hour_, std::max(rain_, cloud_ * .45f)); }
    // Phủ màu ánh sáng (nhân) lên vùng cảnh; dùng sau khi vẽ nền và trước khi vẽ giao diện.
    void shade(sf::RenderTarget &, sf::FloatRect area, float strength = 1.f) const;
    // Đèn đường, đèn cửa hiệu và quầng đèn lồng khi trời tối.
    void nightLights(sf::RenderTarget &, float time, bool lanterns) const;
    void drawRain(sf::RenderTarget &, sf::FloatRect area) const;
    void drawSplashes(sf::RenderTarget &) const;

  private:
    struct Drop { sf::Vector2f position; float speed, length; };
    struct Splash { sf::Vector2f position; float age; };
    std::vector<Drop> drops_;
    std::vector<Splash> splashes_;
    float rain_{}, cloud_{}, hour_{7.f};
    unsigned seed_{0x9E3779B9u};
    float random();
};
// Biểu tượng thời tiết vẽ bằng hình khối (nắng có tia, mây, mây mưa).
void weatherIcon(sf::RenderTarget &, Weather, sf::Vector2f center, float size, float time);
// Mặt đồng hồ nhỏ chỉ giờ trong game.
void clockIcon(sf::RenderTarget &, sf::Vector2f center, float radius, float hour, sf::Color face, sf::Color ink);
// Biểu tượng cảm xúc vẽ cạnh đầu khách: tim, nốt nhạc, giọt mồ hôi, gân tức, nước mắt, "...", "!".
void emote(sf::RenderTarget &, const sf::Font &, Expression, sf::Vector2f anchor, float time, float scale = 1.f);
// Độ lệch chuyển động của khách theo biểu cảm (nhún, lắc, rũ xuống...).
sf::Vector2f expressionMotion(Expression, float time, unsigned seed);
} // namespace tiem::ui

#pragma once
#include <SFML/Graphics.hpp>
#include <string_view>

namespace tiem::ui {
struct Theme {
    sf::Color cream{255, 248, 230};
    sf::Color ink{81, 54, 44};
    sf::Color muted{133, 104, 86};
    sf::Color green{79, 111, 76};
    sf::Color peach{239, 166, 124};
    sf::Color paper{255, 254, 246};
    sf::Color gold{226, 168, 58};
    sf::Color orange{237, 126, 52};
    sf::Color red{196, 84, 60};
    sf::Color wood{139, 89, 49};
};
void panel(sf::RenderTarget &target, sf::FloatRect rect, sf::Color color, float radius = 16.f);
void text(sf::RenderTarget &target, const sf::Font &font, std::string_view label, unsigned int size,
          sf::Vector2f position, sf::Color color, bool centered = false);
void backdrop(sf::RenderTarget &target);
// Chữ tự thu nhỏ để vừa chiều rộng cho phép (không nhỏ hơn minSize).
void textFit(sf::RenderTarget &target, const sf::Font &font, std::string_view label, unsigned int size,
             sf::Vector2f position, sf::Color color, float maxWidth, bool centered = false,
             unsigned int minSize = 9);
// Chữ có viền, dùng cho banner/điểm số nổi trên cảnh.
void outlinedText(sf::RenderTarget &target, const sf::Font &font, std::string_view label, unsigned int size,
                  sf::Vector2f center, sf::Color fill, sf::Color outline, float thickness = 2.f);
// Đoạn văn tự xuống dòng; trả về số dòng đã vẽ.
std::size_t paragraph(sf::RenderTarget &target, const sf::Font &font, std::string_view label, unsigned int size,
                      sf::Vector2f position, float width, float lineHeight, sf::Color color,
                      bool centered = false);
float textWidth(const sf::Font &font, std::string_view label, unsigned int size);
// Bảng có bóng đổ mềm phía dưới, tạo cảm giác nổi khỏi nền.
void shadowPanel(sf::RenderTarget &target, sf::FloatRect rect, sf::Color color, float radius = 16.f,
                 float offset = 6.f, sf::Uint8 shadowAlpha = 55);
void panelOutline(sf::RenderTarget &target, sf::FloatRect rect, sf::Color color, float radius, float thickness);
void gradientRect(sf::RenderTarget &target, sf::FloatRect rect, sf::Color top, sf::Color bottom);
// Quầng sáng mềm (hòa trộn cộng) hoặc bóng mờ (hòa trộn thường).
void radial(sf::RenderTarget &target, sf::Vector2f center, sf::Vector2f radius, sf::Color inner,
            sf::Color outer = {0, 0, 0, 0}, bool additive = false);
// Làm tối nhẹ các cạnh màn hình để tập trung vào giữa.
void vignette(sf::RenderTarget &target, sf::Uint8 strength = 70);
// Phím tắt dạng keycap nhỏ.
void keycap(sf::RenderTarget &target, const sf::Font &font, std::string_view label, sf::Vector2f center);
void checkMark(sf::RenderTarget &target, sf::Vector2f center, float radius, sf::Color background,
               sf::Color mark = {255, 255, 255});
// Bụi nắng bay chậm: không giữ trạng thái, chỉ phụ thuộc thời gian.
void dust(sf::RenderTarget &target, sf::FloatRect area, float time, int count = 26, sf::Uint8 alpha = 90);
// Dải nắng chéo hòa trộn cộng, vẽ lên cảnh đường phố.
void lightShafts(sf::RenderTarget &target, sf::FloatRect area, float time, sf::Uint8 alpha = 28);
// Hơi nóng bốc lên từ một điểm (trạng thái từ thời gian).
void steam(sf::RenderTarget &target, sf::Vector2f base, float time, float intensity = 1.f, float height = 90.f);
} // namespace tiem::ui

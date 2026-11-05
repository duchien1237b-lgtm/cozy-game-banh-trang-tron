#pragma once
#include <cstdint>
#include <string>
namespace tiem {
// Thời tiết trong ngày. Lớp chỉ chứa dữ liệu/luật chơi, không phụ thuộc SFML hay giao diện.
enum class Weather { Sunny, Cloudy, Rainy };
// Hệ số tác động lên một ngày bán hàng.
struct WeatherEffect {
    float spawn{1.f};     // tốc độ khách đến (nhân)
    float patience{1.f};  // kiên nhẫn của khách (nhân)
    float tip{0.f};       // tip cộng thêm theo % tiền món
    float freshBias{0.f}; // >0: khách thích đồ tươi; <0: thích đồ khô/sốt
};
// Kế hoạch thời tiết một ngày: thời tiết nền và (có thể) một đợt xen giữa, ví dụ mưa rào buổi chiều.
struct WeatherPlan {
    Weather base{Weather::Sunny};
    Weather interlude{Weather::Rainy};
    float begin{-1.f}, end{-1.f}; // tỉ lệ thời gian của ngày [0, 1]; < 0 nếu không có đợt xen
    bool changes() const { return begin >= 0.f && end > begin; }
    Weather at(float dayProgress) const;
    // Tỉ lệ thời gian trong ngày trời mưa (0..1), dùng để dự báo lượng khách.
    float rainShare() const;
};
class WeatherSystem {
  public:
    // Dự báo cố định theo (seed của bản lưu, số ngày): màn chuẩn bị và màn chơi luôn thấy cùng một kết quả.
    static WeatherPlan forecast(std::uint32_t seed, int day);
    static WeatherEffect effect(Weather);
    // Hệ số trung bình cả ngày, dùng cho gợi ý nhập hàng.
    static WeatherEffect average(const WeatherPlan &);
    static const char *name(Weather);
    static std::string summary(const WeatherPlan &);
    static std::string advice(const WeatherPlan &);
    // Trọng số chọn topping theo loại ("fresh"/"dry"/"sauce") khi khách gọi món dưới thời tiết có freshBias.
    static float toppingWeight(const std::string &type, float freshBias);
};
} // namespace tiem

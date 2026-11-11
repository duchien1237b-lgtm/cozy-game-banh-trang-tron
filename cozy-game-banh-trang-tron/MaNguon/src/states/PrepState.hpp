#pragma once
#include "states/UiState.hpp"
#include "ui/Effects.hpp"
#include "gameplay/Weather.hpp"
#include <map>

namespace tiem {
// Màn chuẩn bị đầu ngày: xem dự báo thời tiết, nhập nguyên liệu vào kho rồi mới mở quán.
class PrepState final : public UiState {
  public:
    PrepState(StateStack &, AppContext &);
    bool handleEvent(const sf::Event &, Pointer) override;
    bool update(float dt) override;
    void render(sf::RenderTarget &) override;

  private:
    void rebuild();
    void adjust(const std::string &id, int delta);
    void commit();
    sf::FloatRect cardRect(std::size_t slot) const;
    int cartCost() const;
    std::vector<std::size_t> open_;          // chỉ số topping đã mở khóa
    std::map<std::string, int> cart_;        // số muỗng định nhập thêm
    std::map<std::string, int> demand_;      // nhu cầu ước tính theo thời tiết
    WeatherPlan plan_;
    std::size_t page_{};
    bool dirty_{false};
    float age_{};
    Pointer pointer_;
    ui::Effects effects_;
};
} // namespace tiem

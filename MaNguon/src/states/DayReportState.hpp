#pragma once
#include "states/UiState.hpp"
#include "ui/Effects.hpp"

namespace tiem {
// Bảng tổng kết cuối ngày (thiết kế mới): xếp hạng ngày, sổ thu chi có lãi ròng,
// biểu đồ khách theo giờ 7h–21h kèm thời tiết, nhiệm vụ, khách nổi bật và thành tựu.
class DayReportState final : public UiState {
  public:
    DayReportState(StateStack &, AppContext &);
    bool handleEvent(const sf::Event &, Pointer) override;
    bool update(float dt) override;
    void render(sf::RenderTarget &) override;

  private:
    float count(float delay, float seconds = .9f) const;
    void drawGrade(sf::RenderTarget &, sf::FloatRect);
    void drawLedger(sf::RenderTarget &, sf::FloatRect);
    void drawTimeline(sf::RenderTarget &, sf::FloatRect);
    void drawSide(sf::RenderTarget &, sf::FloatRect);
    float age_{};
    unsigned starsPopped_{};
    bool celebrated_{false};
    Pointer pointer_;
    ui::Effects effects_;
};
} // namespace tiem

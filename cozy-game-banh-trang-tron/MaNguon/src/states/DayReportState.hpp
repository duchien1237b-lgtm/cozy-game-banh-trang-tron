#pragma once
#include "states/UiState.hpp"
#include "ui/Effects.hpp"

namespace tiem {
// Ba khung đồng bộ: xếp hạng, sổ thu chi, nhiệm vụ và khách nổi bật.
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
    void drawSide(sf::RenderTarget &, sf::FloatRect);
    float age_{};
    unsigned starsPopped_{};
    bool celebrated_{false};
    Pointer pointer_;
    ui::Effects effects_;
};
} // namespace tiem

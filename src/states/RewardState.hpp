#pragma once
#include "states/UiState.hpp"
#include "ui/Effects.hpp"

namespace tiem {
// Danh sách thành tựu: đã mở, tiến độ các thành tựu còn lại.
class AchievementsState final : public UiState {
  public:
    AchievementsState(StateStack &, AppContext &);
    bool handleEvent(const sf::Event &, Pointer) override;
    bool update(float dt) override;
    void render(sf::RenderTarget &) override;
    bool transparent() const override { return true; }

  private:
    float age_{};
};

// Quà đăng nhập 7 ngày: nhận một lần mỗi ngày, chuỗi liên tiếp tăng mốc thưởng.
class DailyRewardState final : public UiState {
  public:
    DailyRewardState(StateStack &, AppContext &);
    bool handleEvent(const sf::Event &, Pointer) override;
    bool update(float dt) override;
    void render(sf::RenderTarget &) override;
    bool transparent() const override { return true; }

  private:
    void claim();
    std::int64_t today_{};
    float age_{};
    bool claimed_{false};
    int reward_{};
    ui::Effects effects_;
};
} // namespace tiem

#pragma once
#include "states/UiState.hpp"
#include "ui/Effects.hpp"
namespace tiem {
// State tổng kết và cửa hàng chia sẻ context; không còn là màn mô phỏng.
class ManagementState final : public UiState {
  public:
    ManagementState(StateStack &, AppContext &, StateID);
    bool handleEvent(const sf::Event &, Pointer) override;
    bool update(float dt) override;
    void render(sf::RenderTarget &) override;

  private:
    void rebuild();
    void drawResult(sf::RenderTarget &);
    void drawShop(sf::RenderTarget &);
    StateID kind_;
    bool ingredients_{false}, dirty_{false};
    std::size_t page_{};
    ui::Effects effects_;
};
} // namespace tiem

#pragma once
#include "states/UiState.hpp"
#include "ui/Effects.hpp"
#include <deque>

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
    float count(float delay, float seconds = .9f) const;
    sf::FloatRect cardRect(std::size_t slot) const;
    StateID kind_;
    bool ingredients_{false}, dirty_{false}, celebrated_{false};
    unsigned starsPopped_{};
    std::size_t page_{};
    float age_{};
    Pointer pointer_;
    ui::Effects effects_;
    struct Toast { std::string id; float age{}; };
    std::deque<Toast> toasts_;
};
} // namespace tiem

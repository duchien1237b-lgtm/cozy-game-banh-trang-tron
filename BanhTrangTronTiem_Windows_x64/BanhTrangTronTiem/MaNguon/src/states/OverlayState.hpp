#pragma once
#include "states/UiState.hpp"
namespace tiem {
class OverlayState final : public UiState {
  public:
    OverlayState(StateStack &, AppContext &, StateID);
    bool handleEvent(const sf::Event &, Pointer) override;
    void render(sf::RenderTarget &) override;
    bool transparent() const override {
        return true;
    }

  private:
    StateID kind_;
    int slider_{-1};
    bool confirmExit_{false};
};
} // namespace tiem

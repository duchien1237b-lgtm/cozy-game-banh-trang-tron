#pragma once
#include "states/UiState.hpp"
namespace tiem {
class MainMenuState final : public UiState {
  public:
    MainMenuState(StateStack &, AppContext &);
    bool handleEvent(const sf::Event &, Pointer) override;
    bool update(float dt) override;
    void render(sf::RenderTarget &) override;

  private:
    bool help_{false};
    float time_{};
};
} // namespace tiem

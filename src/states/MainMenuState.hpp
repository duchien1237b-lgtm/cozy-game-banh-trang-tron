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
    bool confirmReset_{false};
    std::vector<ui::Button> resetButtons_;
    float time_{};
};
} // namespace tiem

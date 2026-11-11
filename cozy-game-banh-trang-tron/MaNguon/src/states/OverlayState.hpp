#pragma once
#include "states/UiState.hpp"

namespace tiem {
class OverlayState final : public UiState {
  public:
    OverlayState(StateStack &, AppContext &, StateID);
    ~OverlayState() override;
    void onCovered() override;
    bool handleEvent(const sf::Event &, Pointer) override;
    bool update(float dt) override;
    void render(sf::RenderTarget &) override;
    bool transparent() const override {
        return true;
    }

  private:
    void refreshLabels();
    StateID kind_;
    int slider_{-1};
    bool confirmExit_{false};
    bool help_{false};
    float age_{};
    std::unique_ptr<ui::Button> helpBack_;
    void refreshKeyLabels();
    KeyBindings keyDraft_;
    int waitingKey_{-1};
    std::string keyNotice_;
    std::vector<ui::Button> keyButtons_;
    std::unique_ptr<ui::Button> keyReset_, keySave_;
};
} // namespace tiem

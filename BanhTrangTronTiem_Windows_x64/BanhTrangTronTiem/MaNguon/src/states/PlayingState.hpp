#pragma once
#include "gameplay/Minigames.hpp"
#include "states/UiState.hpp"
#include "ui/Effects.hpp"
namespace tiem {
class PlayingState final : public UiState {
  public:
    PlayingState(StateStack &, AppContext &);
    bool handleEvent(const sf::Event &, Pointer) override;
    bool update(float dt) override;
    void render(sf::RenderTarget &) override;
    void onCovered() override;

  private:
    enum class Stage { Paper, Toppings, Mixing, Ready };
    void resetBowl();
    void startMix();
    void hitMix();
    void serve(std::size_t index);
    void addPortion(const std::string &id);
    void refreshButtons();
    void cancelDrag();
    std::optional<std::size_t> selectedIndex();
    sf::FloatRect tray(std::size_t slot) const;
    sf::FloatRect customerCard(std::size_t index) const;
    bool overBowl(sf::Vector2f) const;
    void drawCustomers(sf::RenderTarget &);
    void drawWorktop(sf::RenderTarget &);
    DaySession day_;
    Bowl bowl_;
    std::unique_ptr<PaperTearing> paper_;
    std::unique_ptr<MixingMinigame> mix_;
    ui::Effects effects_;
    Stage stage_{Stage::Paper};
    std::vector<std::size_t> available_;
    std::optional<std::size_t> dragging_;
    Pointer pointer_;
    unsigned selectedId_{};
    std::size_t page_{};
    bool bowlDrag_{false}, poured_{false}, finished_{false};
    float pourTime_{}, animation_{};
    std::string message_{"Giữ chuột ở đầu vạch, vuốt theo 3 đường nét đứt."};
};
} // namespace tiem

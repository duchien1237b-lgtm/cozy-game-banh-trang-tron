#pragma once
#include "gameplay/Minigames.hpp"
#include "states/UiState.hpp"
#include "ui/Effects.hpp"
#include "ui/CookingEffects.hpp"
#include <deque>
namespace tiem {
class PlayingState final : public UiState {
  public:
    PlayingState(StateStack &, AppContext &);
    ~PlayingState() override;
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
    void addPortions(const std::string &id, int count);
    void refreshButtons();
    void cancelDrag();
    void cancelCharge();
    std::optional<std::size_t> selectedIndex();
    sf::FloatRect tray(std::size_t slot) const;
    sf::FloatRect customerArea(std::size_t index) const;
    bool hasDecorations() const;
    float bowlScale() const;
    bool overBowl(sf::Vector2f) const;
    void drawCustomers(sf::RenderTarget &);
    void drawWorktop(sf::RenderTarget &);
    DaySession day_;
    Bowl bowl_;
    std::unique_ptr<PaperTearing> paper_;
    std::unique_ptr<MixingMinigame> mix_;
    ui::Effects effects_;
    ui::CookingEffects cookingFx_;
    std::deque<float> mixTrail_;
    float tearFxClock_{};
    ToppingCharge charge_;
    struct ReviewToast { std::string customer; Receipt receipt; float remaining{3.5f}; };
    std::deque<ReviewToast> reviews_;
    struct IngredientDrop { std::string id; float age; int index; bool popped{false}; bool landed{false}; };
    std::vector<IngredientDrop> drops_;
    float selectionAge_{1.f};
    std::deque<float> happySounds_;
    Stage stage_{Stage::Paper};
    std::vector<std::size_t> available_;
    std::optional<std::size_t> selectedTopping_;
    Pointer pointer_;
    unsigned selectedId_{};
    std::size_t page_{};
    bool bowlDrag_{false}, finished_{false};
    float animation_{};
    std::string message_{"Giữ và rê qua lại trên bánh; thời gian mỗi giai đoạn hiển thị dưới thớt."};
};
} // namespace tiem

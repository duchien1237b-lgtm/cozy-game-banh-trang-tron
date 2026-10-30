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
    void undoPortion();
    void primaryAction();
    void selectTray(std::size_t slot);
    void cycleCustomer(int direction);
    void startDay();
    void refreshButtons();
    void cancelDrag();
    void cancelCharge();
    void say(std::string text);
    void banner(std::string title, std::string sub, sf::Color color, float life = 2.4f);
    void collectAchievements();
    std::string stageHint() const;
    int tutorialStep() const;
    std::optional<std::size_t> selectedIndex();
    sf::FloatRect tray(std::size_t slot) const;
    sf::FloatRect customerArea(std::size_t index) const;
    bool hasDecorations() const;
    float bowlScale() const;
    bool toppingsComplete();
    bool overBowl(sf::Vector2f) const;
    void drawHud(sf::RenderTarget &);
    void drawCustomers(sf::RenderTarget &);
    void drawWorktop(sf::RenderTarget &);
    void drawBowlPanel(sf::RenderTarget &);
    void drawHint(sf::RenderTarget &);
    void drawTutorial(sf::RenderTarget &);
    void drawOverlays(sf::RenderTarget &);
    void drawBriefing(sf::RenderTarget &);
    void drawReviewCard(sf::RenderTarget &);

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
    struct Banner { std::string title, sub; sf::Color color; float age{}, life{}; };
    std::deque<Banner> banners_;
    struct CoinFly { sf::Vector2f from; float age, delay; };
    std::vector<CoinFly> coins_;
    struct AchievementToast { std::string id; float age{}; };
    std::deque<AchievementToast> toasts_;
    float selectionAge_{1.f};
    struct HappySound { float delay; std::string event; };
    std::deque<HappySound> happySounds_;
    Stage stage_{Stage::Paper};
    std::vector<std::size_t> available_;
    std::optional<std::size_t> selectedTopping_;
    Pointer pointer_;
    unsigned selectedId_{};
    std::size_t page_{};
    bool bowlDrag_{false}, finished_{false};
    // Màn chuẩn bị đầu ngày: đồng hồ và khách chưa chạy cho tới khi người chơi bấm Mở cửa.
    bool briefing_{true};
    float briefingAge_{};
    std::unique_ptr<ui::Button> startButton_, skipTutorial_;
    bool tutorial_{false}, tutorialPressed_{false};
    bool lastRush_{false}, warned20_{false};
    int lastCombo_{}, lastSecond_{-1};
    float walletShown_{}, walletBump_{};
    float animation_{}, messageAge_{};
    std::string message_{"Giữ và rê qua lại trên bánh để xé."};
};
} // namespace tiem

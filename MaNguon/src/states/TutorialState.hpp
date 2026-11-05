#pragma once
#include "gameplay/Minigames.hpp"
#include "states/UiState.hpp"
#include "ui/CookingEffects.hpp"
#include "ui/Effects.hpp"
#include <memory>

namespace tiem {
// Hướng dẫn cách chơi cho người mới, hiện trước ngày đầu tiên (xem lại được từ menu).
// Gồm 7 trang; ba trang thao tác (xé, thêm topping, trộn) dùng đúng các lớp minigame của màn chơi
// để người chơi tập thử trước khi vào ngày thật.
class TutorialState final : public UiState {
  public:
    TutorialState(StateStack &, AppContext &);
    ~TutorialState() override;
    bool handleEvent(const sf::Event &, Pointer) override;
    bool update(float dt) override;
    void render(sf::RenderTarget &) override;
    void onCovered() override;
    int page() const { return page_; }
    static constexpr int pageCount = 7;

  private:
    void go(int page);
    void finish();
    void resetPractice();
    void drawWelcome(sf::RenderTarget &);
    void drawCustomers(sf::RenderTarget &);
    void drawPaper(sf::RenderTarget &);
    void drawToppings(sf::RenderTarget &);
    void drawMixing(sf::RenderTarget &);
    void drawScoring(sf::RenderTarget &);
    void drawDay(sf::RenderTarget &);
    void heading(sf::RenderTarget &, const std::string &title, const std::string &sub);
    void bullets(sf::RenderTarget &, const std::vector<std::string> &, sf::Vector2f, float width);
    bool practiceDone() const;
    int page_{};
    float age_{}, pageAge_{};
    Pointer pointer_;
    std::unique_ptr<PaperTearing> paper_;
    std::unique_ptr<MixingMinigame> mix_;
    ToppingCharge charge_;
    Bowl bowl_;
    int lastMix_{-1};
    float lastMixValue_{};
    unsigned mixSeed_{0xA11CEu};
    ui::Effects effects_;
    ui::CookingEffects cookingFx_;
};
} // namespace tiem

#pragma once
#include "states/UiState.hpp"
#include <cstdint>
#include <optional>

namespace tiem {
class ReviewsState final : public UiState {
  public:
    ReviewsState(StateStack &, AppContext &);
    bool handleEvent(const sf::Event &, Pointer) override;
    bool update(float dt) override;
    void render(sf::RenderTarget &) override;
    bool transparent() const override { return true; }
    void onCovered() override;

  private:
    std::vector<const ShopReview *> filtered() const;
    ShopReview *editingReview();
    void rebuildButtons();
    void openEditor(std::uint64_t id);
    void closeEditor();
    void submitReply();
    void appendInput(const sf::String &);
    void drawEditor(sf::RenderTarget &);
    std::size_t page_{};
    bool unansweredOnly_{}, lowStarsOnly_{}, rebuild_{}, inputFocused_{}, selectAll_{};
    std::size_t cursor_{};
    std::optional<std::uint64_t> editing_;
    sf::String draft_;
    std::string notice_;
    bool noticeError_{};
    float blink_{};
    std::size_t detailScroll_{};
};
} // namespace tiem

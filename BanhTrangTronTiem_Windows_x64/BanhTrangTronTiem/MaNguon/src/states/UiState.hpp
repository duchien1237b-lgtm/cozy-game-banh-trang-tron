#pragma once
#include "core/AudioManager.hpp"
#include "core/ResourceManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/StateStack.hpp"
#include "ui/Button.hpp"
#include <utility>
#include <vector>

namespace tiem {
class UiState : public State {
  public:
    using State::State;
    bool update(float dt) override {
        for (auto &button : buttons_)
            button.update(dt);
        return false;
    }
    void onCovered() override {
        for (auto &button : buttons_)
            button.cancelInteraction();
    }

  protected:
    GameServices &game() const {
        return *context_.game;
    }
    void sound(const std::string &name) {
        if (context_.audio)
            context_.audio->play(name);
    }
    ui::Button &addButton(std::string label, sf::FloatRect rect, ui::Button::Callback callback) {
        buttons_.emplace_back(context_.resources.uiFont(), std::move(label), rect, std::move(callback));
        buttons_.back().setClickSound(context_.playClick);
        return buttons_.back();
    }
    void handleButtons(const sf::Event &event, Pointer mouse) {
        for (auto &button : buttons_)
            button.handleEvent(event, mouse);
    }
    void renderButtons(sf::RenderTarget &target) const {
        for (const auto &button : buttons_)
            button.render(target);
    }
    void returnToMenu() {
        stack_.clear();
        stack_.push(StateID::MainMenu);
    }
    std::vector<ui::Button> buttons_;
};
} // namespace tiem

#pragma once
#include "core/State.hpp"
#include "core/ResourceManager.hpp"
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include <utility>

namespace tiem::ui {
class Button {
  public:
    using Callback = std::function<void()>;
    Button(const sf::Font &font, std::string label, sf::FloatRect bounds, Callback onClick);
    void setClickSound(Callback hook) {
        clickSound_ = std::move(hook);
    }
    void setResources(ResourceManager &resources) { resources_ = &resources; }
    void setIcon(std::string path) { iconPath_ = std::move(path); }
    void setEnabled(bool enabled);
    void setLabel(std::string label) {
        label_ = std::move(label);
    }
    bool enabled() const {
        return enabled_;
    }
    void cancelInteraction();
    void handleEvent(const sf::Event &event, Pointer mouse);
    void update(float dt);
    void render(sf::RenderTarget &target) const;

  private:
    ResourceManager *resources_{nullptr};
    const sf::Font &font_;
    std::string label_;
    std::string iconPath_;
    sf::FloatRect bounds_;
    Callback onClick_;
    Callback clickSound_;
    bool enabled_{true};
    bool hovered_{false};
    bool pressed_{false};
    float lift_{0.f};
};
} // namespace tiem::ui

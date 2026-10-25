#pragma once
#include "core/AppConfig.hpp"
#include "core/AudioManager.hpp"
#include "core/ResourceManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/StateStack.hpp"

namespace tiem {
class Game {
  public:
    Game();
    void run();

  private:
    void registerStates();
    void resize(unsigned int width, unsigned int height);
    Pointer mouseFor(const sf::Event &event) const;
    void handleEvents();
    std::filesystem::path root_;
    AppConfig config_;
    sf::RenderWindow window_;
    sf::View view_;
    // Thứ tự hủy ngược: state/sound hủy trước tài nguyên, cửa sổ hủy cuối.
    ResourceManager resources_;
    GameServices services_;
    AudioManager audio_;
    AppContext context_;
    StateStack states_;
    bool focused_{true};
};
} // namespace tiem

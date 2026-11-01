#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <optional>

namespace tiem {
class ResourceManager;
struct AppConfig;
class StateStack;
struct GameServices;
class AudioManager;
enum class StateID { MainMenu, Playing, DayResult, UpgradeShop, Pause, Settings, Reviews, Achievements, DailyReward };
using Pointer = std::optional<sf::Vector2f>;

struct AppContext {
    sf::RenderWindow &window;
    ResourceManager &resources;
    const AppConfig &config;
    std::function<void()> playClick;
    GameServices *game{nullptr};
    AudioManager *audio{nullptr};
    // Bật/tắt toàn màn hình (Game xử lý việc tạo lại cửa sổ).
    std::function<void(bool)> setFullscreen;
};

class State {
  public:
    State(StateStack &stack, AppContext &context) : stack_(stack), context_(context) {}
    virtual ~State() = default;
    // true: truyền xuống state dưới; false: chặn (Pause chặn update gameplay).
    virtual bool handleEvent(const sf::Event &event, Pointer mouse) = 0;
    virtual bool update(float dt) = 0;
    virtual void render(sf::RenderTarget &target) = 0;
    virtual bool transparent() const {
        return false;
    }
    virtual void onCovered() {}

  protected:
    StateStack &stack_;
    AppContext &context_;
};
} // namespace tiem

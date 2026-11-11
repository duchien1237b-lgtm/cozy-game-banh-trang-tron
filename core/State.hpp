
// Khai báo khung mẫu chuẩn cho mọi màn hình trong game,  chứa tài nguyên dùng chung
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

// dlieu tọa độ chuột
using Pointer = std::optional<sf::Vector2f>;
struct AppContext {
    sf::RenderWindow &window;
    ResourceManager &resources;
    const AppConfig &config;
    std::function<void()> playClick;
    GameServices *game{nullptr};
    AudioManager *audio{nullptr};
    std::function<void(bool)> setFullscreen; // Hàm bật,tắt toàn màn hình
};
class State {
  public:
    State(StateStack &stack, AppContext &context) : stack_(stack), context_(context) {}
    virtual ~State() = default;
    virtual bool handleEvent(const sf::Event &event, Pointer mouse) = 0; // Xử lý chuột/phím
    virtual bool update(float dt) = 0;                                    // Cập nhật logic theo thời gian
    virtual void render(sf::RenderTarget &target) = 0;                   // Vẽ giao diện màn hình
    virtual bool transparent() const {
        return false;
    }
    virtual void onCovered() {}

  protected:
    StateStack &stack_;
    AppContext &context_; 
};
}

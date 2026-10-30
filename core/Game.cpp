#include "core/Game.hpp"
#include "core/Paths.hpp"
#include "core/Utf8.hpp"
#include "states/MainMenuState.hpp"
#include "states/ManagementState.hpp"
#include "states/OverlayState.hpp"
#include "states/PlayingState.hpp"
#include "states/RewardState.hpp"
#include "states/ReviewsState.hpp"
#include <algorithm>

namespace tiem {
Game::Game()
    : root_(findDataRoot()), config_(AppConfig::load(root_ / "data/app.json")),
      window_(sf::VideoMode(config_.width, config_.height), utf8(config_.title), sf::Style::Default,
              sf::ContextSettings(0, 0, 4)),
      view_(sf::FloatRect(0, 0, 1280, 720)), resources_(root_, config_.font), services_(root_),
      audio_(resources_, root_),
      context_{window_, resources_, config_, [this] { audio_.play("ui_click"); }, &services_, &audio_,
               [this](bool on) { applyWindowMode(on); }} {
    window_.setFramerateLimit(config_.fpsLimit);
    window_.setKeyRepeatEnabled(false);
    if (const auto *texture = resources_.texture("assets/images/cozy/ui/game_icon.png")) {
        const sf::Image icon = texture->copyToImage();
        const auto size = icon.getSize();
        window_.setIcon(size.x, size.y, icon.getPixelsPtr());
    }
    resources_.uiFont();
    audio_.update(services_.progress.music, services_.progress.sfx);
    resize(config_.width, config_.height);
    registerStates();
    states_.push(StateID::MainMenu);
    states_.applyPendingChanges();
    if (services_.progress.fullscreen)
        applyWindowMode(true);
}

void Game::applyWindowMode(bool fullscreen) {
    if (fullscreen == fullscreen_)
        return;
    fullscreen_ = fullscreen;
    services_.progress.fullscreen = fullscreen;
    const sf::ContextSettings settings(0, 0, 4);
    if (fullscreen)
        window_.create(sf::VideoMode::getDesktopMode(), utf8(config_.title), sf::Style::Fullscreen, settings);
    else
        window_.create(sf::VideoMode(config_.width, config_.height), utf8(config_.title), sf::Style::Default, settings);
    window_.setFramerateLimit(config_.fpsLimit);
    window_.setKeyRepeatEnabled(false);
    if (const auto *texture = resources_.texture("assets/images/cozy/ui/game_icon.png")) {
        const sf::Image icon = texture->copyToImage();
        const auto size = icon.getSize();
        window_.setIcon(size.x, size.y, icon.getPixelsPtr());
    }
    resize(window_.getSize().x, window_.getSize().y);
}

void Game::registerStates() {
    states_.registerFactory(StateID::MainMenu,
                            [this] { return std::make_unique<MainMenuState>(states_, context_); });
    states_.registerFactory(StateID::Playing,
                            [this] { return std::make_unique<PlayingState>(states_, context_); });
    states_.registerFactory(StateID::Reviews,
                            [this] { return std::make_unique<ReviewsState>(states_, context_); });
    states_.registerFactory(StateID::Achievements,
                            [this] { return std::make_unique<AchievementsState>(states_, context_); });
    states_.registerFactory(StateID::DailyReward,
                            [this] { return std::make_unique<DailyRewardState>(states_, context_); });
    for (const auto id : {StateID::Pause, StateID::Settings})
        states_.registerFactory(id,
                                [this, id] { return std::make_unique<OverlayState>(states_, context_, id); });
    for (const auto id : {StateID::DayResult, StateID::UpgradeShop})
        states_.registerFactory(
            id, [this, id] { return std::make_unique<ManagementState>(states_, context_, id); });
}

void Game::resize(unsigned int width, unsigned int height) {
    if (width == 0 || height == 0)
        return;
    const float ratio = static_cast<float>(width) / static_cast<float>(height);
    const float logicalRatio = view_.getSize().x / view_.getSize().y;
    sf::FloatRect viewport(0, 0, 1, 1);
    if (ratio > logicalRatio) {
        viewport.width = logicalRatio / ratio;
        viewport.left = (1.f - viewport.width) / 2.f;
    } else {
        viewport.height = ratio / logicalRatio;
        viewport.top = (1.f - viewport.height) / 2.f;
    }
    view_.setViewport(viewport);
    window_.setView(view_);
}

Pointer Game::mouseFor(const sf::Event &event) const {
    auto pixel = sf::Mouse::getPosition(window_);
    if (event.type == sf::Event::MouseMoved)
        pixel = {event.mouseMove.x, event.mouseMove.y};
    else if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased)
        pixel = {event.mouseButton.x, event.mouseButton.y};
    // Vùng viền letterbox không được tương tác với nội dung game.
    if (!window_.getViewport(view_).contains(pixel))
        return std::nullopt;
    return window_.mapPixelToCoords(pixel, view_);
}
void Game::handleEvents() {
    sf::Event event{};
    while (window_.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window_.close();
            break;
        }
        if (event.type == sf::Event::Resized)
            resize(event.size.width, event.size.height);
        // F11 bật/tắt toàn màn hình ở mọi màn hình.
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F11) {
            applyWindowMode(!fullscreen_);
            services_.save();
            continue;
        }
        if (event.type == sf::Event::LostFocus)
            focused_ = false;
        if (event.type == sf::Event::GainedFocus)
            focused_ = true;
        states_.handleEvent(event, focused_ ? mouseFor(event) : std::nullopt);
        if (!window_.isOpen())
            break;
    }
}
void Game::run() {
    sf::Clock clock;
    unsigned lastScene = states_.sceneChanges();
    float fade = 1.f;
    while (window_.isOpen()) {
        // Giới hạn bước mô phỏng sau khi kéo cửa sổ hoặc bị hệ điều hành ngắt lâu.
        const float dt = std::min(clock.restart().asSeconds(), 0.25f);
        handleEvents();
        if (!window_.isOpen())
            break;
        if (focused_)
            states_.update(dt);
        audio_.update(services_.progress.music, services_.progress.sfx);
        if (states_.empty()) {
            window_.close();
            break;
        }
        window_.clear({46, 36, 30});
        window_.setView(view_);
        states_.render(window_);
        if (states_.sceneChanges() != lastScene) {
            lastScene = states_.sceneChanges();
            fade = 1.f;
        }
        if (fade > 0.f) {
            fade = std::max(0.f, fade - dt * 3.2f);
            sf::RectangleShape cover({1280, 720});
            cover.setFillColor({30, 22, 16, static_cast<sf::Uint8>(255.f * fade * fade)});
            window_.draw(cover);
        }
        window_.display();
    }
    services_.save();
}
}

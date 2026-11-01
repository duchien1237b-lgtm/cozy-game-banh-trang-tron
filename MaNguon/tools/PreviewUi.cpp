#include "core/AppConfig.hpp"
#include "ui/Art.hpp"
#include "states/MainMenuState.hpp"
#include "states/PlayingState.hpp"
#include "states/ManagementState.hpp"
#include "states/ReviewsState.hpp"
#include "states/OverlayState.hpp"
#include "states/RewardState.hpp"
#include "core/Utf8.hpp"
#include <iostream>
#include <cmath>
#include <array>
int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) return 2;
    const auto root = std::filesystem::u8path(argv[1]);
    const auto output = std::filesystem::u8path(argv[2]);
    std::filesystem::create_directories(output);
    tiem::AppConfig config = tiem::AppConfig::load(root / "data/app.json");
    tiem::ResourceManager resources(root, config.font);
    tiem::GameServices game(root, output / "test-saves");
    game.progress = tiem::Progress::fresh(game.data);
    if (argc == 4 && std::string(argv[3]) == "--decorated") {
        game.progress.money = 1000000;
        for (std::size_t i = 0; i < game.data.upgrades.size(); ++i) {
            const auto &id = game.data.upgrades[i].id;
            if (id == "decor" || id == "lantern" || id == "curtain")
                if (!tiem::UpgradeSystem::buy(game.data, game.progress, i))
                    throw std::runtime_error("Could not prepare decorated gameplay");
        }
    }
    sf::RenderWindow window;
    tiem::StateStack stack;
    tiem::AppContext context{window, resources, config, [] {}, &game, nullptr};
    const bool testVoices = argc == 4 && std::string(argv[3]) == "--happy-test";
    const bool testEffects = testVoices || (argc == 4 && std::string(argv[3]) == "--sfx-test");
    std::unique_ptr<tiem::AudioManager> effectAudio;
    if (testEffects) {
        for (const auto *name : {"choose-topping.mp3", "giao-mon.mp3",
                                 "people-background-noise.mp3", "purchased.mp3",
                                 "childen_happy.mp3", "female_happy.mp3", "man_happy.mp3", "oldman_happy.mp3",
                                 "selecting-button.mp3", "tear-ricepaper.mp3", "pop.mp3",
                                 "female_customer_leave.mp3", "male_customer_leave.mp3", "day_end.mp3", "charging_sound.mp3",
                                 "mix/perfect-mixed.mp3", "mix/good-mixed.mp3", "mix/miss-mixed.mp3"}) {
            const auto *buffer = resources.sound(std::string("assets/audio/sfx/sound-effect/") + name);
            if (!buffer || buffer->getDuration().asSeconds() <= 0 || buffer->getSampleCount() == 0)
                throw std::runtime_error(std::string("Cannot decode supplied effect: ") + name);
        }
        effectAudio = std::make_unique<tiem::AudioManager>(resources, root);
        effectAudio->update(0, 60);
        context.audio = effectAudio.get();
        context.playClick = [&] { effectAudio->play("ui_click"); };
    }
    const auto expectEffect = [&](const char *name) {
        if (testEffects && effectAudio->lastEffectPath() != std::string("assets/audio/sfx/sound-effect/") + name)
            throw std::runtime_error(std::string("Wrong effect for interaction: ") + name + " got " + effectAudio->lastEffectPath());
    };
    if (testVoices) {
        const std::map<std::string, std::string> expected = {
            {"student", "childen_happy.mp3"}, {"college", "childen_happy.mp3"},
            {"schoolgirl", "childen_happy.mp3"}, {"auntie", "female_happy.mp3"},
            {"office", "female_happy.mp3"}, {"grandma", "female_happy.mp3"},
            {"special_guest", "female_happy.mp3"}, {"uncle", "oldman_happy.mp3"},
            {"courier", "man_happy.mp3"}, {"mechanic", "man_happy.mp3"},
            {"photographer", "man_happy.mp3"}
        };
        for (const auto &kind : game.data.customers) {
            effectAudio->play(kind.happySound());
            expectEffect(expected.at(kind.id).c_str());
        }
        std::cout << "Decoded four happy voices and verified routing for all 11 customers, including Minh.\n";
        return 0;
    }
    if (argc == 4 && std::string(argv[3]) == "--music-only") {
        const auto durationOf = [&](const char *name) {
            sf::InputSoundFile input;
            if (!input.openFromFile((root / "assets/audio/music" / name).string()))
                throw std::runtime_error(std::string("Cannot decode ") + name);
            std::array<sf::Int16, 4096> samples{};
            if (input.read(samples.data(), samples.size()) == 0 || input.getDuration().asSeconds() <= 0)
                throw std::runtime_error(std::string("No audio samples in ") + name);
            return input.getDuration();
        };
        const auto menuDuration = durationOf("menu.mp3");
        const auto playingDuration = durationOf("playing.mp3");
        tiem::AudioManager audio(resources, root);
        audio.update(0, 0);
        tiem::AppContext audioContext{window, resources, config, [] {}, &game, &audio};
        const auto expect = [&](const char *scene, sf::Time duration) {
            if (audio.musicScene() != scene || audio.musicStatus() != sf::SoundSource::Playing ||
                !audio.musicLoops() || std::abs(audio.musicDuration().asSeconds() - duration.asSeconds()) > .1f)
                throw std::runtime_error(std::string("Music transition failed: ") + scene);
        };
        tiem::MainMenuState menuState(stack, audioContext);
        expect("menu", menuDuration);
        tiem::PlayingState playingState(stack, audioContext);
        expect("playing", playingDuration);
        tiem::ManagementState resultState(stack, audioContext, tiem::StateID::DayResult);
        expect("menu", menuDuration);
        tiem::ManagementState upgradeState(stack, audioContext, tiem::StateID::UpgradeShop);
        expect("menu", menuDuration);
        tiem::PlayingState nextDay(stack, audioContext);
        expect("playing", playingDuration);
        tiem::MainMenuState returnMenu(stack, audioContext);
        expect("menu", menuDuration);
        std::cout << "Decoded both MP3s and verified looping music transitions: menu "
                  << menuDuration.asSeconds() << "s, playing " << playingDuration.asSeconds() << "s.\n";
        return 0;
    }
    sf::RenderTexture target;
    if (!target.create(1280, 720)) return 3;
    if (argc == 4 && std::string(argv[3]) == "--menu-icons") {
        game.dailyPrompted = true;
        for (const auto *path : {"assets/images/cozy/ui/daily_gift_icon.png",
                                  "assets/images/cozy/ui/achievement_icon.png"})
            if (!resources.texture(path)) throw std::runtime_error("Missing menu icon");
        stack.registerFactory(tiem::StateID::MainMenu, [&] { return std::make_unique<tiem::MainMenuState>(stack, context); });
        stack.registerFactory(tiem::StateID::DailyReward, [&] { return std::make_unique<tiem::DailyRewardState>(stack, context); });
        stack.registerFactory(tiem::StateID::Achievements, [&] { return std::make_unique<tiem::AchievementsState>(stack, context); });
        stack.push(tiem::StateID::MainMenu);
        stack.applyPendingChanges();
        const auto captureMenu = [&](const char *name) {
            target.clear(); stack.render(target); target.display();
            if (!target.getTexture().copyToImage().saveToFile((output / name).string()))
                throw std::runtime_error("Cannot save menu icon preview");
        };
        const auto clickMenu = [&](sf::Vector2f position) {
            sf::Event e{}; e.mouseButton.button = sf::Mouse::Left;
            e.type = sf::Event::MouseButtonPressed; stack.handleEvent(e, position);
            e.type = sf::Event::MouseButtonReleased; stack.handleEvent(e, position);
        };
        stack.update(1.f);
        captureMenu("menu-icons.png");
        sf::Event hover{}; hover.type = sf::Event::MouseMoved;
        stack.handleEvent(hover, sf::Vector2f(72, 59)); stack.update(.2f);
        captureMenu("menu-gift-hover.png");
        clickMenu({72, 59});
        if (stack.top() != tiem::StateID::DailyReward) throw std::runtime_error("Left icon did not open daily reward");
        stack.update(.3f); captureMenu("menu-daily-open.png");
        clickMenu({640, 602});
        if (stack.top() != tiem::StateID::MainMenu) throw std::runtime_error("Daily reward did not close");
        clickMenu({1208, 59});
        if (stack.top() != tiem::StateID::Achievements) throw std::runtime_error("Right icon did not open achievements");
        stack.update(.3f); captureMenu("menu-achievements-open.png");
        clickMenu({640, 639});
        clickMenu({168, 578}); clickMenu({356, 578});
        if (stack.top() != tiem::StateID::MainMenu || stack.size() != 1)
            throw std::runtime_error("Old reward button hit areas still active");
        game.progress.lastDaily = tiem::DailyReward::today();
        game.progress.dailyStreak = 1;
        game.progress.achievements.insert(tiem::AchievementSystem::all().front().id);
        captureMenu("menu-icons-claimed.png");
        // Reset confirmation must capture clicks before the corner buttons.
        clickMenu({262, 414}); clickMenu({72, 59}); clickMenu({1208, 59});
        if (stack.top() != tiem::StateID::MainMenu || stack.size() != 1)
            throw std::runtime_error("Corner icons bypassed reset confirmation");
        std::cout << "Menu icons rendered; left/right navigation, close, removed hit areas and modal input passed.\n";
        return 0;
    }
    if (argc == 4 && std::string(argv[3]) == "--bowls-only") {
        for (const auto *path : {"assets/images/bowl_remodel/bowl_topping_before_mix.png",
                                  "assets/images/bowl_remodel/bowl_topping_after_mix.png"})
            if (!resources.texture(path)) throw std::runtime_error("Missing remodeled bowl texture");
        target.clear({255, 248, 230});
        const char *labels[] = {"Bánh vừa xé", "Đang thêm topping", "Đủ topping • Chưa trộn", "Đã trộn xong"};
        tiem::Bowl bowl;
        for (int state = 0; state < 4; ++state) {
            if (state == 1) bowl.portions.push_back(game.data.toppings.front().id);
            bowl.mixed = state == 3;
            const float x = 160.f + state * 320.f;
            tiem::ui::bowl(target, resources, game.data, bowl, {x, 330}, .85f, false, state == 2);
            tiem::ui::text(target, resources.uiFont(), labels[state], 19, {x, 520}, {81, 54, 44}, true);
        }
        target.display();
        if (!target.getTexture().copyToImage().saveToFile((output / "bowl-states.png").string()))
            return 4;
        std::cout << "Loaded both remodel textures; rendered torn paper, partial, complete and mixed bowls.\n";
        return 0;
    }
    const auto capture = [&](tiem::State &state, const char *name) {
        target.clear();
        state.render(target);
        target.display();
        if (!target.getTexture().copyToImage().saveToFile((output / name).string()))
            throw std::runtime_error("Cannot save preview");
    };
    tiem::MainMenuState menu(stack, context);
    if (testEffects && effectAudio->ambienceStatus() != sf::SoundSource::Stopped)
        throw std::runtime_error("Crowd sound should be stopped in the menu");
    capture(menu, "menu.png");
    tiem::PlayingState playing(stack, context);
    if (testEffects) {
        if (effectAudio->ambienceStatus() != sf::SoundSource::Playing || !effectAudio->ambienceLoops() ||
            std::abs(effectAudio->ambienceVolume() - 21.f) > .01f)
            throw std::runtime_error("Crowd ambience did not start at a quieter volume in gameplay");
        effectAudio->update(0, 0);
        if (effectAudio->ambienceVolume() != 0) throw std::runtime_error("Muting effects did not mute crowd ambience");
        effectAudio->update(0, 60);
    }
    for (int i = 0; i < 370; ++i) playing.update(.1f);
    capture(playing, "playing.png");
    // Exercise the visible controls, including free rubbing and delivery targets.
    const auto mouse = [&](sf::Event::EventType type, sf::Vector2f position) {
        sf::Event event{};
        event.type = type;
        if (type == sf::Event::MouseMoved) {
            event.mouseMove.x = static_cast<int>(position.x);
            event.mouseMove.y = static_cast<int>(position.y);
        } else {
            event.mouseButton.button = sf::Mouse::Left;
            event.mouseButton.x = static_cast<int>(position.x);
            event.mouseButton.y = static_cast<int>(position.y);
        }
        playing.handleEvent(event, position);
    };
    const auto click = [&](sf::Vector2f position) {
        mouse(sf::Event::MouseButtonPressed, position);
        mouse(sf::Event::MouseButtonReleased, position);
    };
    click({640, 280});
    capture(playing, "playing-selected.png");
    mouse(sf::Event::MouseButtonPressed, {737, 501});
    sf::Vector2f point;
    for (int frame = 1; frame <= 180; ++frame) {
        point = {737 + std::sin(frame * .3f) * 50, 501 + std::cos(frame * .23f) * 35};
        mouse(sf::Event::MouseMoved, point);
        playing.update(1.f / 60);
        if (frame == 45) capture(playing, "paper-tearing-fx.png");
        if (frame == 90) {
            expectEffect("tear-ricepaper.mp3");
            capture(playing, "paper-stage-01.png");
        }
    }
    capture(playing, "paper-stage-02.png");
    mouse(sf::Event::MouseButtonReleased, point);
    capture(playing, "playing-toppings.png");
    click({105, 445});
    expectEffect("choose-topping.mp3");
    mouse(sf::Event::MouseButtonPressed, {737, 491});
    if (testEffects) {
        if (effectAudio->chargingStatus() != sf::SoundSource::Playing ||
            std::abs(effectAudio->chargingVolume() - 51.f) > .01f)
            throw std::runtime_error("Charging sound failed to start at its adjusted volume");
        const auto *buffer = resources.sound("assets/audio/sfx/sound-effect/charging_sound.mp3");
        if (std::abs(buffer->getDuration().asSeconds() / effectAudio->chargingPitch() - game.data.number("topping_charge_seconds")) > .01f)
            throw std::runtime_error("Charging sound duration does not match the power bar");
        effectAudio->update(0, 0);
        if (effectAudio->chargingVolume() != 0) throw std::runtime_error("Charging sound ignored mute");
        effectAudio->update(0, 60);
    }
    playing.update(.5f);
    mouse(sf::Event::MouseMoved, {32, 690});
    if (testEffects && effectAudio->chargingStatus() != sf::SoundSource::Stopped)
        throw std::runtime_error("Charging sound continued after pointer left bowl");
    mouse(sf::Event::MouseMoved, {737, 491});
    mouse(sf::Event::MouseButtonReleased, {737, 491});
    capture(playing, "playing-charge-cancelled.png");
    for (int topping = 0; topping < 3; ++topping) {
        click({105.f + topping * 142.f, 445});
        mouse(sf::Event::MouseButtonPressed, {737, 491});
        playing.update(game.data.number("topping_charge_seconds") * (.3f + topping * .3f));
        if (topping == 0) capture(playing, "playing-charge.png");
        capture(playing, ("charge-fx-level-" + std::to_string(topping + 1) + ".png").c_str());
        const unsigned beforePops = testEffects ? effectAudio->ingredientPopCount() : 0;
        mouse(sf::Event::MouseButtonReleased, {737, 491});
        capture(playing, ("charge-release-level-" + std::to_string(topping + 1) + ".png").c_str());
        if (testEffects && effectAudio->chargingStatus() != sf::SoundSource::Stopped)
            throw std::runtime_error("Charging sound continued after release");
        for (int frame = 0; frame < 90; ++frame) {
            playing.update(1.f / 60);
            if (frame == 21) capture(playing, ("ingredient-drop-level-" + std::to_string(topping + 1) + ".png").c_str());
        }
        if (testEffects) {
            expectEffect("pop.mp3");
            if (effectAudio->ingredientPopCount() - beforePops != static_cast<unsigned>((topping + 1) * 2))
                throw std::runtime_error("Each portion must produce two icons and exactly two pops");
        }
    }
    click({1120, 560});
    capture(playing, "playing-mixing.png");
    tiem::MixingMinigame referenceMix(game.data, 0, game.progress.day);
    const float firstDelay = .45f / game.data.number("mix_speed");
    const float firstTarget = .135f + (.73f * .18f);
    const float goodPosition = firstTarget + game.data.number("mix_window") * .36f;
    const float secondDelay = (1.5f - goodPosition) / game.data.number("mix_speed");
    const float delays[] = {firstDelay, secondDelay, .3f};
    const char *mixEffects[] = {"mix/perfect-mixed.mp3", "mix/good-mixed.mp3", "mix/miss-mixed.mp3"};
    for (int hit = 0; hit < 3; ++hit) {
        const float dt = testEffects ? delays[hit] : .3f;
        playing.update(dt);
        if (testEffects) {
            referenceMix.update(dt);
            const auto score = referenceMix.hit();
            if (!score || (hit == 0 && *score <= .9f) || (hit == 1 && (*score <= .5f || *score > .9f)) ||
                (hit == 2 && *score > .5f)) throw std::runtime_error("Bad test timing for mix feedback");
        }
        click({737, 490});
        if (testEffects) expectEffect(mixEffects[hit]);
        if (hit == 0) capture(playing, "playing-mixing-moved.png");
        capture(playing, ("mix-hit-fx-" + std::to_string(hit + 1) + ".png").c_str());
    }
    capture(playing, "playing-ready.png");
    const int servedBefore = game.progress.served;
    const auto savedWeights = game.data.balance["weights"];
    if (testEffects) game.data.balance["weights"] = {{"paper", 1}, {"mix", 0}, {"recipe", 0}, {"speed", 0}};
    mouse(sf::Event::MouseButtonPressed, {737, 491});
    mouse(sf::Event::MouseMoved, {640, 280});
    mouse(sf::Event::MouseButtonReleased, {640, 280});
    expectEffect("giao-mon.mp3");
    game.data.balance["weights"] = savedWeights;
    capture(playing, "playing-review.png");
    if (game.progress.served != servedBefore + 1)
        throw std::runtime_error("UI interaction failed: paper, topping, mixing or delivery coordinates");
    if (game.progress.reviews.size() != 1 || game.progress.reviews.front().text.empty())
        throw std::runtime_error("Delivery did not produce an online review");
    for (int i = 0; i < 6; ++i) playing.update(.1f);
    if (testEffects) {
        const auto &path = effectAudio->lastEffectPath();
        bool matched = false;
        for (const auto *voice : {"childen_happy.mp3", "female_happy.mp3", "man_happy.mp3", "oldman_happy.mp3"})
            matched = matched || path == std::string("assets/audio/sfx/sound-effect/") + voice;
        if (!matched) throw std::runtime_error("Delivery did not play a customer-specific happy voice");
    }
    for (int i = 0; i < 30; ++i) playing.update(.1f);
    capture(playing, "playing-review-expired.png");
    // Stress the narrowest layout with four customers and six ingredients per order.
    game.progress = tiem::Progress::fresh(game.data);
    for (const auto &topping : game.data.toppings) game.progress.unlocked.insert(topping.id);
    game.progress.levels["queue"] = 1;
    game.data.balance["order_initial_types"] = 6;
    game.data.balance["patience_seconds"] = 200;
    game.data.balance["spawn_seconds"] = 2;
    game.data.balance["spawn_min_seconds"] = 2;
    tiem::PlayingState fullQueue(stack, context);
    for (int i = 0; i < 100; ++i) fullQueue.update(.1f);
    capture(fullQueue, "playing-four-customers.png");
    for (const auto *name : {"ingredients-page-02.png", "ingredients-page-03.png"}) {
        sf::Event pageClick{};
        pageClick.type = sf::Event::MouseButtonPressed;
        pageClick.mouseButton.button = sf::Mouse::Left;
        fullQueue.handleEvent(pageClick, sf::Vector2f(350, 693));
        pageClick.type = sf::Event::MouseButtonReleased;
        fullQueue.handleEvent(pageClick, sf::Vector2f(350, 693));
        capture(fullQueue, name);
    }
    if (testEffects) game.progress.money = 1000000;
    tiem::ManagementState shop(stack, context, tiem::StateID::UpgradeShop);
    if (testEffects) {
        if (effectAudio->ambienceStatus() != sf::SoundSource::Stopped)
            throw std::runtime_error("Crowd ambience did not stop outside gameplay");
        const auto shopClick = [&](sf::Vector2f position) {
            sf::Event event{};
            event.type = sf::Event::MouseButtonPressed;
            event.mouseButton.button = sf::Mouse::Left;
            shop.handleEvent(event, position);
            event.type = sf::Event::MouseButtonReleased;
            shop.handleEvent(event, position);
            shop.update(.016f);
        };
        shopClick({505, 264});
        expectEffect("purchased.mp3");
    }
    capture(shop, "upgrades.png");
    const auto beforeUpgradePreview = game.progress;
    game.progress = tiem::Progress::fresh(game.data);
    game.progress.money = 10000000;
    for (int level = 1; level <= 3; ++level) {
        for (std::size_t i = 0; i < game.data.upgrades.size(); ++i)
            if (static_cast<int>(game.data.upgrades[i].levels.size()) >= level &&
                !tiem::UpgradeSystem::buy(game.data, game.progress, i))
                throw std::runtime_error("Preview upgrade purchase failed");
        tiem::ManagementState upgradedShop(stack, context, tiem::StateID::UpgradeShop);
        capture(upgradedShop, ("upgrades-level-" + std::to_string(level) + ".png").c_str());
        tiem::PlayingState upgradedGame(stack, context);
        for (int frame = 0; frame < 20; ++frame) upgradedGame.update(.1f);
        capture(upgradedGame, ("upgraded-paper-" + std::to_string(level) + ".png").c_str());
        sf::Event gesture{};
        gesture.type = sf::Event::MouseButtonPressed;
        gesture.mouseButton.button = sf::Mouse::Left;
        upgradedGame.handleEvent(gesture, sf::Vector2f(737, 491));
        for (int frame = 0; frame < 180; ++frame) {
            gesture.type = sf::Event::MouseMoved;
            upgradedGame.handleEvent(gesture, sf::Vector2f(frame % 2 ? 757.f : 717.f, 491));
            upgradedGame.update(1.f / 60);
        }
        gesture.type = sf::Event::MouseButtonReleased;
        gesture.mouseButton.button = sf::Mouse::Left;
        upgradedGame.handleEvent(gesture, sf::Vector2f(737, 491));
        capture(upgradedGame, ("upgraded-bowl-" + std::to_string(level) + ".png").c_str());
    }
    game.progress = beforeUpgradePreview;
    // Use actual purchases and isolated saves to exercise each decoration independently.
    for (const auto &item : {std::pair<const char *, const char *>{"decor", "decor-bunting.png"},
                             {"lantern", "decor-lantern.png"}, {"curtain", "decor-curtain.png"}}) {
        game.progress = tiem::Progress::fresh(game.data);
        game.progress.money = 1000000;
        for (std::size_t i = 0; i < game.data.upgrades.size(); ++i)
            if (game.data.upgrades[i].id == item.first && !tiem::UpgradeSystem::buy(game.data, game.progress, i))
                throw std::runtime_error("Decoration purchase failed");
        if (tiem::UpgradeSystem::level(game.progress, item.first) != 1)
            throw std::runtime_error("Decoration was not purchased");
        tiem::PlayingState decorated(stack, context);
        for (int i = 0; i < 100; ++i) decorated.update(.1f);
        capture(decorated, item.second);
    }
    game.progress.money = 1000000;
    for (std::size_t i = 0; i < game.data.upgrades.size(); ++i)
        if (game.data.upgrades[i].id == "decor" || game.data.upgrades[i].id == "lantern")
            if (!tiem::UpgradeSystem::buy(game.data, game.progress, i)) throw std::runtime_error("Decoration purchase failed");
    if (!game.saves.write(game.progress)) throw std::runtime_error("Could not save decorations");
    game.progress = game.saves.load(game.data);
    for (const auto *id : {"decor", "lantern", "curtain"})
        if (tiem::UpgradeSystem::level(game.progress, id) != 1) throw std::runtime_error("Purchased decoration lost on reload");
    tiem::PlayingState allDecor(stack, context);
    for (int i = 0; i < 100; ++i) allDecor.update(.1f);
    capture(allDecor, "decor-all.png");
    game.progress.levels["queue"] = 1;
    tiem::PlayingState fourDecor(stack, context);
    for (int i = 0; i < 100; ++i) fourDecor.update(.1f);
    capture(fourDecor, "decor-four-customers.png");
    // Open the reviews window through the actual menu button, using isolated progress only.
    game.progress = tiem::Progress::fresh(game.data);
    tiem::StateStack reviewStack;
    reviewStack.registerFactory(tiem::StateID::MainMenu,
        [&] { return std::make_unique<tiem::MainMenuState>(reviewStack, context); });
    reviewStack.registerFactory(tiem::StateID::Reviews,
        [&] { return std::make_unique<tiem::ReviewsState>(reviewStack, context); });
    reviewStack.push(tiem::StateID::MainMenu);
    reviewStack.applyPendingChanges();
    const auto reviewClick = [&](sf::Vector2f position) {
        sf::Event event{};
        event.type = sf::Event::MouseButtonPressed;
        event.mouseButton.button = sf::Mouse::Left;
        reviewStack.handleEvent(event, position);
        event.type = sf::Event::MouseButtonReleased;
        reviewStack.handleEvent(event, position);
        reviewStack.update(.016f);
    };
    const auto captureReviews = [&](const char *name) {
        target.clear();
        reviewStack.render(target);
        target.display();
        if (!target.getTexture().copyToImage().saveToFile((output / name).string()))
            throw std::runtime_error("Could not render reviews window");
    };
    reviewClick({170, 541});
    expectEffect("selecting-button.mp3");
    if (reviewStack.top() != tiem::StateID::Reviews)
        throw std::runtime_error("Menu button did not open reviews");
    captureReviews("online-reviews-empty.png");
    sf::Event closeReviews{};
    closeReviews.type = sf::Event::KeyPressed;
    closeReviews.key.code = sf::Keyboard::Escape;
    reviewStack.handleEvent(closeReviews, std::nullopt);
    if (reviewStack.top() != tiem::StateID::MainMenu)
        throw std::runtime_error("Closing reviews did not return to menu");
    for (int i = 0; i < 7; ++i) {
        tiem::Receipt sample;
        sample.score = i < 5 ? i * .25f : i == 5 ? .5f : 1.f;
        sample.recipe = sample.mix = sample.speed = sample.paper = sample.score;
        tiem::ReviewSystem::record(game.progress, sample, i / 3 + 1);
    }
    reviewClick({170, 541});
    captureReviews("online-reviews.png");
    const auto reviewedId = game.progress.reviews.back().id;
    const auto reviewKey = [&](sf::Keyboard::Key key, bool control = false) {
        sf::Event event{};
        event.type = sf::Event::KeyPressed;
        event.key.code = key;
        event.key.control = control;
        reviewStack.handleEvent(event, std::nullopt);
    };
    const auto reviewType = [&](const std::string &value) {
        for (const auto codepoint : tiem::utf8(value)) {
            sf::Event event{};
            event.type = sf::Event::TextEntered;
            event.text.unicode = codepoint;
            reviewStack.handleEvent(event, std::nullopt);
        }
    };
    reviewClick({1089, 342});
    reviewKey(sf::Keyboard::Enter);
    if (!game.progress.reviews.back().reply.empty())
        throw std::runtime_error("Empty reply was accepted");
    const std::string reply = "Cảm ơn bạn đã góp ý chân thành. Tiệm sẽ chăm chút từng phần bánh hơn nữa!";
    reviewType(reply + "x");
    reviewKey(sf::Keyboard::BackSpace);
    captureReviews("online-review-editor.png");
    reviewClick({956, 607});
    if (game.progress.reviews.back().id != reviewedId || game.progress.reviews.back().reply != reply)
        throw std::runtime_error("Reply editor saved the wrong review or damaged Vietnamese text");
    captureReviews("online-reviews-replied.png");
    reviewClick({1089, 342});
    reviewKey(sf::Keyboard::A, true);
    const std::string revised = "Tiệm đã ghi nhận góp ý của bạn. Hẹn gặp lại bạn với phần bánh thật ngon nhé!";
    reviewType(revised);
    reviewKey(sf::Keyboard::Enter);
    if (game.progress.reviews.back().reply != revised)
        throw std::runtime_error("Editing the existing response failed");
    reviewClick({1030, 126});
    captureReviews("online-reviews-unanswered.png");
    reviewClick({1127, 648});
    captureReviews("online-reviews-page-2.png");
    reviewClick({1089, 342});
    reviewClick({353, 544});
    reviewClick({956, 607});
    if (game.progress.reviews[3].reply.empty() || !game.progress.reviews[4].reply.empty())
        throw std::runtime_error("Filtered pagination replied to the wrong review");
    reviewClick({1128, 83});
    if (reviewStack.top() != tiem::StateID::MainMenu)
        throw std::runtime_error("Reviews close button failed");
    game.progress = game.saves.load(game.data);
    if (game.progress.reviews.back().reply != revised || game.progress.reviews[3].reply.empty())
        throw std::runtime_error("Replies were not persisted");
    reviewClick({170, 541});
    captureReviews("online-reviews-reloaded.png");
    // Render all new models in actual gameplay, then exercise voice routing with short patience.
    const auto allKinds = game.data.customers;
    const auto originalBalance = game.data.balance;
    for (const auto &kind : allKinds) {
        if (kind.id != "schoolgirl" && kind.id != "grandma" && kind.id != "mechanic" &&
            kind.id != "photographer" && kind.id != "special_guest") continue;
        game.data.customers = {kind};
        game.data.balance = originalBalance;
        game.progress = tiem::Progress::fresh(game.data);
        tiem::PlayingState character(stack, context);
        character.update(.25f);
        capture(character, ("customer-" + kind.id + ".png").c_str());
        for (const auto *emotion : {"happy", "normal", "angry"})
            if (!resources.texture("assets/images/cozy/customers/" + kind.style + "_" + emotion + ".png"))
                throw std::runtime_error("Missing new customer portrait");
        if (testEffects) {
            game.data.balance["patience_seconds"] = .1f;
            game.data.balance["patience_min_seconds"] = .1f;
            tiem::PlayingState impatient(stack, context);
            impatient.update(.5f);
            expectEffect(kind.femaleVoice ? "female_customer_leave.mp3" : "male_customer_leave.mp3");
            impatient.update(game.data.number("day_seconds"));
            expectEffect("day_end.mp3");
        }
    }
    game.data.customers = allKinds;
    game.data.balance = originalBalance;
    if (testEffects) {
        // A sauce batch still produces per-icon pops, without a separate pour sound.
        game.progress = tiem::Progress::fresh(game.data);
        game.progress.unlocked = {"sa_te", "muoi_tom", "xoai_song"};
        tiem::PlayingState sauce(stack, context);
        const auto send = [&](sf::Event::EventType type, sf::Vector2f point) {
            sf::Event e{}; e.type = type; e.mouseButton.button = sf::Mouse::Left;
            sauce.handleEvent(e, point);
        };
        send(sf::Event::MouseButtonPressed, {737, 491});
        for (int i = 0; i < 180; ++i) {
            send(sf::Event::MouseMoved, {i % 2 ? 757.f : 717.f, 491});
            sauce.update(1.f / 60);
        }
        send(sf::Event::MouseButtonReleased, {737, 491});
        send(sf::Event::MouseButtonPressed, {389, 445});
        send(sf::Event::MouseButtonReleased, {389, 445});
        send(sf::Event::MouseButtonPressed, {737, 491});
        sauce.update(game.data.number("topping_charge_seconds") * .3f);
        send(sf::Event::MouseButtonReleased, {737, 491});
        expectEffect("choose-topping.mp3");
        const auto popsBefore = effectAudio->ingredientPopCount();
        for (int i = 0; i < 60; ++i) sauce.update(1.f / 60);
        if (effectAudio->ingredientPopCount() - popsBefore != 2)
            throw std::runtime_error("Sauce lost its per-icon pops");
        send(sf::Event::MouseButtonPressed, {737, 491});
        sauce.update(game.data.number("topping_charge_seconds"));
        if (effectAudio->chargingStatus() != sf::SoundSource::Stopped)
            throw std::runtime_error("Charging sound continued after power bar filled");
        send(sf::Event::MouseButtonReleased, {737, 491});
        for (int i = 0; i < 90; ++i) sauce.update(1.f / 60);
        send(sf::Event::MouseButtonPressed, {737, 491});
        sauce.onCovered();
        if (effectAudio->chargingStatus() != sf::SoundSource::Stopped)
            throw std::runtime_error("Charging sound continued when gameplay was covered");
    }
    game.progress = tiem::Progress::fresh(game.data);
    game.progress.day = 10;
    game.progress.money = 987654;
    game.progress.served = 42;
    game.progress.levels["paper"] = 3;
    game.progress.unlocked.insert("kho_bo");
    game.progress.music = 23;
    game.progress.sfx = 47;
    game.progress.reviews.push_back({1, "Khách thử", "Rất ngon", "Cảm ơn bạn", 9, 5});
    game.hasResult = true;
    if (!game.save()) throw std::runtime_error("Cannot prepare isolated restart test");
    tiem::StateStack navigation;
    navigation.registerFactory(tiem::StateID::MainMenu, [&] { return std::make_unique<tiem::MainMenuState>(navigation, context); });
    navigation.registerFactory(tiem::StateID::Settings, [&] { return std::make_unique<tiem::OverlayState>(navigation, context, tiem::StateID::Settings); });
    navigation.registerFactory(tiem::StateID::Playing, [&] { return std::make_unique<tiem::PlayingState>(navigation, context); });
    const auto navigateClick = [&](sf::Vector2f point) {
        sf::Event e{}; e.type = sf::Event::MouseButtonPressed; e.mouseButton.button = sf::Mouse::Left;
        navigation.handleEvent(e, point);
        e.type = sf::Event::MouseButtonReleased;
        navigation.handleEvent(e, point);
        navigation.update(.016f);
    };
    const auto navigationCapture = [&](const char *name) {
        target.clear(); navigation.render(target); target.display();
        if (!target.getTexture().copyToImage().saveToFile((output / name).string()))
            throw std::runtime_error("Cannot capture menu navigation");
    };
    navigation.push(tiem::StateID::MainMenu); navigation.applyPendingChanges();
    navigationCapture("menu-new-navigation.png");
    navigateClick({340, 483});
    if (navigation.top() != tiem::StateID::Settings) throw std::runtime_error("Menu settings button failed");
    navigationCapture("settings.png");
    navigateClick({640, 485});
    navigationCapture("settings-help.png");
    sf::Event escape{}; escape.type = sf::Event::KeyPressed; escape.key.code = sf::Keyboard::Escape;
    navigation.handleEvent(escape, std::nullopt);
    if (navigation.top() != tiem::StateID::Settings) throw std::runtime_error("Closing help also closed settings");
    navigateClick({640, 545});
    if (navigation.top() != tiem::StateID::MainMenu) throw std::runtime_error("Settings did not return to menu");
    navigateClick({260, 365});
    if (navigation.top() != tiem::StateID::Playing || game.progress.day != 10 || game.progress.money != 987654)
        throw std::runtime_error("Continue changed saved progress");
    navigation.clear(); navigation.push(tiem::StateID::MainMenu); navigation.applyPendingChanges();
    navigateClick({260, 425});
    navigationCapture("restart-confirm.png");
    navigateClick({510, 455});
    if (game.progress.day != 10 || game.progress.money != 987654) throw std::runtime_error("Cancel reset progress");
    navigateClick({260, 425});
    navigateClick({770, 455});
    const auto restarted = game.saves.load(game.data);
    if (navigation.top() != tiem::StateID::Playing || restarted.day != 1 ||
        restarted.money != game.data.integer("initial_money") || !restarted.levels.empty() ||
        !restarted.reviews.empty() || restarted.served != 0 || restarted.unlocked.count("kho_bo") ||
        restarted.music != 23 || restarted.sfx != 47 || game.hasResult)
        throw std::runtime_error("Restart did not persist fresh progress or preserve sound settings");
    navigation.clear(); navigation.push(tiem::StateID::MainMenu); navigation.applyPendingChanges();
    navigationCapture("menu-after-restart.png");
    std::cout << "Rendered UI previews; gameplay, sound, menu navigation, help and confirmed restart passed.\n";
}

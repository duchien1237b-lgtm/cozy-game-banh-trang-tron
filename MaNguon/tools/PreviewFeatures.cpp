// Xuất ảnh xem trước các tính năng mới (thời tiết, giờ trong ngày, khách, chuẩn bị, tổng kết, hướng dẫn)
// bằng RenderTexture, không cần mở cửa sổ. Dùng để kiểm tra giao diện khi chỉnh sửa.
#include "core/AppConfig.hpp"
#include "gameplay/Inventory.hpp"
#include "states/PlayingState.hpp"
#include "states/PrepState.hpp"
#include "states/DayReportState.hpp"
#include "states/TutorialState.hpp"
#include "states/MainMenuState.hpp"
#include "states/ManagementState.hpp"
#include "states/OverlayState.hpp"
#include <iostream>
int main(int argc, char **argv) {
    if (argc < 3) return 2;
    const auto root = std::filesystem::u8path(argv[1]);
    const auto output = std::filesystem::u8path(argv[2]);
    const std::string only = argc > 3 ? argv[3] : "";
    std::filesystem::create_directories(output);
    tiem::AppConfig config = tiem::AppConfig::load(root / "data/app.json");
    tiem::ResourceManager resources(root, config.font);
    tiem::GameServices game(root, output / "test-saves");
    game.progress = tiem::Progress::fresh(game.data);
    game.progress.tutorialDone = true;
    game.progress.guideDone = true;
    sf::RenderWindow window;
    tiem::StateStack stack;
    tiem::AppContext context{window, resources, config, [] {}, &game, nullptr, {}};
    sf::RenderTexture target;
    if (!target.create(1280, 720)) return 3;
    const auto save = [&](tiem::State &state, const std::string &name) {
        target.clear();
        state.render(target);
        target.display();
        target.getTexture().copyToImage().saveToFile((output / (name + ".png")).string());
        std::cout << "saved " << name << "\n";
    };
    const auto key = [](tiem::State &state, sf::Keyboard::Key code) {
        sf::Event e{};
        e.type = sf::Event::KeyPressed;
        e.key.code = code;
        state.handleEvent(e, std::nullopt);
    };
    const auto findSeed = [&](int day, tiem::Weather base, bool changes) {
        for (std::uint32_t seed = 1; seed < 100000; ++seed) {
            const auto plan = tiem::WeatherSystem::forecast(seed, day);
            if (plan.base == base && plan.changes() == changes) return seed;
        }
        return 1u;
    };
    if (only.empty() || only == "playing") {
        struct Shot { const char *name; tiem::Weather weather; float seconds; int day; };
        const Shot shots[] = {{"play-morning-sunny", tiem::Weather::Sunny, 8, 3},
                              {"play-noon-rain", tiem::Weather::Rainy, 60, 3},
                              {"play-sunset", tiem::Weather::Cloudy, 145, 3},
                              {"play-night", tiem::Weather::Sunny, 172, 3}};
        for (const auto &shot : shots) {
            game.progress.day = shot.day;
            game.progress.worldSeed = findSeed(shot.day, shot.weather, false);
            for (const auto &t : game.data.toppings) game.progress.unlocked.insert(t.id);
            for (const auto &t : game.data.toppings) game.progress.stock[t.id] = 6;
            game.progress.stock["xoai_song"] = 0;
            tiem::PlayingState playing(stack, context);
            key(playing, sf::Keyboard::Enter);
            for (float t = 0; t < shot.seconds; t += .1f) playing.update(.1f);
            save(playing, shot.name);
        }
    }
    const auto click = [](tiem::State &state, sf::Vector2f at, sf::Mouse::Button button = sf::Mouse::Left) {
        sf::Event e{};
        e.mouseButton.button = button;
        e.mouseButton.x = static_cast<int>(at.x);
        e.mouseButton.y = static_cast<int>(at.y);
        e.type = sf::Event::MouseButtonPressed;
        state.handleEvent(e, at);
        e.type = sf::Event::MouseButtonReleased;
        state.handleEvent(e, at);
    };
    const auto press = [](tiem::State &state, sf::Vector2f at, bool down) {
        sf::Event e{};
        e.mouseButton.button = sf::Mouse::Left;
        e.type = down ? sf::Event::MouseButtonPressed : sf::Event::MouseButtonReleased;
        state.handleEvent(e, at);
    };
    const auto move = [](tiem::State &state, sf::Vector2f at) {
        sf::Event e{};
        e.type = sf::Event::MouseMoved;
        e.mouseMove.x = static_cast<int>(at.x);
        e.mouseMove.y = static_cast<int>(at.y);
        state.handleEvent(e, at);
    };
    if (only.empty() || only == "prep") {
        game.progress = tiem::Progress::fresh(game.data);
        game.progress.guideDone = true;
        game.progress.day = 4;
        game.progress.money = 52000;
        game.progress.worldSeed = findSeed(4, tiem::Weather::Sunny, true);
        game.progress.unlocked.insert("sa_te");
        game.progress.unlocked.insert("dau_phong");
        game.progress.stock = {{"muoi_tom", 7}, {"hanh_phi", 3}, {"sa_te", 4}};
        tiem::PrepState prep(stack, context);
        prep.update(1.f);
        move(prep, {650, 160});
        prep.update(.2f);
        save(prep, "prep");
    }
    if (only.empty() || only == "report") {
        game.progress = tiem::Progress::fresh(game.data);
        game.progress.day = 6;
        tiem::DayResult r;
        r.day = 5;
        r.revenue = 182000;
        r.tips = 41000;
        r.comboBonus = 9000;
        r.rushBonus = 6000;
        r.weatherBonus = 4000;
        r.goalBonus = 22600;
        r.ingredientCost = 38500;
        r.spoiledPortions = 6;
        r.spoiledValue = 4200;
        r.served = 11;
        r.left = 1;
        r.scoreSum = 9.6f;
        r.bestCombo = 5;
        r.perfectMixes = 14;
        r.bestScore = .97f;
        r.bestKind = 7;
        r.bestPersonality = 4;
        r.delighted = 5;
        r.weather = tiem::WeatherSystem::forecast(findSeed(5, tiem::Weather::Sunny, true), 5);
        const int served[] = {0, 1, 1, 0, 2, 2, 1, 0, 1, 1, 2, 0, 0, 0};
        const int left[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0};
        for (int i = 0; i < 14; ++i) { r.servedByHour[static_cast<std::size_t>(i)] = served[i]; r.leftByHour[static_cast<std::size_t>(i)] = left[i]; }
        r.goals = tiem::DaySession::makeGoals(game.data, 5);
        r.goals[0].progress = r.goals[0].target;
        r.goals[1].progress = r.goals[1].target / 2;
        r.newAchievements = {"combo_5"};
        game.progress.stock = {{"muoi_tom", 5}, {"hanh_phi", 2}};
        game.lastDay = r;
        tiem::DayReportState report(stack, context);
        for (int i = 0; i < 30; ++i) report.update(.1f);
        save(report, "report");
    }
    if (only.empty() || only == "tutorial") {
        game.progress = tiem::Progress::fresh(game.data);
        tiem::TutorialState guide(stack, context);
        for (int page = 0; page < tiem::TutorialState::pageCount; ++page) {
            for (int i = 0; i < 25; ++i) guide.update(.1f);
            if (page == 2) {
                press(guide, {360, 340}, true);
                for (int i = 0; i < 40; ++i) { move(guide, {300.f + (i % 2) * 120.f, 340}); guide.update(.05f); }
            }
            if (page == 3) {
                for (int n = 0; n < 2; ++n) {
                    press(guide, {360, 390}, true);
                    for (int i = 0; i < 8; ++i) guide.update(.1f);
                    press(guide, {360, 390}, false);
                }
                press(guide, {360, 390}, true);
                for (int i = 0; i < 5; ++i) guide.update(.1f);
            }
            if (page == 4) {
                key(guide, sf::Keyboard::Space);
                for (int i = 0; i < 6; ++i) guide.update(.1f);
            }
            save(guide, "tutorial-" + std::to_string(page + 1));
            key(guide, sf::Keyboard::Right);
        }
    }
    if (only.empty() || only == "menu") {
        game.progress = tiem::Progress::fresh(game.data);
        game.dailyPrompted = true;
        tiem::MainMenuState menu(stack, context);
        menu.update(.5f);
        save(menu, "menu");
    }
    if (only.empty() || only == "flow") {
        // Ngày chơi thật: nhập hàng, xé, thêm topping, trộn, giao, kiểm tra kho bị trừ và khách rời quầy.
        game.progress = tiem::Progress::fresh(game.data);
        game.progress.guideDone = game.progress.tutorialDone = true;
        game.progress.worldSeed = findSeed(2, tiem::Weather::Rainy, false);
        game.progress.day = 2;
        tiem::PlayingState playing(stack, context);
        key(playing, sf::Keyboard::Enter);
        playing.update(.1f);
        const int before = tiem::InventorySystem::stock(game.progress, "muoi_tom");
        press(playing, {737, 520}, true);
        for (int i = 0; i < 80; ++i) { move(playing, {690.f + (i % 2) * 90.f, 520}); playing.update(.05f); }
        press(playing, {737, 520}, false);
        playing.update(.1f);
        click(playing, {105, 470});
        press(playing, {737, 513}, true);
        for (int i = 0; i < 12; ++i) playing.update(.1f);
        press(playing, {737, 513}, false);
        for (int i = 0; i < 25; ++i) playing.update(.1f);
        const int after = tiem::InventorySystem::stock(game.progress, "muoi_tom");
        save(playing, "flow-toppings");
        std::cout << "stock muoi_tom " << before << " -> " << after << "\n";
        if (after >= before) throw std::runtime_error("Adding toppings did not consume stock");
        key(playing, sf::Keyboard::Space); // trộn
        for (int beat = 0; beat < 3; ++beat) {
            for (int i = 0; i < 40; ++i) playing.update(.05f);
            key(playing, sf::Keyboard::Space);
        }
        for (int i = 0; i < 6; ++i) playing.update(.05f);
        save(playing, "flow-mixed");
        key(playing, sf::Keyboard::Space); // giao
        for (int i = 0; i < 6; ++i) playing.update(.1f);
        save(playing, "flow-served");
        if (game.progress.served != 1) throw std::runtime_error("Dish was not served in flow");
    }
    if (only.empty() || only == "journey") {
        // Hành trình người chơi mới qua StateStack thật: menu → hướng dẫn → chuẩn bị → chơi → tổng kết → chuẩn bị ngày 2.
        game.progress = tiem::Progress::fresh(game.data);
        game.dailyPrompted = true;
        tiem::StateStack flow;
        tiem::AppContext ctx{window, resources, config, [] {}, &game, nullptr, {}};
        flow.registerFactory(tiem::StateID::MainMenu, [&] { return std::make_unique<tiem::MainMenuState>(flow, ctx); });
        flow.registerFactory(tiem::StateID::Tutorial, [&] { return std::make_unique<tiem::TutorialState>(flow, ctx); });
        flow.registerFactory(tiem::StateID::Prep, [&] { return std::make_unique<tiem::PrepState>(flow, ctx); });
        flow.registerFactory(tiem::StateID::Playing, [&] { return std::make_unique<tiem::PlayingState>(flow, ctx); });
        flow.registerFactory(tiem::StateID::DayResult, [&] { return std::make_unique<tiem::DayReportState>(flow, ctx); });
        flow.registerFactory(tiem::StateID::UpgradeShop, [&] { return std::make_unique<tiem::ManagementState>(flow, ctx, tiem::StateID::UpgradeShop); });
        flow.registerFactory(tiem::StateID::Pause, [&] { return std::make_unique<tiem::OverlayState>(flow, ctx, tiem::StateID::Pause); });
        const auto expect = [&](tiem::StateID id, const char *what) {
            flow.applyPendingChanges();
            if (flow.top() != id) throw std::runtime_error(std::string("Journey failed at: ") + what);
            std::cout << "ok: " << what << "\n";
        };
        const auto keyFlow = [&](sf::Keyboard::Key code) {
            sf::Event e{};
            e.type = sf::Event::KeyPressed;
            e.key.code = code;
            flow.handleEvent(e, std::nullopt);
            flow.applyPendingChanges();
        };
        const auto clickFlow = [&](sf::Vector2f at) {
            sf::Event e{};
            e.mouseButton.button = sf::Mouse::Left;
            e.type = sf::Event::MouseButtonPressed;
            flow.handleEvent(e, at);
            e.type = sf::Event::MouseButtonReleased;
            flow.handleEvent(e, at);
            flow.applyPendingChanges();
        };
        flow.push(tiem::StateID::MainMenu);
        expect(tiem::StateID::MainMenu, "menu");
        clickFlow({262, 359}); // Bắt đầu mở quán
        expect(tiem::StateID::Tutorial, "first start opens the guide");
        for (int i = 0; i < tiem::TutorialState::pageCount; ++i) { flow.update(.2f); keyFlow(sf::Keyboard::Right); }
        expect(tiem::StateID::Prep, "guide finishes into ingredient prep");
        if (!game.progress.guideDone) throw std::runtime_error("Guide not marked done");
        const int before = game.progress.money;
        flow.update(.2f);
        clickFlow({1100, 666}); // Nhập hàng & mở quán
        expect(tiem::StateID::Playing, "prep opens the shop");
        if (game.progress.money >= before || game.progress.todayCost != before - game.progress.money)
            throw std::runtime_error("Prep purchase not charged");
        keyFlow(sf::Keyboard::Enter); // Mở cửa
        for (int i = 0; i < 1900; ++i) { flow.update(.1f); flow.applyPendingChanges(); if (flow.top() != tiem::StateID::Playing) break; }
        expect(tiem::StateID::DayResult, "day closes at 21:00 into the new report");
        if (game.lastDay.ingredientCost != before - game.progress.money + game.lastDay.goalBonus + game.lastDay.revenue + game.lastDay.tips &&
            game.lastDay.ingredientCost <= 0)
            throw std::runtime_error("Report missing ingredient cost");
        for (int i = 0; i < 20; ++i) flow.update(.1f);
        target.clear(); flow.render(target); target.display();
        target.getTexture().copyToImage().saveToFile((output / "journey-report.png").string());
        keyFlow(sf::Keyboard::Enter);
        expect(tiem::StateID::Prep, "report continues to next day's prep");
        if (game.progress.day != 2) throw std::runtime_error("Day did not advance");
        keyFlow(sf::Keyboard::Escape);
        expect(tiem::StateID::MainMenu, "prep can go back to menu");
        clickFlow({262, 359});
        expect(tiem::StateID::Prep, "returning player skips the guide");
        std::cout << "Journey passed. Ingredient cost day 1: " << game.lastDay.ingredientCost << ", spoiled " << game.lastDay.spoiledPortions << "\n";
    }
    return 0;
}

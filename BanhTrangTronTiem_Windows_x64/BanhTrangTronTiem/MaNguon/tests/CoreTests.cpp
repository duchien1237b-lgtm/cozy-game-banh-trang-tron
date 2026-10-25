#include "core/AppConfig.hpp"
#include "core/ResourceManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/StateStack.hpp"
#include "core/Utf8.hpp"
#include "gameplay/Minigames.hpp"
#include "gameplay/Model.hpp"
#include "ui/Button.hpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

struct Counts {
    int updates{};
    int events{};
    int destroyed{};
};
class Probe final : public tiem::State {
  public:
    Probe(tiem::StateStack &stack, tiem::AppContext &context, Counts &counts, bool popOnEvent = false)
        : State(stack, context), counts_(counts), popOnEvent_(popOnEvent) {}
    ~Probe() override {
        ++counts_.destroyed;
    }
    bool handleEvent(const sf::Event &, tiem::Pointer) override {
        ++counts_.events;
        if (popOnEvent_) {
            stack_.pop();
            require(counts_.destroyed == 0, "State destroyed inside its own event callback");
        }
        return false;
    }
    bool update(float) override {
        ++counts_.updates;
        return false;
    }
    void render(sf::RenderTarget &) override {}

  private:
    Counts &counts_;
    bool popOnEvent_;
};
} // namespace

int main(int argc, char **argv) {
    try {
        require(argc == 2, "Expected project root");
        const std::filesystem::path root(argv[1]);
        const auto config = tiem::AppConfig::load(root / "data/app.json");
        require(config.width == 1280 && config.height == 720 && config.fpsLimit == 60, "Wrong window config");
        const auto invalid = std::filesystem::temp_directory_path() / "tiem_invalid_config_test.json";
        {
            std::ofstream out(invalid);
            out << R"({"title":"Test","font":"font.ttf","window":{"width":-1,"height":720,"fps_limit":60}})";
        }
        bool rejected = false;
        try {
            tiem::AppConfig::load(invalid);
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        std::filesystem::remove(invalid);
        require(rejected, "Negative width accepted");

        const tiem::GameData data(root);
        auto progress = tiem::Progress::fresh(data);
        require(data.toppings.size() == 13 && progress.unlocked.size() == 5, "Wrong initial ingredients");
        require(data.customers.size() == 6 && data.upgrades.size() == 8, "Missing content");
        require(data.number("day_seconds") == 120, "Default day must last 120 seconds");
        tiem::Bowl bowl;
        require(bowl.add("muoi_tom", 2) && bowl.add("xoai_song", 2), "Portions rejected before capacity");
        require(!bowl.add("muoi_tom", 2) && bowl.count("muoi_tom") == 1, "Capacity overflow changed bowl");
        bowl.mixed = true;
        require(!bowl.add("rau_ram", 9), "Mixed dish accepted more ingredients");
        bowl.clear();
        require(bowl.portions.empty() && !bowl.mixed, "Reset failed");

        // Chấm một đơn biết trước: kiểm tra thiếu, thừa, sai và trọng số tốc độ.
        tiem::Customer customer;
        customer.patience = customer.remaining = 60;
        customer.order.recipe = {{"muoi_tom", 1}, {"xoai_song", 2}};
        customer.order.price = 20000;
        bowl.add("muoi_tom", 24);
        bowl.add("xoai_song", 24);
        bowl.add("xoai_song", 24);
        bowl.paperQuality = bowl.mixQuality = 1;
        bowl.mixed = true;
        auto receipt = tiem::EconomySystem::score(data, progress, customer, bowl);
        require(std::abs(receipt.score - 1) < .0001f && receipt.base == 20000 && receipt.tip == 3600,
                "Perfect recipe payout wrong");
        customer.remaining = 30;
        auto slower = tiem::EconomySystem::score(data, progress, customer, bowl);
        require(std::abs(slower.score - .95f) < .0001f, "Speed weight must be 10 percent");
        bowl.portions.pop_back();
        require(tiem::EconomySystem::score(data, progress, customer, bowl).recipe < 1,
                "Missing topping not penalized");
        bowl.portions.push_back("trung_cut");
        require(tiem::EconomySystem::score(data, progress, customer, bowl).recipe < .5f,
                "Wrong topping not penalized");
        bowl.portions = {"muoi_tom", "xoai_song", "xoai_song", "rau_ram"};
        require(tiem::EconomySystem::score(data, progress, customer, bowl).recipe < 1,
                "Extra topping not penalized");

        const auto initialMoney = progress.money;
        require(!tiem::UpgradeSystem::buy(data, progress, 0) && progress.money == initialMoney,
                "Unaffordable upgrade changed money");
        progress.money = 1000000;
        require(tiem::UpgradeSystem::buy(data, progress, 0), "Could not buy patience");
        require(tiem::UpgradeSystem::effect(data, progress, "patience") == 8, "Wrong level-one effect");
        require(tiem::UpgradeSystem::buy(data, progress, 0), "Could not upgrade twice");
        require(tiem::UpgradeSystem::effect(data, progress, "patience") == 16,
                "Level effects should not accumulate twice");
        require(tiem::UpgradeSystem::buy(data, progress, 0), "Could not buy final level");
        require(!tiem::UpgradeSystem::buy(data, progress, 0), "Bought beyond maximum level");
        require(tiem::UpgradeSystem::unlock(data, progress, 5), "Could not unlock kho bo");
        const int afterUnlock = progress.money;
        require(!tiem::UpgradeSystem::unlock(data, progress, 5) && progress.money == afterUnlock,
                "Charged twice for unlock");
        require(!tiem::UpgradeSystem::buy(data, progress, 900), "Invalid upgrade accepted");

        tiem::PaperTearing paper(data, 0);
        paper.begin({720, 375});
        paper.move({840, 375});
        require(!paper.release(), "Tearing should start at an endpoint");
        paper.begin({640, 375});
        paper.move({670, 375});
        require(!paper.release(), "Short stroke was accepted");
        for (int line = 0; line < 3; ++line) {
            const float y = tiem::PaperTearing::lineY(line);
            paper.begin({640, y});
            paper.update(data.number("paper_ideal_seconds"));
            paper.move({840, y});
            const auto quality = paper.release();
            require(quality && *quality > .999f, "Straight complete tear should be perfect");
        }
        require(paper.done() && paper.quality() > .999f, "Paper completion wrong");
        paper.begin({640, 375});
        require(!paper.release(), "Already-cut line accepted twice");
        tiem::PaperTearing reverse(data, 0);
        reverse.begin({840, 375});
        reverse.move({640, 375});
        require(reverse.release().has_value(), "Reverse swipe failed");
        tiem::PaperTearing cancelled(data, 0);
        cancelled.begin({640, 375});
        cancelled.move({840, 375});
        cancelled.cancel();
        require(!cancelled.release() && !cancelled.cut(0), "Cancel committed a tear");
        tiem::PaperTearing assisted(data, 10);
        assisted.begin({640, 397});
        assisted.move({840, 397});
        require(assisted.release().has_value(), "Paper upgrade does not widen tolerance");

        tiem::MixingMinigame mix(data, 0, 1);
        mix.update(.45f / data.number("mix_speed"));
        require(mix.hit().value_or(0) == 1, "Center should score perfect");
        require(!mix.hit(), "Double-click bypassed cooldown");
        for (int hit = 1; hit < data.integer("mix_hits"); ++hit) {
            mix.update(2.f / data.number("mix_speed"));
            require(mix.hit().value_or(0) == 1, "Reflected timing cursor lost position");
        }
        require(mix.done() && mix.score() == 1 && !mix.hit(), "Mix did not stop after configured hits");
        tiem::MixingMinigame miss(data, 0, 1), wide(data, .1f, 1);
        require(miss.hit().value_or(1) == data.balance["mix_scores"]["miss"].get<float>(),
                "Outside region should miss");
        require(wide.width() > miss.width(), "Mix upgrade does not widen region");

        auto dailyProgress = tiem::Progress::fresh(data);
        tiem::DaySession day(data, dailyProgress, 42);
        require(day.customers().size() == 1, "First customer missing");
        require(day.customers()[0].order.recipe.size() == 2, "Day one should have two topping types");
        tiem::Bowl exact;
        for (const auto &pair : day.customers()[0].order.recipe) {
            require(dailyProgress.unlocked.count(pair.first) != 0, "Locked topping appeared in order");
            for (int count = 0; count < pair.second; ++count)
                exact.add(pair.first, day.capacity());
        }
        require(day.serve(0, exact).base == 0 && day.customers().size() == 1, "Unmixed dish served");
        exact.mixed = true;
        exact.mixQuality = exact.paperQuality = 1;
        auto sale = day.serve(0, exact);
        require(day.result().served == 1 && dailyProgress.served == 1 && day.customers().empty(),
                "Sale did not remove customer or record stats");
        require(dailyProgress.money == initialMoney + sale.base + sale.tip, "Wallet did not receive sale");
        const int paid = dailyProgress.money;
        require(day.serve(0, exact).base == 0 && dailyProgress.money == paid,
                "Could pay same customer twice");
        for (int i = 0; i < 40; ++i)
            day.update(.5f);
        require(!day.customers().empty(), "No customer spawned during day");
        day.update(120);
        require(day.ended() && dailyProgress.day == 2 && day.result().left > 0,
                "Day did not finish or count remaining customers");
        day.finish();
        day.update(120);
        require(dailyProgress.day == 2 && day.result().day == 1, "End-of-day committed twice");
        require(day.result().rating() >= 1 && day.result().rating() <= 5, "Rating outside 1..5");
        auto patientData = data;
        patientData.balance["patience_seconds"] = 1000;
        auto queueProgress = tiem::Progress::fresh(data);
        queueProgress.money = 1000000;
        tiem::DaySession queueDay(patientData, queueProgress, 4);
        for (int i = 0; i < 90; ++i)
            queueDay.update(1);
        require(queueDay.customers().size() == 3, "Queue exceeded default capacity");
        for (std::size_t i = 0; i < data.upgrades.size(); ++i)
            if (data.upgrades[i].effect == "queue")
                require(tiem::UpgradeSystem::buy(data, queueProgress, i), "Queue upgrade failed");
        tiem::DaySession expanded(patientData, queueProgress, 4);
        for (int i = 0; i < 90; ++i)
            expanded.update(1);
        require(expanded.customers().size() == 4, "Queue upgrade did not add place");
        auto laterProgress = progress;
        laterProgress.day = 7;
        tiem::DaySession later(data, laterProgress, 3);
        require(later.customers()[0].order.recipe.size() == 4, "Later days did not increase complexity");

        // Dùng thư mục riêng; không chạm tới tiến trình thật của người chơi.
        const auto temp =
            std::filesystem::temp_directory_path() /
            ("tiem_tests_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        tiem::SaveSystem saves(temp);
        progress.day = 4;
        progress.music = 23;
        progress.sfx = 47;
        require(saves.write(progress), "Could not write save");
        const auto loaded = saves.load(data);
        require(loaded.day == 4 && loaded.money == progress.money && loaded.unlocked == progress.unlocked &&
                    loaded.levels == progress.levels,
                "Save lost progress");
        require(loaded.music == 23 && loaded.sfx == 47, "Save lost settings");
        progress.day = 5;
        require(saves.write(progress), "Second save failed");
        {
            std::ofstream corrupt(temp / "save.json");
            corrupt << "broken json";
        }
        require(saves.load(data).day == 4 && !saves.error().empty(), "Corrupt save did not recover backup");
        require(saves.write(progress) && saves.load(data).day == 5, "Could not save after recovery");
        std::filesystem::remove_all(temp);
        require(tiem::moneyText(1234567) == "1.234.567 đ", "Currency format wrong");

        sf::RenderWindow window(sf::VideoMode(320, 180), "Core tests");
        window.setVisible(false);
        tiem::ResourceManager resources(root, config.font);
        const auto &font = resources.uiFont();
        require(&font == &resources.uiFont(), "Font cache returned different object");
        for (auto codepoint : tiem::utf8("Bánh tráng trộn nóng hổi, cay xé lưỡi! ă â đ ê ô ơ ư Ắ ộ ỵ"))
            require(font.hasGlyph(codepoint), "Missing Vietnamese glyph");
        require(resources.texture("assets/images/not_present.png") == nullptr,
                "Missing image should return fallback");
        require(resources.texture("assets/images/not_present.png") == nullptr, "Missing image cache failed");
        require(resources.sound("assets/audio/not_present.wav") == nullptr,
                "Missing audio should be optional");

        int clicks = 0;
        int sounds = 0;
        tiem::ui::Button button(font, "Chơi", {10, 10, 100, 40}, [&] { ++clicks; });
        button.setClickSound([&] { ++sounds; });
        sf::Event press{};
        press.type = sf::Event::MouseButtonPressed;
        press.mouseButton.button = sf::Mouse::Left;
        auto release = press;
        release.type = sf::Event::MouseButtonReleased;
        button.handleEvent(press, sf::Vector2f(30, 30));
        button.handleEvent(release, sf::Vector2f(30, 30));
        require(clicks == 1 && sounds == 1, "Normal click must fire once");
        button.handleEvent(press, sf::Vector2f(200, 200));
        button.handleEvent(release, sf::Vector2f(30, 30));
        require(clicks == 1, "Drag-in activated button");
        button.handleEvent(press, sf::Vector2f(30, 30));
        button.handleEvent(release, std::nullopt);
        require(clicks == 1, "Letterbox release activated button");
        button.setEnabled(false);
        button.handleEvent(press, sf::Vector2f(30, 30));
        button.handleEvent(release, sf::Vector2f(30, 30));
        require(clicks == 1, "Disabled button activated");
        button.setEnabled(true);
        button.handleEvent(press, sf::Vector2f(30, 30));
        sf::Event lost{};
        lost.type = sf::Event::LostFocus;
        button.handleEvent(lost, std::nullopt);
        button.handleEvent(release, sf::Vector2f(30, 30));
        require(clicks == 1, "Focus loss left pressed state stuck");

        tiem::AppContext context{window, resources, config, {}};
        Counts playing, pause;
        tiem::StateStack stack;
        stack.registerFactory(tiem::StateID::Playing,
                              [&] { return std::make_unique<Probe>(stack, context, playing); });
        stack.registerFactory(tiem::StateID::Pause,
                              [&] { return std::make_unique<Probe>(stack, context, pause, true); });
        stack.push(tiem::StateID::Playing);
        require(stack.empty(), "Push should be deferred");
        stack.applyPendingChanges();
        stack.update(0.1f);
        stack.push(tiem::StateID::Pause);
        stack.applyPendingChanges();
        stack.update(0.1f);
        require(playing.updates == 1 && pause.updates == 1, "Pause failed to block gameplay update");
        stack.handleEvent(press, sf::Vector2f(30, 30));
        require(playing.events == 0 && pause.events == 1 && pause.destroyed == 1,
                "Event leaked through overlay");
        require(stack.size() == 1 && stack.top() == tiem::StateID::Playing,
                "Pop did not resume previous state");
        stack.update(0.1f);
        require(playing.updates == 2, "Gameplay did not resume");
        stack.replace(tiem::StateID::Playing);
        stack.applyPendingChanges();
        require(stack.size() == 1 && playing.destroyed == 1, "Replace leaked old state");
        stack.clear();
        stack.applyPendingChanges();
        require(stack.empty() && playing.destroyed == 2, "Clear leaked state");
        stack.pop();
        stack.applyPendingChanges();
        require(stack.empty(), "Pop empty stack is not safe");
        std::cout << "PASS: data, recipe scoring, upgrades, paper, mixing, customers, day loop, save "
                     "recovery, UTF-8, input and state lifetime\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

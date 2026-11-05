#include "core/AppConfig.hpp"
#include "core/ResourceManager.hpp"
#include "core/SaveSystem.hpp"
#include "core/StateStack.hpp"
#include "core/Utf8.hpp"
#include "gameplay/CustomerMood.hpp"
#include "gameplay/Inventory.hpp"
#include "gameplay/Minigames.hpp"
#include "gameplay/Model.hpp"
#include "ui/Button.hpp"
#include <algorithm>
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
        const std::map<std::string, std::string> happyVoices = {
            {"student", "child"}, {"college", "child"}, {"schoolgirl", "child"},
            {"auntie", "female"}, {"office", "female"}, {"grandma", "female"},
            {"special_guest", "female"}, {"uncle", "oldman"},
            {"courier", "man"}, {"mechanic", "man"}, {"photographer", "man"}
        };
        for (const auto &kind : data.customers)
            require(kind.happySound() == "customer_happy_" + happyVoices.at(kind.id),
                    "Wrong happy voice for customer");
        tiem::CustomerKind voicePriority;
        voicePriority.child = voicePriority.femaleVoice = voicePriority.olderAdult = true;
        require(voicePriority.happySound() == "customer_happy_child", "Child voice must override gender");
        voicePriority.child = false;
        require(voicePriority.happySound() == "customer_happy_female", "Older women must retain female voice");
        auto progress = tiem::Progress::fresh(data);
        require(data.toppings.size() == 13 && progress.unlocked.size() == 5, "Wrong initial ingredients");
        require(data.customers.size() == 11 && data.upgrades.size() == 8, "Missing content");
        std::set<std::size_t> seenKinds;
        int specialVisits = 0;
        for (unsigned seed = 0; seed < 1000; ++seed) {
            auto visitProgress = tiem::Progress::fresh(data);
            tiem::DaySession visit(data, visitProgress, seed);
            const auto index = visit.customers().front().kind;
            seenKinds.insert(index);
            const auto &kind = data.customers[index];
            if (kind.special) {
                ++specialVisits;
                require(kind.name == "Chị Dung" && kind.femaleVoice, "Special guest identity is wrong");
            }
        }
        require(seenKinds.size() == 11, "Some customer kinds cannot spawn");
        require(specialVisits > 0 && specialVisits < 100, "Special guest frequency is wrong");
        require(data.number("day_seconds") == 180, "Default day must last three minutes");
        require(tiem::difficultyMultiplier(data, 1) == 1.f &&
                std::abs(tiem::difficultyMultiplier(data, 2) - 1.05f) < .0001f &&
                std::abs(tiem::difficultyMultiplier(data, 14) - 1.65f) < .0001f &&
                tiem::difficultyMultiplier(data, 14) == tiem::difficultyMultiplier(data, 1000),
                "Difficulty ramp does not cap at 65 percent");
        for (const auto &kind : data.customers) {
            auto personalData = data;
            personalData.customers = {kind};
            for (unsigned seed = 0; seed < 20; ++seed) {
                auto freshProgress = tiem::Progress::fresh(data);
                for (const auto &t : data.toppings) freshProgress.unlocked.insert(t.id);
                int previousTotal = 0;
                float previousPatience = 10000.f;
                std::vector<int> cappedShape;
                float cappedPatience = 0;
                for (int dayNumber : {1, 2, 7, 14, 30, 1000}) {
                    freshProgress.day = dayNumber;
                    tiem::DaySession session(personalData, freshProgress, seed);
                    const auto &guest = session.customers().front();
                    int total = 0;
                    for (const auto &item : guest.order.recipe) {
                        total += item.second;
                    }
                    require(total >= kind.minTypes && total <= static_cast<int>(std::floor(kind.minTypes * 1.65f + .0001f)) + 2 &&
                            total <= session.capacity(), "Order workload below minimum or exceeded the cap plus random extra");
                    // basePatience là phần do độ khó quyết định; tính cách và thời tiết nhân thêm ở patience.
                    require(guest.basePatience <= previousPatience + .0001f &&
                            guest.basePatience >= data.number("patience_seconds") * kind.patienceFactor / 1.65f - .001f,
                            "Patience scaling is not monotonic or exceeds the difficulty cap");
                    const auto &persona = data.personalities.at(guest.personality);
                    require(guest.patience > 0 && std::abs(guest.patience - guest.basePatience * persona.patience *
                            tiem::WeatherSystem::effect(session.weather()).patience) < .001f,
                            "Personality or weather patience factor not applied");
                    if (dayNumber == 1) require(total <= kind.minTypes + 1 &&
                            std::abs(guest.basePatience - 84.f * kind.patienceFactor) < .001f, "Day one is not easier");
                    // Thời tiết đổi loại topping được gọi, nhưng khối lượng đơn (số loại, số muỗng) vẫn ngừng tăng sau ngày 14.
                    const std::vector<int> shape = {static_cast<int>(guest.order.recipe.size()), total};
                    if (dayNumber == 14) { cappedShape = shape; cappedPatience = guest.basePatience; }
                    if (dayNumber > 14) require(shape == cappedShape && guest.basePatience == cappedPatience,
                            "Customer difficulty kept increasing past day 14");
                    previousTotal = total;
                    previousPatience = guest.basePatience;
                }
            }
        }
        tiem::MixingMinigame cappedMix(data, 0, 14), farFutureMix(data, 0, 1000);
        cappedMix.update(.2f); farFutureMix.update(.2f);
        require(cappedMix.position() == farFutureMix.position(), "Mixing difficulty keeps increasing after the cap");
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
        auto specialData = data;
        for (const auto &kind : data.customers)
            if (kind.special) specialData.customers = {kind};
        auto specialProgress = tiem::Progress::fresh(specialData);
        specialProgress.levels["decor"] = 3;
        specialProgress.levels["lantern"] = specialProgress.levels["curtain"] = 1;
        require(tiem::EconomySystem::score(specialData, specialProgress, customer, bowl).tip == customer.order.price,
                "Special tip must be exactly the listed selling price, without upgrade stacking");
        auto poorBowl = bowl;
        poorBowl.portions.clear();
        poorBowl.paperQuality = poorBowl.mixQuality = 0;
        require(tiem::EconomySystem::score(specialData, specialProgress, customer, poorBowl).tip == customer.order.price,
                "Special tip must not depend on rating");
        tiem::DaySession specialDay(specialData, specialProgress, 9);
        const int guestPrice = specialDay.customers().front().order.price;
        const int guestWallet = specialProgress.money;
        const auto guestSale = specialDay.serve(0, poorBowl);
        require(guestSale.tip == guestPrice && specialDay.result().tips == guestPrice &&
                specialProgress.money == guestWallet + guestSale.base + guestPrice,
                "Special guest tip was not credited to wallet and daily totals");
        require(specialDay.serve(0, poorBowl).tip == 0, "Special guest paid twice");
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

        // Free rubbing: count moving time only, retain progress across pauses.
        const sf::FloatRect paperArea{0, 0, 200, 200};
        require(data.number("paper_stage_seconds") == 1.5f, "Each tear stage must take 1.5 seconds");
        tiem::PaperTearing paper(data, paperArea);
        paper.begin({250, 100});
        paper.move({100, 100});
        paper.update(.1f);
        require(paper.elapsed() == 0, "Press outside paper started tearing");
        paper.begin({100, 100});
        paper.update(2.f);
        require(paper.elapsed() == 0, "Stationary hold advanced tearing");
        const auto rub = [](tiem::PaperTearing &sheet, int frames) {
            for (int i = 0; i < frames; ++i) {
                sheet.move({i % 2 ? 75.f : 125.f, i % 3 ? 110.f : 85.f});
                sheet.update(.1f);
            }
        };
        rub(paper, 14);
        require(paper.completedStages() == 0 && !paper.done(), "First stage completed before 1.5 seconds");
        paper.move({100, 100});
        paper.update(.1f);
        require(paper.completedStages() == 1 && std::abs(paper.progress() - .5f) < .0001f,
                "First image transition must occur at 1.5 seconds");
        require(!paper.release(), "Partial tearing committed the paper");
        const float halfTime = paper.elapsed();
        paper.move({125, 100});
        paper.update(1);
        require(paper.elapsed() == halfTime, "Released pointer advanced tearing");
        paper.begin({100, 100});
        paper.move({300, 100});
        paper.update(.1f);
        require(paper.elapsed() == halfTime, "Rubbing outside the paper advanced tearing");
        paper.move({100, 100});
        paper.update(.1f);
        require(paper.elapsed() == halfTime, "Reentry counted travel outside the paper");
        paper.move({125, 100});
        paper.cancel();
        paper.update(.1f);
        require(!paper.release() && paper.elapsed() == halfTime, "Focus loss lost progress or committed tearing");
        paper.begin({100, 100});
        rub(paper, 14);
        require(!paper.done() && paper.completedStages() == 1, "Second stage completed too early");
        paper.move({100, 100});
        paper.update(.1f);
        require(paper.done() && paper.completedStages() == 2 && paper.quality() == 1,
                "Three seconds of free rubbing did not complete both stages");
        require(paper.release().value_or(0) == 1 && !paper.release(), "Completed release must commit once");
        // A focus loss after the final image must not leave the UI permanently stuck.
        paper.cancel();
        paper.begin({100, 100});
        require(paper.release().value_or(0) == 1, "Completed paper cannot resume after focus loss");
        tiem::PaperTearing resetPaper(data, paperArea);
        require(resetPaper.progress() == 0 && !resetPaper.done(), "Fresh bowl inherited tearing progress");
        resetPaper.begin({100, 100});
        resetPaper.move({125, 100});
        resetPaper.update(.1f);
        resetPaper.update(.1f);
        require(std::abs(resetPaper.elapsed() - .1f) < .0001f, "Idle frames reused earlier movement");
        resetPaper.move({75, 100});
        resetPaper.update(50);
        require(resetPaper.elapsed() <= .3501f, "Stalled frame instantly tore the paper");
        tiem::PaperTearing assisted(data, paperArea, 10);
        require(!resetPaper.contains({207, 100}) && assisted.contains({207, 100}),
                "Paper upgrade does not widen rubbing area");
        require(assisted.stageSeconds() == 1.5f, "Upgrade changed requested stage duration");

        // Purchased levels survive save compatibility: level IDs, not old effect values, select duration.
        auto boardProgress = tiem::Progress::fresh(data);
        boardProgress.money = 1000000;
        for (std::size_t index = 0; index < data.upgrades.size(); ++index) {
            if (data.upgrades[index].id != "paper") continue;
            for (int level = 1; level <= 3; ++level) {
                require(tiem::UpgradeSystem::buy(data, boardProgress, index), "Board purchase failed");
                tiem::PaperTearing upgraded(data, paperArea,
                    tiem::UpgradeSystem::effect(data, boardProgress, "paper"),
                    tiem::UpgradeSystem::level(boardProgress, "paper"));
                const float expected = level == 3 ? .75f : level == 2 ? 1.25f : 1.5f;
                require(upgraded.stageSeconds() == expected, "Wrong purchased board duration");
                upgraded.begin({100, 100});
                const int frames = static_cast<int>(expected * 120);
                for (int frame = 0; frame < frames; ++frame) {
                    upgraded.move({frame % 2 ? 110.f : 90.f, 100});
                    upgraded.update(1.f / 60);
                    if (frame == frames - 2) require(!upgraded.done(), "Upgraded paper finished early");
                }
                require(upgraded.done() && upgraded.release().value_or(0) == 1, "Upgraded paper did not finish");
            }
        }
        tiem::ToppingCharge charge(data.number("topping_charge_seconds"));
        charge.update(10);
        require(charge.power() == 0 && charge.release() == 0, "Unpressed topping charge advanced");
        require(data.number("topping_charge_seconds") == 2.4f, "Power bar should fill in 2.4 seconds");
        charge.begin();
        charge.update(1.2f);
        require(std::abs(charge.power() - .5f) < .0001f, "Power bar still charges at its old speed");
        charge.cancel();
        charge.begin();
        charge.update(.05f);
        require(charge.release() == 0, "Accidental quick click added topping");
        for (const auto &sample : {std::pair<float, int>{.3f, 1}, {.55f, 2}, {1.f, 3}}) {
            charge.begin();
            charge.update(sample.first * data.number("topping_charge_seconds"));
            require(charge.portions() == sample.second, "Power does not match the displayed portion count");
            require(charge.release() == sample.second && !charge.active() && charge.release() == 0,
                    "A release should commit once and reset the charge");
        }
        charge.begin();
        charge.update(100);
        require(charge.power() == 1 && charge.portions() == 3, "Charge must saturate at three portions");
        charge.cancel();
        require(charge.release() == 0 && charge.power() == 0, "Cancelled charge still adds topping");

        tiem::MixingMinigame mix(data, 0, 1);
        mix.update(.45f / data.number("mix_speed"));
        require(mix.hit().value_or(0) == 1, "Initial center should score perfect");
        require(std::abs(mix.target() - .5f) > .1f, "Target did not move after a hit");
        const auto targetDuringCooldown = mix.target();
        require(!mix.hit() && mix.target() == targetDuringCooldown, "Double-click bypassed cooldown or moved zone");
        for (int hit = 1; hit < data.integer("mix_hits"); ++hit) {
            const float oldTarget = mix.target();
            float elapsed = 0;
            bool accepted = false;
            for (int frame = 0; frame < 3000; ++frame) {
                mix.update(.002f);
                elapsed += .002f;
                if (elapsed > data.number("mix_hit_cooldown") && std::abs(mix.position() - mix.target()) < mix.width() * .1f) {
                    require(mix.hit().value_or(0) == 1, "Scoring did not follow the relocated zone");
                    accepted = true;
                    break;
                }
            }
            require(accepted && std::abs(mix.target() - oldTarget) > .1f, "Next mixing zone was not reachable or did not move");
            require(mix.target() >= mix.width() / 2 && mix.target() <= 1.f - mix.width() / 2,
                    "Green zone escaped the timing bar");
        }
        require(mix.done() && mix.score() == 1 && !mix.hit(), "Mix did not stop after configured hits");
        tiem::MixingMinigame miss(data, 0, 1), wide(data, .8f, 1);
        require(miss.hit().value_or(1) == data.balance["mix_scores"]["miss"].get<float>() && miss.target() != .5f,
                "A miss must also relocate the green zone");
        require(wide.width() > miss.width(), "Mix upgrade does not widen region");
        wide.hit();
        require(wide.target() - wide.width() / 2 >= 0 && wide.target() + wide.width() / 2 <= 1,
                "Upgraded mixing zone exceeds bounds");
        wide.update(100);
        require(wide.position() >= 0 && wide.position() <= 1, "Reflected cursor escaped the bar");
        for (const auto &sample : {std::pair<float, int>{0.f, 1}, {.25f, 2}, {.5f, 3}, {.75f, 4}, {1.f, 5}}) {
            tiem::Receipt rated;
            rated.score = sample.first;
            require(rated.stars() == sample.second && std::string(rated.review()).size() > 0,
                    "Delivery star rating or review missing");
        }

        // Online reviews have their own authors and longer posts, not the immediate delivery quote.
        auto reviewProgress = tiem::Progress::fresh(data);
        require(reviewProgress.reviews.empty() && reviewProgress.nextReviewId == 1,
                "Fresh players must not receive invented historical reviews");
        for (const auto &sample : {std::pair<float, int>{0.f, 1}, {.25f, 2}, {.5f, 3}, {.75f, 4}, {1.f, 5}}) {
            tiem::Receipt rated;
            rated.score = rated.recipe = rated.mix = rated.paper = rated.speed = sample.first;
            tiem::ReviewSystem::record(reviewProgress, rated, sample.second);
            const auto &review = reviewProgress.reviews.back();
            require(review.stars == sample.second && review.day == sample.second &&
                        review.text != rated.review() && tiem::utf8(review.text).getSize() > 100 &&
                        tiem::utf8(review.text).getSize() <= 280,
                    "Online post is missing, too long or reused the delivery quote");
            for (const auto &kind : data.customers)
                require(review.author != kind.name, "Online review reused a fixed NPC identity");
        }
        std::set<std::string> onlineAuthors, onlinePosts;
        auto variety = tiem::Progress::fresh(data);
        for (int i = 0; i < 32; ++i) {
            tiem::ReviewSystem::record(variety, receipt, 2);
            onlineAuthors.insert(variety.reviews.back().author);
            onlinePosts.insert(variety.reviews.back().text);
        }
        require(onlineAuthors.size() == 32 && onlinePosts.size() >= 8,
                "Online identities or posts repeat too frequently");
        const auto replyId = reviewProgress.reviews.back().id;
        const auto originalAuthor = reviewProgress.reviews.back().author;
        const auto originalPost = reviewProgress.reviews.back().text;
        const int beforeReplyMoney = reviewProgress.money;
        const std::string replyText = "Cảm ơn bạn đã ghé tiệm! Hẹn gặp lại bạn nhé.";
        require(tiem::ReviewSystem::reply(reviewProgress, replyId, replyText), "Vietnamese reply rejected");
        require(!tiem::ReviewSystem::reply(reviewProgress, replyId, "  \n\t\r") &&
                    !tiem::ReviewSystem::reply(reviewProgress, replyId, "\xc2\xa0") &&
                    !tiem::ReviewSystem::reply(reviewProgress, replyId, "\xc0\xaf") &&
                    !tiem::ReviewSystem::reply(reviewProgress, 9999, replyText),
                "Blank, invalid UTF-8 or nonexistent review reply accepted");
        std::string unicodeLimit;
        for (int i = 0; i < 240; ++i) unicodeLimit += "đ";
        require(tiem::ReviewSystem::reply(reviewProgress, replyId, unicodeLimit) &&
                    !tiem::ReviewSystem::reply(reviewProgress, replyId, unicodeLimit + "đ"),
                "Reply character limit counted UTF-8 bytes instead of characters");
        require(tiem::ReviewSystem::reply(reviewProgress, replyId, replyText) &&
                    reviewProgress.reviews.back().author == originalAuthor &&
                    reviewProgress.reviews.back().text == originalPost && reviewProgress.money == beforeReplyMoney,
                "Editing reply changed author, original review or economy");
        auto cappedReviews = tiem::Progress::fresh(data);
        for (int i = 0; i < 120; ++i) tiem::ReviewSystem::record(cappedReviews, receipt, 1);
        require(cappedReviews.reviews.size() == 100 && cappedReviews.reviews.front().id == 21 &&
                    cappedReviews.reviews.back().id == 120 && cappedReviews.nextReviewId == 121,
                "Review history cap dropped newest entries or reused IDs");

        auto dailyProgress = tiem::Progress::fresh(data);
        tiem::DaySession day(data, dailyProgress, 42);
        require(day.customers().size() == 1, "First customer missing");
        require(day.customers()[0].order.recipe.size() >= 3, "Day one must request at least three topping types");
        for (int dayNumber : {1, 3, 7, 30}) {
            for (unsigned seed = 0; seed < 20; ++seed) {
                auto generatedProgress = tiem::Progress::fresh(data);
                generatedProgress.day = dayNumber;
                tiem::DaySession generated(data, generatedProgress, seed);
                const auto &recipe = generated.customers().front().order.recipe;
                const auto &kind = data.customers[generated.customers().front().kind];
                require(recipe.size() >= std::min<std::size_t>(kind.minTypes, generatedProgress.unlocked.size()) &&
                        recipe.size() <= static_cast<std::size_t>(kind.maxTypes), "Order ignores customer topping range");
                int total = 0;
                for (const auto &item : recipe) {
                    require(generatedProgress.unlocked.count(item.first) != 0, "Generated order requested locked ingredient");
                    require(item.second >= 1 && item.second <= kind.maxQuantity, "Wrong customer topping quantity");
                    total += item.second;
                }
                require(total <= generated.capacity(), "Order exceeds bowl capacity");
            }
        }
        tiem::Bowl exact;
        for (const auto &pair : day.customers()[0].order.recipe) {
            require(dailyProgress.unlocked.count(pair.first) != 0, "Locked topping appeared in order");
            for (int count = 0; count < pair.second; ++count)
                exact.add(pair.first, day.capacity());
        }
        require(day.serve(0, exact).base == 0 && day.customers().size() == 1 && dailyProgress.reviews.empty(),
                "Unmixed dish served or created a review");
        exact.mixed = true;
        exact.mixQuality = exact.paperQuality = 1;
        auto sale = day.serve(0, exact);
        require(day.result().served == 1 && dailyProgress.served == 1 && day.customers().empty(),
                "Sale did not remove customer or record stats");
        require(dailyProgress.money == initialMoney + sale.base + sale.tip, "Wallet did not receive sale");
        require(dailyProgress.reviews.size() <= 1 &&
                    (dailyProgress.reviews.empty() || dailyProgress.reviews.front().stars == sale.stars()),
                "Optional online review has the wrong sale rating");
        const auto reviewCount = dailyProgress.reviews.size();
        const int paid = dailyProgress.money;
        require(day.serve(0, exact).base == 0 && dailyProgress.money == paid && dailyProgress.reviews.size() == reviewCount,
                "Could pay or review same customer twice");
        int onlinePostsAfterSales = 0;
        for (unsigned seed = 0; seed < 200; ++seed) {
            auto occasional = tiem::Progress::fresh(data);
            tiem::DaySession session(data, occasional, seed);
            tiem::Bowl dish;
            dish.mixQuality = dish.paperQuality = 1;
            for (const auto &item : session.customers().front().order.recipe)
                for (int n = 0; n < item.second; ++n) dish.add(item.first, session.capacity());
            dish.mixed = true;
            const auto receiptForPost = session.serve(0, dish);
            require(occasional.served == 1 && session.result().served == 1 && receiptForPost.base > 0,
                    "Skipping an online review must not skip the sale or its stats");
            require(occasional.reviews.size() <= 1 &&
                        (occasional.reviews.empty() || occasional.reviews.front().stars == receiptForPost.stars()),
                    "Online post must belong to its completed sale");
            onlinePostsAfterSales += static_cast<int>(occasional.reviews.size());
            const auto count = occasional.reviews.size();
            const auto nextId = occasional.nextReviewId;
            session.serve(0, dish);
            require(occasional.reviews.size() == count && occasional.nextReviewId == nextId,
                    "An invalid duplicate sale must not create an online post");
        }
        require(onlinePostsAfterSales >= 50 && onlinePostsAfterSales <= 110,
                "Online reviews should occur for roughly 40% of customers, not every sale");
        for (int i = 0; i < 50; ++i)
            day.update(.5f);
        require(!day.customers().empty(), "No customer spawned during day");
        day.update(data.number("day_seconds"));
        require(day.ended() && dailyProgress.day == 2 && day.result().left > 0,
                "Day did not finish or count remaining customers");
        day.finish();
        day.update(data.number("day_seconds"));
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
        auto fullyUnlocked = tiem::Progress::fresh(data);
        fullyUnlocked.day = 7;
        for (const auto &topping : data.toppings) fullyUnlocked.unlocked.insert(topping.id);
        tiem::DaySession largeOrder(data, fullyUnlocked, 8);
        require(largeOrder.customers().front().order.recipe.size() <=
                static_cast<std::size_t>(data.customers[largeOrder.customers().front().kind].maxTypes),
                "Order exceeds customer topping preference");
        for (const auto &kind : data.customers) {
            auto personalData = data;
            personalData.customers = {kind};
            for (unsigned seed = 0; seed < 20; ++seed) {
                tiem::DaySession personal(personalData, fullyUnlocked, seed);
                const auto &recipe = personal.customers().front().order.recipe;
                require(recipe.size() >= static_cast<std::size_t>(kind.minTypes) && recipe.size() <= static_cast<std::size_t>(kind.maxTypes), "Customer type limit ignored");
                int portions = 0;
                for (const auto &item : recipe) {
                    require(item.second >= 1 && item.second <= kind.maxQuantity,
                            "Customer portion preference ignored");
                    portions += item.second;
                }
                require(portions <= personal.capacity(), "Personal order exceeds bowl capacity");
            }
        }
        auto timedProgress = tiem::Progress::fresh(data);
        tiem::DaySession timed(data, timedProgress, 10);
        timed.update(179.f);
        require(!timed.ended() && timed.remaining() == 1.f, "Day ended before three minutes");
        timed.update(1.f);
        require(timed.ended() && timedProgress.day == 2, "Day did not end at three minutes");
        auto laterProgress = progress;
        laterProgress.day = 7;
        tiem::DaySession later(data, laterProgress, 3);
        require(later.customers()[0].order.recipe.size() <= std::min<std::size_t>(
                data.customers[later.customers()[0].kind].maxTypes, laterProgress.unlocked.size()),
                "Later orders exceed customer preferences");

        // Dùng thư mục riêng; không chạm tới tiến trình thật của người chơi.
        const auto temp =
            std::filesystem::temp_directory_path() /
            ("tiem_tests_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        tiem::SaveSystem saves(temp);
        progress.day = 4;
        progress.music = 23;
        progress.sfx = 47;
        tiem::ReviewSystem::record(progress, receipt, 4);
        require(tiem::ReviewSystem::reply(progress, progress.reviews.back().id, replyText), "Reply setup failed");
        require(saves.write(progress), "Could not write save");
        const auto loaded = saves.load(data);
        require(loaded.day == 4 && loaded.money == progress.money && loaded.unlocked == progress.unlocked &&
                    loaded.levels == progress.levels,
                "Save lost progress");
        require(loaded.music == 23 && loaded.sfx == 47, "Save lost settings");
        require(loaded.reviews.size() == 1 && loaded.reviews.front().author == progress.reviews.front().author &&
                    loaded.reviews.front().text == progress.reviews.front().text &&
                    loaded.reviews.front().reply == replyText && loaded.reviews.front().stars == 5 &&
                    loaded.reviews.front().day == 4 && loaded.nextReviewId == progress.nextReviewId,
                "Save lost online review, identity, reply or sequence");
        progress.day = 5;
        require(saves.write(progress), "Second save failed");
        {
            std::ofstream corrupt(temp / "save.json");
            corrupt << "broken json";
        }
        require(saves.load(data).day == 4 && !saves.error().empty(), "Corrupt save did not recover backup");
        require(saves.write(progress) && saves.load(data).day == 5, "Could not save after recovery");
        nlohmann::json savedJson;
        { std::ifstream input(temp / "save.json"); input >> savedJson; }
        const auto writeTestSave = [&](const nlohmann::json &value) {
            std::ofstream output(temp / "save.json", std::ios::trunc);
            output << value.dump(2);
        };
        auto legacy = savedJson;
        legacy.erase("reviews");
        legacy.erase("next_review_id");
        writeTestSave(legacy);
        const auto legacyLoaded = saves.load(data);
        require(legacyLoaded.day == 5 && legacyLoaded.money == progress.money && legacyLoaded.reviews.empty() &&
                    legacyLoaded.nextReviewId == 1 && saves.error().empty(),
                "Legacy save compatibility failed or fabricated historical reviews");
        auto malformed = savedJson;
        malformed["reviews"] = "broken optional field";
        malformed["next_review_id"] = nlohmann::json::object();
        writeTestSave(malformed);
        auto recoveredReviews = saves.load(data);
        require(recoveredReviews.day == 5 && recoveredReviews.money == progress.money &&
                    recoveredReviews.reviews.empty() && saves.error().empty(),
                "Malformed optional reviews discarded core progress");
        auto validEntry = savedJson["reviews"][0];
        validEntry["id"] = 7;
        validEntry["reply"] = std::string(241, 'a');
        malformed["reviews"] = nlohmann::json::array({nullptr, 5, {{"id", 2}}, validEntry, validEntry});
        malformed["next_review_id"] = 1;
        writeTestSave(malformed);
        recoveredReviews = saves.load(data);
        require(recoveredReviews.day == 5 && recoveredReviews.reviews.size() == 1 &&
                    recoveredReviews.reviews.front().id == 7 && recoveredReviews.reviews.front().reply.empty() &&
                    recoveredReviews.nextReviewId == 8 && saves.error().empty(),
                "Malformed entries, duplicate IDs, overlong reply or counter repair failed");
        malformed["reviews"] = nlohmann::json::array();
        for (int i = 1; i <= 110; ++i) {
            validEntry["id"] = i;
            validEntry["reply"] = replyText;
            malformed["reviews"].push_back(validEntry);
        }
        writeTestSave(malformed);
        recoveredReviews = saves.load(data);
        require(recoveredReviews.reviews.size() == 100 && recoveredReviews.reviews.front().id == 11 &&
                    recoveredReviews.reviews.back().id == 110 && recoveredReviews.nextReviewId == 111,
                "Loaded review history did not retain the newest 100 entries");
        const auto blockedFolder = temp / "restart-blocked";
        { std::ofstream blocker(blockedFolder); blocker << "not a directory"; }
        tiem::GameServices blockedGame(root, blockedFolder);
        blockedGame.progress = progress;
        blockedGame.hasResult = true;
        require(!blockedGame.restart() && blockedGame.progress.day == progress.day &&
                blockedGame.progress.money == progress.money && blockedGame.hasResult &&
                blockedGame.progress.unlocked == progress.unlocked && blockedGame.progress.levels == progress.levels,
                "Failed restart destroyed existing in-memory progress");
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
        {
            // Quà hằng ngày: nhận một lần mỗi ngày, chuỗi liên tiếp tăng, bỏ ngày thì về mốc đầu.
            auto p = tiem::Progress::fresh(data);
            const std::int64_t d0 = 20000;
            require(tiem::DailyReward::canClaim(p, d0), "Fresh save cannot claim daily reward");
            const int first = tiem::DailyReward::claim(data, p, d0);
            require(first > 0, "Daily reward gave nothing");
            require(tiem::DailyReward::claim(data, p, d0) == 0, "Daily reward claimed twice in a day");
            require(!tiem::DailyReward::canClaim(p, d0), "Reward claimable again same day");
            require(tiem::DailyReward::canClaim(p, d0 + 1), "Reward not claimable next day");
            require(tiem::DailyReward::claim(data, p, d0 + 1) >= first, "Streak reward decreased");
            require(tiem::DailyReward::nextIndex(p, d0 + 10) == 0, "Missed days did not reset streak");
            // Mục tiêu mỗi ngày luôn có và có thưởng; thành tựu chỉ mở một lần.
            const auto goals = tiem::DaySession::makeGoals(data, 3);
            require(!goals.empty(), "No daily goals");
            for (const auto &g : goals) require(g.target > 0 && g.reward > 0, "Invalid daily goal");
            auto q = tiem::Progress::fresh(data);
            tiem::AchievementSystem::check(data, q);
            require(tiem::AchievementSystem::check(data, q).empty(), "Achievements unlocked twice");
        }
        // ---- Tính năng mới: đồng hồ 7h-21h, thời tiết, kho nguyên liệu, tính cách khách, nhịp trộn ngẫu nhiên ----
        {
            const tiem::DayClock clock(180);
            require(clock.hour(180) == 7.f && clock.hour(0) == 21.f && std::abs(clock.hour(90) - 14.f) < .001f,
                    "Day clock must run from 07:00 to 21:00");
            require(tiem::DayClock::format(7.5f) == "07:30" && tiem::DayClock::format(20.99f) == "20:50" &&
                    tiem::DayClock::format(21.f) == "21:00", "Clock text format wrong");
            require(clock.slot(180) == 0 && clock.slot(0) == tiem::DayClock::hours - 1 && clock.slot(.01f) == 13,
                    "Hour slots out of range");
            require(tiem::DayClock::light(12, 0).night == 0.f && tiem::DayClock::light(21, 0).night > .8f &&
                    tiem::DayClock::light(20, 0).night > tiem::DayClock::light(18, 0).night &&
                    tiem::DayClock::light(12, 1).tint.r < tiem::DayClock::light(12, 0).tint.r,
                    "Scene must darken towards night and under rain");
            require(tiem::DayClock::trafficFactor(12) > 1.f && tiem::DayClock::trafficFactor(7.2f) < 1.f, "Traffic curve wrong");
        }
        {
            const auto first = tiem::WeatherSystem::forecast(123, 1);
            require(first.base == tiem::Weather::Sunny && !first.changes(), "Day one must be sunny");
            std::set<int> kinds;
            int changing = 0;
            for (int dayNumber = 2; dayNumber < 300; ++dayNumber) {
                const auto plan = tiem::WeatherSystem::forecast(77, dayNumber);
                const auto again = tiem::WeatherSystem::forecast(77, dayNumber);
                require(plan.base == again.base && plan.begin == again.begin && plan.end == again.end, "Forecast not deterministic");
                kinds.insert(static_cast<int>(plan.base));
                if (plan.changes()) {
                    ++changing;
                    require(plan.begin >= 0 && plan.end <= 1 && plan.end > plan.begin && plan.interlude != plan.base,
                            "Weather interlude invalid");
                    require(plan.at((plan.begin + plan.end) / 2) == plan.interlude && plan.at(plan.begin - .01f) == plan.base,
                            "Weather plan lookup wrong");
                }
                require(plan.rainShare() >= 0 && plan.rainShare() <= 1, "Rain share outside 0..1");
            }
            require(kinds.size() == 3 && changing > 60 && changing < 200, "Weather variety looks wrong");
            const auto rain = tiem::WeatherSystem::effect(tiem::Weather::Rainy);
            const auto sun = tiem::WeatherSystem::effect(tiem::Weather::Sunny);
            require(rain.spawn < 1 && rain.patience > 1 && rain.tip > 0 && sun.spawn > 1 && sun.freshBias > 0,
                    "Weather effects inverted");
            require(tiem::WeatherSystem::toppingWeight("fresh", .35f) > tiem::WeatherSystem::toppingWeight("dry", .35f) &&
                    tiem::WeatherSystem::toppingWeight("fresh", -.3f) < tiem::WeatherSystem::toppingWeight("sauce", -.3f),
                    "Weather taste preference wrong");
            require(!tiem::WeatherSystem::summary(tiem::WeatherSystem::forecast(77, 5)).empty(), "Weather summary empty");
        }
        {
            auto stocked = tiem::Progress::fresh(data);
            for (const auto &t : data.toppings)
                require(tiem::InventorySystem::stock(stocked, t.id) == (t.initial ? 8 : 0), "Starter stock wrong");
            const int money = stocked.money;
            const auto &mango = data.topping("xoai_song");
            require(tiem::InventorySystem::unitCost(data, mango) == 1000 && tiem::InventorySystem::rushCost(data, mango) == 2000,
                    "Ingredient prices wrong");
            require(tiem::InventorySystem::buy(data, stocked, "xoai_song", 5) == 5 && stocked.money == money - 5000 &&
                    stocked.todayCost == 5000 && tiem::InventorySystem::stock(stocked, "xoai_song") == 13, "Buying stock failed");
            require(tiem::InventorySystem::buy(data, stocked, "kho_bo", 5) == 0, "Locked topping could be stocked");
            stocked.money = 1000000;
            require(tiem::InventorySystem::buy(data, stocked, "muoi_tom", 1000) == tiem::InventorySystem::maxStock(data) - 8,
                    "Stock cap not applied");
            auto broke = tiem::Progress::fresh(data);
            broke.money = 900;
            require(tiem::InventorySystem::buy(data, broke, "xoai_song", 1) == 0 && broke.money == 900, "Bought without money");
            require(!tiem::InventorySystem::buyCart(data, broke, {{"muoi_tom", 1}, {"xoai_song", 1}}) &&
                    tiem::InventorySystem::stock(broke, "muoi_tom") == 8 && broke.money == 900, "Cart purchase is not atomic");
            require(tiem::InventorySystem::take(stocked, "rau_ram", 3) == 3 && tiem::InventorySystem::take(stocked, "rau_ram", 99) == 5 &&
                    tiem::InventorySystem::take(stocked, "rau_ram", 1) == 0, "Taking stock wrong");
            tiem::InventorySystem::restore(data, stocked, "rau_ram", 2);
            const auto lost = tiem::InventorySystem::spoil(data, stocked);
            require(lost.portions == 13 + 8 + 2 && tiem::InventorySystem::stock(stocked, "xoai_song") == 0 &&
                    tiem::InventorySystem::stock(stocked, "hanh_phi") == 8 && lost.value > 0,
                    "Only fresh ingredients may spoil overnight");
            auto planner = tiem::Progress::fresh(data);
            planner.stock.clear();
            const auto plan = tiem::WeatherSystem::forecast(planner.worldSeed, 3);
            const auto cart = tiem::InventorySystem::suggest(data, planner, plan);
            require(!cart.empty() && tiem::InventorySystem::cost(data, cart) <= planner.money * .6f + 1,
                    "Suggested cart empty or over budget");
            for (const auto &item : cart) require(planner.unlocked.count(item.first) && item.second > 0, "Suggested a locked item");
            // Một ngày chơi: tiền nhập hàng được ghi vào bảng kết quả, đồ tươi hỏng, thống kê theo giờ khớp tổng.
            auto dayProgress = tiem::Progress::fresh(data);
            dayProgress.money = 100000;
            tiem::InventorySystem::buy(data, dayProgress, "trung_cut", 4);
            tiem::DaySession session(data, dayProgress, 21);
            tiem::Bowl dish;
            dish.mixed = true;
            dish.paperQuality = dish.mixQuality = 1;
            for (const auto &item : session.customers().front().order.recipe)
                for (int n = 0; n < item.second; ++n) dish.portions.push_back(item.first);
            session.update(5);
            session.serve(0, dish);
            session.update(200);
            const auto &result = session.result();
            int servedSum = 0, leftSum = 0;
            for (int i = 0; i < tiem::DayClock::hours; ++i) {
                servedSum += result.servedByHour[static_cast<std::size_t>(i)];
                leftSum += result.leftByHour[static_cast<std::size_t>(i)];
            }
            require(result.ingredientCost == 6000 && dayProgress.todayCost == 0 && result.spoiledPortions == 8 + 8 + 8 + 4 &&
                    tiem::InventorySystem::stock(dayProgress, "trung_cut") == 0 && tiem::InventorySystem::stock(dayProgress, "muoi_tom") == 8,
                    "Ingredient ledger or spoilage wrong at closing time");
            require(servedSum == result.served && leftSum == result.left && result.bestScore > 0, "Hourly stats do not add up");
            require(result.profit() == result.revenue + result.tips + result.goalBonus - result.ingredientCost, "Profit formula wrong");
            require(std::string("SABCD").find(result.grade()) != std::string::npos, "Grade outside S..D");
        }
        {
            require(data.personalities.size() == 7 && data.personalities.front().id == "normal" &&
                    data.personalities.front().strict == 1 && data.personalities.front().tip == 1, "Personalities not loaded");
            const auto picky = data.personalityIndex("picky"), generous = data.personalityIndex("generous");
            require(data.personalities[picky].strict > 1 && data.personalities[generous].tip > 1, "Personality factors wrong");
            tiem::Customer guest;
            guest.patience = guest.remaining = 60;
            guest.order.recipe = {{"muoi_tom", 2}, {"xoai_song", 1}};
            guest.order.price = 20000;
            tiem::Bowl near;
            near.portions = {"muoi_tom", "xoai_song"};
            near.paperQuality = near.mixQuality = 1;
            near.mixed = true;
            const auto plain = tiem::EconomySystem::score(data, progress, guest, near);
            guest.personality = picky;
            const auto strict = tiem::EconomySystem::score(data, progress, guest, near);
            guest.personality = generous;
            const auto rich = tiem::EconomySystem::score(data, progress, guest, near);
            require(strict.recipe < plain.recipe && rich.tip > plain.tip, "Picky or generous personality has no effect");
            std::set<std::size_t> seenPersonalities;
            for (unsigned seed = 0; seed < 300; ++seed) {
                auto visitProgress = tiem::Progress::fresh(data);
                tiem::DaySession visit(data, visitProgress, seed);
                seenPersonalities.insert(visit.customers().front().personality);
            }
            require(seenPersonalities.size() == data.personalities.size(), "Some personalities never appear");
            const auto &hot = data.personalities[data.personalityIndex("hot_tempered")];
            const auto &calm = data.personalities[data.personalityIndex("easygoing")];
            tiem::Customer waiting;
            waiting.patience = 100;
            waiting.remaining = 99;
            require(tiem::CustomerMood::waiting(waiting, hot) == tiem::Expression::Happy, "New customer should greet happily");
            waiting.remaining = 40;
            require(tiem::CustomerMood::waiting(waiting, hot) == tiem::Expression::Angry &&
                    tiem::CustomerMood::waiting(waiting, calm) != tiem::Expression::Angry,
                    "Hot-tempered customers must anger sooner than easygoing ones");
            waiting.remaining = 25;
            require(tiem::CustomerMood::waiting(waiting, calm) == tiem::Expression::Bored, "Easygoing customer should look bored");
            require(tiem::CustomerMood::reaction(5, hot) == tiem::Expression::Delighted &&
                    tiem::CustomerMood::reaction(4, data.personalities[picky]) == tiem::Expression::Neutral &&
                    tiem::CustomerMood::reaction(1, hot) == tiem::Expression::Sad, "Serve reaction wrong");
            for (int e = 0; e <= static_cast<int>(tiem::Expression::Surprised); ++e) {
                const std::string face = tiem::CustomerMood::face(static_cast<tiem::Expression>(e));
                require(face == "happy" || face == "normal" || face == "angry", "Expression maps to missing artwork");
                require(!tiem::CustomerMood::line(hot, static_cast<tiem::Expression>(e), 3).empty(), "Missing dialogue line");
            }
        }
        {
            std::set<int> rhythms;
            for (unsigned seed = 0; seed < 40; ++seed) {
                tiem::MixingMinigame a(data, 0, 3, seed), b(data, 0, 3, seed);
                for (int beat = 0; beat < data.integer("mix_hits"); ++beat) {
                    for (int f = 0; f < 37; ++f) { a.update(.017f); b.update(.017f); }
                    a.hit();
                    b.hit();
                    require(a.target() == b.target() && a.rhythm() == b.rhythm() && a.position() == b.position(),
                            "Mixing rhythm not reproducible from its seed");
                    require(a.target() - a.width() / 2 >= 0 && a.target() + a.width() / 2 <= 1 && a.width() > 0,
                            "Random mixing zone outside the bar");
                    rhythms.insert(static_cast<int>(a.rhythm()));
                }
            }
            require(rhythms.size() >= 4, "Mixing rhythm is not varied");
        }
        {
            const auto folder = std::filesystem::temp_directory_path() /
                ("tiem_feature_save_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
            tiem::SaveSystem store(folder);
            auto saved = tiem::Progress::fresh(data);
            saved.guideDone = true;
            saved.stock["muoi_tom"] = 17;
            saved.todayCost = 4200;
            saved.worldSeed = 987654321u;
            require(store.write(saved), "Feature save failed");
            const auto loaded = store.load(data);
            require(loaded.guideDone && tiem::InventorySystem::stock(loaded, "muoi_tom") == 17 && loaded.todayCost == 4200 &&
                    loaded.worldSeed == 987654321u, "New progress fields did not round-trip");
            std::filesystem::remove_all(folder);
        }
        std::cout << "PASS: data, recipe scoring, upgrades, paper, mixing, customers, day loop, save "
                     "recovery, UTF-8, input and state lifetime\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

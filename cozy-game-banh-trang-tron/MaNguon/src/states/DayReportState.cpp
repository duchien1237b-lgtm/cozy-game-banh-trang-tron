#include "states/DayReportState.hpp"
#include "gameplay/CustomerMood.hpp"
#include "gameplay/Inventory.hpp"
#include "ui/Art.hpp"
#include "ui/Atmosphere.hpp"
#include "core/Utf8.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace tiem {
namespace {
float easeOutBack(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1.f;
    t = std::clamp(t, 0.f, 1.f);
    return 1.f + c3 * std::pow(t - 1.f, 3.f) + c1 * std::pow(t - 1.f, 2.f);
}
const char *gradeTitle(char grade) {
    switch (grade) {
    case 'S': return "Một ngày rực rỡ!";
    case 'A': return "Tiệm đắt khách quá!";
    case 'B': return "Một ngày ổn áp";
    case 'C': return "Còn chỗ để tiến bộ";
    default: return "Ngày hơi vất vả";
    }
}
sf::Color gradeColor(char grade) {
    switch (grade) {
    case 'S': return {226, 168, 58};
    case 'A': return {96, 150, 86};
    case 'B': return {70, 140, 180};
    case 'C': return {200, 130, 70};
    default: return {170, 110, 100};
    }
}
std::string signedMoney(int value) { return (value < 0 ? "−" : "+") + moneyText(std::abs(value)); }
void reportFrame(sf::RenderTarget &target, ResourceManager &resources, sf::FloatRect box,
                 const char *title, sf::Color accent, sf::Color tint, int icon) {
    // Same wooden edge, inset paper and header height across the three columns.
    ui::shadowPanel(target, box, {255, 250, 235}, 22, 7, 60);
    ui::panelOutline(target, box, {143, 96, 59}, 22, 3);
    ui::panelOutline(target, {box.left + 5, box.top + 5, box.width - 10, box.height - 10},
                     {224, 195, 143}, 18, 1);
    ui::panel(target, {box.left + 12, box.top + 12, box.width - 24, 46}, tint, 13);
    const sf::Vector2f center{box.left + 36, box.top + 35};
    if (icon == 0) ui::star(target, center, 12, accent);
    else if (icon == 1) ui::image(target, resources, "ui/coin_icon", {center.x - 13, center.y - 13, 26, 26});
    else ui::checkMark(target, center, 11, accent);
    ui::text(target, resources.titleFont(), title, 23, {box.left + 58, box.top + 20}, accent);
    // Small brass pins sit in the paper margin, clear of the content.
    for (float x : {box.left + 13, box.left + box.width - 13}) {
        sf::CircleShape pin(2.5f, 12);
        pin.setOrigin(2.5f, 2.5f);
        pin.setPosition(x, box.top + box.height - 13);
        pin.setFillColor({186, 143, 80});
        target.draw(pin);
    }
}
} // namespace

DayReportState::DayReportState(StateStack &stack, AppContext &context) : UiState(stack, context) {
    if (context_.audio) context_.audio->music("menu");
    addButton("Về menu", {30, 650, 170, 46}, [this] { returnToMenu(); });
    addButton("Cửa hàng nâng cấp", {700, 650, 250, 46}, [this] { stack_.replace(StateID::UpgradeShop); });
    addButton("Chuẩn bị ngày " + std::to_string(game().progress.day), {965, 650, 285, 46}, [this] { stack_.replace(StateID::Prep); });
}
float DayReportState::count(float delay, float seconds) const {
    return ui::easeOut(std::clamp((age_ - delay) / seconds, 0.f, 1.f));
}
bool DayReportState::handleEvent(const sf::Event &e, Pointer mouse) {
    pointer_ = mouse;
    if (e.type == sf::Event::KeyPressed && game().progress.keys.matches("start", e.key.code)) {
        sound("ui_click");
        stack_.replace(StateID::Prep);
        return false;
    }
    handleButtons(e, mouse);
    return false;
}
bool DayReportState::update(float dt) {
    age_ += dt;
    effects_.update(dt);
    const auto &r = game().lastDay;
    for (unsigned i = 0; i < 5; ++i)
        if (!(starsPopped_ & (1u << i)) && age_ > .45f + static_cast<float>(i) * .15f) {
            starsPopped_ |= 1u << i;
            if (static_cast<float>(i) + .5f <= r.rating()) {
                sound("ingredient_pop");
                effects_.burst({128.f + static_cast<float>(i) * 44.f, 340}, {255, 214, 98}, 6);
            }
        }
    if (!celebrated_ && age_ > 1.3f) {
        celebrated_ = true;
        if (r.grade() == 'S' || r.grade() == 'A') {
            effects_.sparkle({200, 210}, {255, 214, 98}, 34);
            sound("achievement");
        }
    }
    return UiState::update(dt);
}
void DayReportState::drawGrade(sf::RenderTarget &target, sf::FloatRect box) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &titleFont = context_.resources.titleFont();
    const auto &r = game().lastDay;
    reportFrame(target, context_.resources, box, "Thành quả hôm nay", {148, 101, 39}, {246, 226, 178}, 0);
    const char grade = r.grade();
    const sf::Color color = gradeColor(grade);
    // Huy chương xếp hạng: nảy vào và tỏa sáng.
    const float pop = easeOutBack((age_ - .15f) / .45f);
    const sf::Vector2f medal{box.left + box.width / 2, box.top + 130};
    ui::radial(target, medal, {96 * pop, 96 * pop}, {color.r, color.g, color.b, 70}, {color.r, color.g, color.b, 0}, true);
    sf::CircleShape ring(62.f * std::max(0.f, pop), 48);
    ring.setOrigin(ring.getRadius(), ring.getRadius());
    ring.setPosition(medal);
    ring.setFillColor(color);
    ring.setOutlineColor({255, 250, 232});
    ring.setOutlineThickness(5.f);
    target.draw(ring);
    if (pop > .2f) {
        ui::outlinedText(target, titleFont, std::string(1, grade), static_cast<unsigned>(64 * std::min(1.f, pop)), medal + sf::Vector2f(0, -6),
                         {255, 252, 238}, sf::Color(static_cast<sf::Uint8>(std::max(0, color.r - 70)), static_cast<sf::Uint8>(std::max(0, color.g - 70)), static_cast<sf::Uint8>(std::max(0, color.b - 70))), 3.f);
    }
    ui::text(target, titleFont, gradeTitle(grade), 24, {medal.x, box.top + 210}, theme.ink, true);
    for (int i = 0; i < 5; ++i) {
        const float p = (starsPopped_ & (1u << i)) ? easeOutBack((age_ - (.45f + static_cast<float>(i) * .15f)) / .3f) : 0.f;
        const bool lit = static_cast<float>(i) + .5f <= r.rating();
        ui::star(target, {medal.x + (static_cast<float>(i) - 2.f) * 44.f, box.top + 248}, 17 * std::max(0.f, p),
                 lit ? sf::Color(232, 176, 60) : sf::Color(224, 217, 194));
    }
    std::ostringstream rating;
    rating << std::fixed << std::setprecision(1) << r.rating() << " / 5 sao";
    ui::text(target, font, rating.str(), 16, {medal.x, box.top + 276}, theme.green, true);
    // Sáu ô chỉ số.
    const int average = r.served > 0 ? static_cast<int>(std::lround(r.scoreSum / static_cast<float>(r.served) * 100.f)) : 0;
    const std::pair<std::string, std::string> tiles[] = {
        {"Phục vụ", std::to_string(r.served)}, {"Bỏ về", std::to_string(r.left)},
        {"Combo cao nhất", "x" + std::to_string(r.bestCombo)}, {"Trộn Perfect", std::to_string(r.perfectMixes)},
        {"Điểm món TB", std::to_string(average) + "/100"}, {"Khách 5 sao", std::to_string(r.delighted)}};
    for (int i = 0; i < 6; ++i) {
        const float tileWidth = (box.width - 48) / 2;
        const sf::FloatRect tile{box.left + 18 + static_cast<float>(i % 2) * (tileWidth + 12),
                                 box.top + 302 + static_cast<float>(i / 2) * 62, tileWidth, 54};
        ui::panel(target, tile, i == 1 ? sf::Color(249, 232, 216) : sf::Color(247, 237, 208), 12);
        ui::panelOutline(target, tile, {231, 211, 172}, 12, 1);
        ui::text(target, font, tiles[i].first, 12, {tile.left + tile.width / 2, tile.top + 12}, theme.muted, true);
        ui::text(target, titleFont, tiles[i].second, 22, {tile.left + tile.width / 2, tile.top + 35},
                 i == 1 && r.left > 0 ? theme.red : theme.ink, true);
    }
    // Thời tiết của ngày.
    ui::panel(target, {box.left + 18, box.top + box.height - 58, box.width - 36, 44}, {237, 234, 214}, 12);
    ui::weatherIcon(target, r.weather.base, {box.left + 40, box.top + box.height - 34}, 34, age_);
    ui::textFit(target, font, WeatherSystem::summary(r.weather), 14, {box.left + 66, box.top + box.height - 44}, theme.ink, box.width - 84);
    ui::text(target, font, "07:00 – 21:00", 12, {box.left + 66, box.top + box.height - 24}, theme.muted);
}
void DayReportState::drawLedger(sf::RenderTarget &target, sf::FloatRect box) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &titleFont = context_.resources.titleFont();
    const auto &r = game().lastDay;
    reportFrame(target, context_.resources, box, "Sổ thu chi", theme.green, {220, 233, 199}, 1);
    ui::text(target, font, "NGÀY " + std::to_string(r.day), 13, {box.left + 26, box.top + 75}, theme.green);
    ui::text(target, font, "Kết sổ lúc 21:00", 12, {box.left + 26, box.top + 94}, theme.muted);
    struct Row { std::string label; int value; sf::Color color; float delay; };
    std::vector<Row> rows;
    const int plainTips = r.tips - r.comboBonus - r.rushBonus - r.weatherBonus;
    rows.push_back({"Tiền món", r.revenue, theme.ink, .5f});
    rows.push_back({"Tiền tip", plainTips, theme.ink, .6f});
    if (r.comboBonus > 0) rows.push_back({"Thưởng combo", r.comboBonus, {155, 114, 34}, .7f});
    if (r.rushBonus > 0) rows.push_back({"Thưởng giờ cao điểm", r.rushBonus, {182, 102, 45}, .75f});
    if (r.weatherBonus > 0) rows.push_back({"Tip khách trú mưa", r.weatherBonus, {70, 120, 150}, .8f});
    if (r.goalBonus > 0) rows.push_back({"Thưởng nhiệm vụ", r.goalBonus, theme.green, .85f});
    rows.push_back({"Nhập nguyên liệu", -r.ingredientCost, theme.red, .95f});
    float y = box.top + 124;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const auto &row = rows[i];
        if (i % 2 == 0) ui::panel(target, {box.left + 18, y - 5, box.width - 36, 35}, {244, 237, 216}, 8);
        ui::textFit(target, font, row.label, 16, {box.left + 28, y}, theme.muted, box.width - 215);
        const std::string value = signedMoney(static_cast<int>(std::lround(static_cast<float>(row.value) * count(row.delay) / 100.f)) * 100);
        const float valueWidth = std::min(164.f, ui::textWidth(font, value, 17));
        ui::textFit(target, font, value, 17, {box.left + box.width - 28 - valueWidth, y}, row.color, valueWidth);
        y += 40;
    }
    // Spoilage is informational: its purchase cost is already included above.
    if (r.spoiledPortions > 0) {
        const float noteY = box.top + 410;
        ui::text(target, font, "Đồ tươi bỏ đi: " + std::to_string(r.spoiledPortions) + " muỗng", 13,
                 {box.left + 28, noteY}, theme.red);
        const std::string lost = "(" + moneyText(r.spoiledValue) + ")";
        const float lostWidth = std::min(128.f, ui::textWidth(font, lost, 13));
        ui::textFit(target, font, lost, 13, {box.left + box.width - 28 - lostWidth, noteY}, theme.red, lostWidth);
    }
    const float totalY = box.top + box.height - 103;
    for (float x = box.left + 24; x < box.left + box.width - 24; x += 10)
        ui::panel(target, {x, totalY - 12, 5, 2}, {194, 177, 143}, 1);
    const int profit = r.profit();
    const sf::Color profitColor = profit >= 0 ? theme.green : theme.red;
    ui::panel(target, {box.left + 18, totalY, box.width - 36, 72},
              profit >= 0 ? sf::Color(222, 235, 200) : sf::Color(247, 220, 207), 14);
    ui::text(target, titleFont, "Lãi ròng", 23, {box.left + 32, totalY + 12}, profitColor);
    const std::string profitText = signedMoney(static_cast<int>(std::lround(static_cast<float>(profit) * count(1.05f, 1.1f) / 100.f)) * 100);
    const float profitWidth = std::min(box.width - 184, ui::textWidth(titleFont, profitText, 27));
    ui::textFit(target, titleFont, profitText, 27, {box.left + box.width - 32 - profitWidth, totalY + 10},
                profitColor, profitWidth);
    ui::text(target, font, "Sau chi phí nhập nguyên liệu", 12, {box.left + 32, totalY + 47}, profitColor);
    ui::text(target, font, "Góc Nhỏ Bánh Tráng • Hẹn ngày mai", 12,
             {box.left + box.width / 2, box.top + box.height - 17}, theme.muted, true);
    if (age_ > 1.6f) {
        const float stamp = easeOutBack((age_ - 1.6f) / .3f);
        const sf::Vector2f center{box.left + box.width - 99, box.top + 87};
        sf::RectangleShape seal({132 * stamp, 28 * stamp});
        seal.setOrigin(66 * stamp, 14 * stamp);
        seal.setPosition(center);
        seal.setRotation(-7);
        seal.setFillColor(sf::Color::Transparent);
        seal.setOutlineColor({95, 133, 80, 165});
        seal.setOutlineThickness(2);
        target.draw(seal);
        sf::Text label(utf8("ĐÃ CHỐT SỔ"), titleFont, static_cast<unsigned>(16 * stamp));
        const auto b = label.getLocalBounds();
        label.setOrigin(b.left + b.width / 2, b.top + b.height / 2);
        label.setPosition(center);
        label.setRotation(-7);
        label.setFillColor({79, 111, 76, 200});
        target.draw(label);
    }
}
void DayReportState::drawSide(sf::RenderTarget &target, sf::FloatRect box) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &titleFont = context_.resources.titleFont();
    const auto &r = game().lastDay;
    const auto &d = game().data;
    reportFrame(target, context_.resources, box, "Dấu ấn trong ngày", {164, 91, 55}, {245, 221, 193}, 2);
    ui::text(target, titleFont, "Nhiệm vụ", 20, {box.left + 22, box.top + 70}, theme.green);
    const auto done = std::count_if(r.goals.begin(), r.goals.end(), [](const auto &g) { return g.done(); });
    ui::text(target, font, std::to_string(done) + "/" + std::to_string(r.goals.size()) + " hoàn thành", 12,
             {box.left + box.width - 119, box.top + 77}, theme.muted);
    for (std::size_t i = 0; i < r.goals.size() && i < 3; ++i) {
        const auto &goal = r.goals[i];
        const float top = box.top + 99 + static_cast<float>(i) * 50;
        const float appear = count(.5f + static_cast<float>(i) * .2f, .4f);
        ui::panel(target, {box.left + 14, top, box.width - 28, 44}, goal.done() ? sf::Color(226, 238, 208) : sf::Color(244, 238, 222), 12);
        if (goal.done()) ui::checkMark(target, {box.left + 34, top + 22}, 10 * appear, theme.green);
        else {
            sf::CircleShape miss(10.f);
            miss.setOrigin(10, 10);
            miss.setPosition(box.left + 34, top + 22);
            miss.setFillColor({214, 205, 184});
            target.draw(miss);
        }
        ui::textFit(target, font, goalText(goal), 14, {box.left + 52, top + 6}, theme.ink, box.width - 76);
        const float ratio = goal.id == "nolose" ? (goal.done() ? 1.f : 0.f)
                            : static_cast<float>(std::min(goal.progress, goal.target)) / static_cast<float>(std::max(1, goal.target));
        ui::ProgressBar::draw(target, {box.left + 52, top + 30, box.width - 160, 6}, ratio * appear, goal.done() ? theme.green : theme.orange);
        const std::string reward = goal.done() ? "+" + moneyText(goal.reward) : goalProgressText(goal);
        ui::text(target, font, reward, 13, {box.left + box.width - 24 - ui::textWidth(font, reward, 13), top + 24}, goal.done() ? theme.green : theme.muted);
    }
    // Khách nổi bật trong ngày.
    const float y = box.top + 259;
    ui::text(target, titleFont, "Khách của ngày", 20, {box.left + 20, y}, theme.orange);
    const sf::FloatRect spot{box.left + 14, y + 30, box.width - 28, 124};
    ui::panel(target, spot, {248, 234, 205}, 14);
    ui::panelOutline(target, spot, {228, 200, 149}, 14, 1);
    if (r.served > 0 && r.bestScore >= 0 && r.bestKind < d.customers.size()) {
        const auto &kind = d.customers[r.bestKind];
        const auto &persona = d.personalities.at(std::min(r.bestPersonality, d.personalities.size() - 1));
        const auto expression = r.bestScore >= .9f ? Expression::Delighted : Expression::Happy;
        ui::image(target, context_.resources, "customers/" + kind.style + "_happy", {spot.left + 4, spot.top + 4 + std::sin(age_ * 3.f) * 2.f, 104, 104});
        ui::emote(target, font, expression, {spot.left + 82, spot.top + 24}, age_, .8f);
        ui::textFit(target, titleFont, kind.name, 19, {spot.left + 118, spot.top + 8}, theme.ink, spot.width - 132);
        const float tw = ui::textWidth(context_.resources.smallFont(), persona.name, 12) + 16;
        ui::panel(target, {spot.left + 118, spot.top + 38, tw, 20}, persona.color, 10);
        ui::text(target, context_.resources.smallFont(), persona.name, 12, {spot.left + 118 + tw / 2, spot.top + 47}, {255, 252, 240}, true);
        ui::text(target, font, "Điểm " + std::to_string(static_cast<int>(std::lround(r.bestScore * 100))) + "/100", 13,
                 {spot.left + 118, spot.top + 62}, theme.green);
        const std::string quote = persona.happy.empty() ? "Ngon lắm!" : "“" + persona.happy[static_cast<std::size_t>(r.day) % persona.happy.size()] + "”";
        ui::textFit(target, context_.resources.dialogFont(), quote, 14, {spot.left + 118, spot.top + 83}, theme.muted, spot.width - 132);
        ui::textFit(target, font, std::to_string(r.delighted) + " khách mê tít  •  " + std::to_string(r.unhappy + r.left) + " khách không vui", 11,
                 {spot.left + 118, spot.top + 106}, theme.muted, spot.width - 132);
    } else {
        ui::paragraph(target, font, "Hôm nay chưa phục vụ được ai. Mai thử nhập hàng vừa đủ và chọn khách sắp hết kiên nhẫn trước nhé!", 14,
                      {spot.left + 14, spot.top + 18}, spot.width - 28, 20, theme.muted);
    }
    // Thành tựu mới / kho còn lại.
    const float ay = box.top + 431;
    if (!r.newAchievements.empty()) {
        ui::text(target, titleFont, "Thành tựu mới", 18, {box.left + 20, ay}, theme.orange);
        for (std::size_t i = 0; i < r.newAchievements.size() && i < 2; ++i) {
            const float top = ay + 30 + static_cast<float>(i) * 40;
            ui::panel(target, {box.left + 14, top, box.width - 28, 34}, {252, 238, 190}, 10);
            ui::star(target, {box.left + 34, top + 17}, 11, theme.gold);
            for (const auto &info : AchievementSystem::all())
                if (info.id == r.newAchievements[i]) ui::textFit(target, font, info.name + " — " + info.description, 13, {box.left + 52, top + 8}, theme.ink, box.width - 80);
        }
    } else {
        ui::text(target, titleFont, "Kho để dành", 18, {box.left + 20, ay}, theme.orange);
        int kept = 0, kinds = 0;
        for (const auto &item : game().progress.stock)
            if (item.second > 0) { kept += item.second; ++kinds; }
        ui::paragraph(target, font, kept > 0 ? "Còn " + std::to_string(kept) + " muỗng đồ khô/sốt (" + std::to_string(kinds) + " loại) chuyển sang ngày mai."
                                              : "Kho trống. Nhớ nhập hàng trước khi mở quán ngày mai.",
                      13, {box.left + 20, ay + 30}, box.width - 40, 18, theme.muted);
    }
}
void DayReportState::render(sf::RenderTarget &target) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &r = game().lastDay;
    // Nền hoàng hôn → đêm: ngày đã khép lại lúc 21:00.
    ui::gradientRect(target, {0, 0, 1280, 720}, {62, 64, 112}, {238, 176, 132});
    ui::radial(target, {640, 760}, {900, 380}, {255, 214, 150, 120}, {255, 214, 150, 0}, true);
    for (int i = 0; i < 40; ++i) {
        const float x = std::fmod(static_cast<float>(i) * 173.f, 1280.f), y = std::fmod(static_cast<float>(i) * 97.f, 220.f);
        const float tw = .5f + .5f * std::sin(age_ * 2.f + static_cast<float>(i));
        ui::radial(target, {x, y}, {2.5f, 2.5f}, {255, 250, 220, static_cast<sf::Uint8>(160 * tw)}, {255, 250, 220, 0}, true);
    }
    ui::outlinedText(target, context_.resources.titleFont(), "Tổng kết ngày " + std::to_string(r.day), 34, {640, 34}, {255, 246, 224}, {94, 53, 34}, 3.f);
    ui::text(target, font, "Tiệm đã đóng cửa lúc 21:00 • " + std::string(gradeTitle(r.grade())), 15, {640, 66}, {255, 240, 214}, true);
    drawGrade(target, {30, 92, 372, 546});
    drawLedger(target, {420, 92, 440, 546});
    drawSide(target, {878, 92, 372, 546});
    // Ví hiện tại.
    ui::shadowPanel(target, {216, 650, 300, 46}, {255, 250, 233, 235}, 16, 3, 40);
    ui::image(target, context_.resources, "ui/coin_icon", {228, 658, 30, 30});
    ui::text(target, font, "Ví: " + moneyText(game().progress.money), 20, {268, 660}, theme.green);
    renderButtons(target);
    effects_.render(target, font);
}
} // namespace tiem

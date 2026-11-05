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
    if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Enter) {
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
                effects_.burst({112.f + static_cast<float>(i) * 44.f, 338}, {255, 214, 98}, 6);
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
    ui::shadowPanel(target, box, theme.paper, 24, 6, 50);
    const char grade = r.grade();
    const sf::Color color = gradeColor(grade);
    // Huy chương xếp hạng: nảy vào và tỏa sáng.
    const float pop = easeOutBack((age_ - .15f) / .45f);
    const sf::Vector2f medal{box.left + box.width / 2, box.top + 104};
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
    ui::text(target, titleFont, gradeTitle(grade), 24, {medal.x, box.top + 196}, theme.ink, true);
    for (int i = 0; i < 5; ++i) {
        const float p = (starsPopped_ & (1u << i)) ? easeOutBack((age_ - (.45f + static_cast<float>(i) * .15f)) / .3f) : 0.f;
        const bool lit = static_cast<float>(i) + .5f <= r.rating();
        ui::star(target, {box.left + 82.f + static_cast<float>(i) * 44.f, box.top + 236}, 17 * std::max(0.f, p),
                 lit ? sf::Color(232, 176, 60) : sf::Color(224, 217, 194));
    }
    std::ostringstream rating;
    rating << std::fixed << std::setprecision(1) << r.rating() << " / 5 sao";
    ui::text(target, font, rating.str(), 16, {medal.x, box.top + 266}, theme.green, true);
    // Bốn ô chỉ số.
    const int average = r.served > 0 ? static_cast<int>(std::lround(r.scoreSum / static_cast<float>(r.served) * 100.f)) : 0;
    const std::pair<std::string, std::string> tiles[] = {
        {"Phục vụ", std::to_string(r.served)}, {"Bỏ về", std::to_string(r.left)},
        {"Combo cao nhất", "x" + std::to_string(r.bestCombo)}, {"Trộn Perfect", std::to_string(r.perfectMixes)},
        {"Điểm món TB", std::to_string(average) + "/100"}, {"Khách 5 sao", std::to_string(r.delighted)}};
    for (int i = 0; i < 6; ++i) {
        const sf::FloatRect tile{box.left + 18 + static_cast<float>(i % 2) * 168, box.top + 294 + static_cast<float>(i / 2) * 64, 158, 56};
        ui::panel(target, tile, {250, 241, 214}, 14);
        ui::text(target, font, tiles[i].first, 12, {tile.left + tile.width / 2, tile.top + 12}, theme.muted, true);
        ui::text(target, titleFont, tiles[i].second, 22, {tile.left + tile.width / 2, tile.top + 35},
                 i == 1 && r.left > 0 ? theme.red : theme.ink, true);
    }
    // Thời tiết của ngày.
    ui::weatherIcon(target, r.weather.base, {box.left + 40, box.top + box.height - 34}, 34, age_);
    ui::textFit(target, font, WeatherSystem::summary(r.weather), 14, {box.left + 66, box.top + box.height - 44}, theme.ink, box.width - 84);
    ui::text(target, font, "07:00 – 21:00", 12, {box.left + 66, box.top + box.height - 24}, theme.muted);
}
void DayReportState::drawLedger(sf::RenderTarget &target, sf::FloatRect box) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &titleFont = context_.resources.titleFont();
    const auto &r = game().lastDay;
    // Sổ thu chi kiểu hóa đơn giấy: mép răng cưa phía trên.
    ui::shadowPanel(target, box, {255, 252, 242}, 18, 6, 50);
    for (float x = box.left + 10; x < box.left + box.width - 10; x += 16) {
        sf::CircleShape notch(5.f, 12);
        notch.setOrigin(5, 5);
        notch.setPosition(x + 3, box.top);
        notch.setFillColor({247, 232, 205});
        target.draw(notch);
    }
    ui::text(target, titleFont, "Sổ thu chi", 22, {box.left + 22, box.top + 16}, theme.ink);
    struct Row { std::string label; int value; sf::Color color; float delay; };
    std::vector<Row> rows;
    const int plainTips = r.tips - r.comboBonus - r.rushBonus - r.weatherBonus;
    rows.push_back({"Tiền món", r.revenue, theme.ink, .5f});
    rows.push_back({"Tiền tip", plainTips, theme.ink, .6f});
    if (r.comboBonus > 0) rows.push_back({"Thưởng combo", r.comboBonus, theme.gold, .7f});
    if (r.rushBonus > 0) rows.push_back({"Thưởng giờ cao điểm", r.rushBonus, theme.orange, .75f});
    if (r.weatherBonus > 0) rows.push_back({"Tip khách trú mưa", r.weatherBonus, {70, 120, 170}, .8f});
    if (r.goalBonus > 0) rows.push_back({"Thưởng nhiệm vụ", r.goalBonus, theme.green, .85f});
    rows.push_back({"Nhập nguyên liệu", -r.ingredientCost, theme.red, .95f});
    float y = box.top + 56;
    for (const auto &row : rows) {
        ui::text(target, font, row.label, 16, {box.left + 24, y}, theme.muted);
        const std::string value = signedMoney(static_cast<int>(std::lround(static_cast<float>(row.value) * count(row.delay) / 100.f)) * 100);
        ui::text(target, font, value, 16, {box.left + box.width - 24 - ui::textWidth(font, value, 16), y}, row.color);
        y += 25;
    }
    if (r.spoiledPortions > 0) {
        ui::text(target, font, "Đồ tươi bỏ đi: " + std::to_string(r.spoiledPortions) + " muỗng", 13, {box.left + 24, y + 1}, theme.red);
        const std::string lost = "(" + moneyText(r.spoiledValue) + ")";
        ui::text(target, font, lost, 13, {box.left + box.width - 24 - ui::textWidth(font, lost, 13), y + 1}, theme.red);
        y += 21;
    }
    // Đường kẻ chấm kiểu hóa đơn.
    for (float x = box.left + 22; x < box.left + box.width - 22; x += 9)
        ui::panel(target, {x, y + 4, 5, 2}, {200, 186, 160}, 1);
    const int profit = r.profit();
    ui::text(target, titleFont, "Lãi ròng", 22, {box.left + 24, y + 14}, theme.ink);
    const std::string profitText = signedMoney(static_cast<int>(std::lround(static_cast<float>(profit) * count(1.05f, 1.1f) / 100.f)) * 100);
    ui::text(target, titleFont, profitText, 26, {box.left + box.width - 24 - ui::textWidth(titleFont, profitText, 26), y + 11},
             profit >= 0 ? theme.green : theme.red);
    // Dấu mộc "ĐÃ CHỐT SỔ".
    if (age_ > 1.6f) {
        const float stamp = easeOutBack((age_ - 1.6f) / .3f);
        sf::RectangleShape seal({132 * stamp, 34 * stamp});
        seal.setOrigin(66 * stamp, 17 * stamp);
        seal.setPosition(box.left + box.width - 96, box.top + 34);
        seal.setRotation(-10);
        seal.setFillColor(sf::Color::Transparent);
        seal.setOutlineColor({196, 84, 60, 170});
        seal.setOutlineThickness(3);
        target.draw(seal);
        sf::Text label(utf8("ĐÃ CHỐT SỔ"), titleFont, static_cast<unsigned>(17 * stamp));
        const auto b = label.getLocalBounds();
        label.setOrigin(b.left + b.width / 2, b.top + b.height / 2);
        label.setPosition(box.left + box.width - 96, box.top + 34);
        label.setRotation(-10);
        label.setFillColor({196, 84, 60, 190});
        target.draw(label);
    }
}
void DayReportState::drawTimeline(sf::RenderTarget &target, sf::FloatRect box) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &r = game().lastDay;
    ui::shadowPanel(target, box, theme.paper, 18, 5, 45);
    ui::text(target, context_.resources.titleFont(), "Khách theo giờ", 19, {box.left + 20, box.top + 12}, theme.green);
    // Chú thích.
    const float lx = box.left + box.width - 230;
    ui::panel(target, {lx, box.top + 20, 12, 12}, {120, 170, 100}, 3);
    ui::text(target, font, "Phục vụ", 12, {lx + 18, box.top + 18}, theme.muted);
    ui::panel(target, {lx + 82, box.top + 20, 12, 12}, {214, 110, 86}, 3);
    ui::text(target, font, "Bỏ về", 12, {lx + 100, box.top + 18}, theme.muted);
    ui::panel(target, {lx + 152, box.top + 20, 12, 12}, {170, 196, 230}, 3);
    ui::text(target, font, "Mưa", 12, {lx + 170, box.top + 18}, theme.muted);
    const sf::FloatRect chart{box.left + 22, box.top + 46, box.width - 44, box.height - 80};
    int peak = 1;
    for (int i = 0; i < DayClock::hours; ++i)
        peak = std::max(peak, r.servedByHour[static_cast<std::size_t>(i)] + r.leftByHour[static_cast<std::size_t>(i)]);
    const float slot = chart.width / static_cast<float>(DayClock::hours);
    for (int i = 0; i < DayClock::hours; ++i) {
        const float x = chart.left + slot * static_cast<float>(i);
        // Nền khung giờ: mưa tô xanh nhạt, trời tối dần về cuối ngày.
        const float mid = (static_cast<float>(i) + .5f) / static_cast<float>(DayClock::hours);
        const auto light = DayClock::light(DayClock::openHour + static_cast<float>(i) + .5f, 0.f);
        // Pha 50% với màu giấy để nền buổi tối nhạt, không bị nhầm thành cột số liệu.
        const auto soften = [](float base, sf::Uint8 tint) { return static_cast<sf::Uint8>(base * (.5f + .5f * tint / 255.f)); };
        sf::Color bg{soften(250, light.tint.r), soften(244, light.tint.g), soften(232, light.tint.b)};
        if (r.weather.at(mid) == Weather::Rainy) bg = {190, 208, 232};
        ui::panel(target, {x + 1, chart.top, slot - 2, chart.height}, bg, 4);
        const float grow = count(.6f + static_cast<float>(i) * .04f, .5f);
        const float served = static_cast<float>(r.servedByHour[static_cast<std::size_t>(i)]);
        const float left = static_cast<float>(r.leftByHour[static_cast<std::size_t>(i)]);
        const float unit = (chart.height - 20) / static_cast<float>(peak);
        const float hs = served * unit * grow, hl = left * unit * grow;
        if (hs > 0) ui::panel(target, {x + slot * .2f, chart.top + chart.height - hs, slot * .6f, hs}, {120, 170, 100}, 3);
        if (hl > 0) ui::panel(target, {x + slot * .2f, chart.top + chart.height - hs - hl, slot * .6f, hl}, {214, 110, 86}, 3);
        if (served + left > 0 && grow > .9f)
            ui::text(target, context_.resources.smallFont(), std::to_string(static_cast<int>(served + left)), 11,
                     {x + slot / 2, chart.top + chart.height - hs - hl - 9}, theme.ink, true);
        if (i % 2 == 0)
            ui::text(target, font, DayClock::slotLabel(i), 11, {x + slot / 2, chart.top + chart.height + 12}, theme.muted, true);
    }
    ui::text(target, font, "21h", 11, {chart.left + chart.width, chart.top + chart.height + 12}, theme.muted, true);
}
void DayReportState::drawSide(sf::RenderTarget &target, sf::FloatRect box) {
    const ui::Theme theme;
    const auto &font = context_.resources.uiFont();
    const auto &titleFont = context_.resources.titleFont();
    const auto &r = game().lastDay;
    const auto &d = game().data;
    ui::shadowPanel(target, box, theme.paper, 24, 6, 50);
    ui::text(target, titleFont, "Nhiệm vụ", 20, {box.left + 20, box.top + 14}, theme.green);
    for (std::size_t i = 0; i < r.goals.size() && i < 3; ++i) {
        const auto &goal = r.goals[i];
        const float top = box.top + 48 + static_cast<float>(i) * 50;
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
        ui::textFit(target, font, goalText(goal), 14, {box.left + 52, top + 6}, theme.ink, box.width - 150);
        const float ratio = goal.id == "nolose" ? (goal.done() ? 1.f : 0.f)
                            : static_cast<float>(std::min(goal.progress, goal.target)) / static_cast<float>(std::max(1, goal.target));
        ui::ProgressBar::draw(target, {box.left + 52, top + 30, box.width - 160, 6}, ratio * appear, goal.done() ? theme.green : theme.orange);
        const std::string reward = goal.done() ? "+" + moneyText(goal.reward) : goalProgressText(goal);
        ui::text(target, font, reward, 13, {box.left + box.width - 24 - ui::textWidth(font, reward, 13), top + 24}, goal.done() ? theme.green : theme.muted);
    }
    // Khách nổi bật trong ngày.
    const float y = box.top + 210;
    ui::text(target, titleFont, "Khách của ngày", 20, {box.left + 20, y}, theme.orange);
    const sf::FloatRect spot{box.left + 14, y + 34, box.width - 28, 112};
    ui::panel(target, spot, {252, 238, 206}, 14);
    if (r.served > 0 && r.bestScore >= 0 && r.bestKind < d.customers.size()) {
        const auto &kind = d.customers[r.bestKind];
        const auto &persona = d.personalities.at(std::min(r.bestPersonality, d.personalities.size() - 1));
        const auto expression = r.bestScore >= .9f ? Expression::Delighted : Expression::Happy;
        ui::image(target, context_.resources, "customers/" + kind.style + "_happy", {spot.left + 4, spot.top + 4 + std::sin(age_ * 3.f) * 2.f, 104, 104});
        ui::emote(target, font, expression, {spot.left + 82, spot.top + 24}, age_, .8f);
        ui::text(target, titleFont, kind.name, 19, {spot.left + 118, spot.top + 10}, theme.ink);
        const float tw = ui::textWidth(context_.resources.smallFont(), persona.name, 12) + 16;
        ui::panel(target, {spot.left + 118, spot.top + 38, tw, 20}, persona.color, 10);
        ui::text(target, context_.resources.smallFont(), persona.name, 12, {spot.left + 118 + tw / 2, spot.top + 47}, {255, 252, 240}, true);
        ui::text(target, font, "Điểm " + std::to_string(static_cast<int>(std::lround(r.bestScore * 100))) + "/100", 14,
                 {spot.left + 126 + tw, spot.top + 39}, theme.green);
        const std::string quote = persona.happy.empty() ? "Ngon lắm!" : "“" + persona.happy[static_cast<std::size_t>(r.day) % persona.happy.size()] + "”";
        ui::textFit(target, context_.resources.dialogFont(), quote, 15, {spot.left + 118, spot.top + 68}, theme.muted, spot.width - 130);
        ui::text(target, font, std::to_string(r.delighted) + " khách mê tít  •  " + std::to_string(r.unhappy + r.left) + " khách không vui", 12,
                 {spot.left + 118, spot.top + 92}, theme.muted);
    } else {
        ui::paragraph(target, font, "Hôm nay chưa phục vụ được ai. Mai thử nhập hàng vừa đủ và chọn khách sắp hết kiên nhẫn trước nhé!", 14,
                      {spot.left + 14, spot.top + 18}, spot.width - 28, 20, theme.muted);
    }
    // Thành tựu mới / kho còn lại.
    const float ay = box.top + 372;
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
    drawLedger(target, {420, 92, 440, 300});
    drawTimeline(target, {420, 404, 440, 234});
    drawSide(target, {878, 92, 372, 546});
    // Ví hiện tại.
    ui::shadowPanel(target, {216, 650, 300, 46}, {255, 250, 233, 235}, 16, 3, 40);
    ui::image(target, context_.resources, "ui/coin_icon", {228, 658, 30, 30});
    ui::text(target, font, "Ví: " + moneyText(game().progress.money), 20, {268, 660}, theme.green);
    renderButtons(target);
    effects_.render(target, font);
}
} // namespace tiem

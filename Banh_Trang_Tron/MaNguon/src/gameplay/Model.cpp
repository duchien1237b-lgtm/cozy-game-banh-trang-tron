#include "gameplay/Model.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <ctime>
#include <limits>
#include <ostream>
#include <stdexcept>
namespace tiem {
namespace {
nlohmann::json read(const std::filesystem::path &path) {
    std::ifstream stream(path);
    if (!stream)
        throw std::runtime_error("Khong mo duoc " + path.u8string());
    nlohmann::json j;
    stream >> j;
    return j;
}
sf::Color color(const nlohmann::json &j) {
    if (!j.is_array() || j.size() != 3)
        throw std::runtime_error("Mau can 3 gia tri RGB");
    for (const auto &v : j)
        if (!v.is_number_integer() || v.get<int>() < 0 || v.get<int>() > 255)
            throw std::runtime_error("RGB ngoai khoang 0..255");
    return {j[0].get<sf::Uint8>(), j[1].get<sf::Uint8>(), j[2].get<sf::Uint8>()};
}
bool validReply(const std::string &text) {
    if (text.empty() || text.size() > 240 * 4) return false;
    std::size_t count = 0;
    bool visible = false;
    for (std::size_t i = 0; i < text.size();) {
        const auto first = static_cast<unsigned char>(text[i]);
        const unsigned bytes = first < 0x80 ? 1 : first >= 0xc2 && first <= 0xdf ? 2
                             : first >= 0xe0 && first <= 0xef ? 3 : first >= 0xf0 && first <= 0xf4 ? 4 : 0;
        if (bytes == 0 || i + bytes > text.size()) return false;
        std::uint32_t codepoint = first & (bytes == 1 ? 0x7f : (1u << (7 - bytes)) - 1);
        for (unsigned n = 1; n < bytes; ++n) {
            const auto next = static_cast<unsigned char>(text[i + n]);
            if ((next & 0xc0) != 0x80) return false;
            codepoint = (codepoint << 6) | (next & 0x3f);
        }
        if ((bytes == 2 && codepoint < 0x80) || (bytes == 3 && codepoint < 0x800) ||
            (bytes == 4 && codepoint < 0x10000) || codepoint > 0x10ffff ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff)) return false;
        if (codepoint < 32 && codepoint != '\n' && codepoint != '\t' && codepoint != '\r') return false;
        if (codepoint == 127) return false;
        const bool whitespace = codepoint <= 32 || codepoint == 0xa0 || codepoint == 0x1680 ||
            (codepoint >= 0x2000 && codepoint <= 0x200b) || codepoint == 0x2028 || codepoint == 0x2029 ||
            codepoint == 0x202f || codepoint == 0x205f || codepoint == 0x3000 || codepoint == 0xfeff;
        visible = visible || !whitespace;
        if (++count > 240) return false;
        i += bytes;
    }
    return visible;
}
} // namespace
GameData::GameData(const std::filesystem::path &root) {
    balance = read(root / "data/balance.json");
    std::set<std::string> ids;
    const auto toppingData = read(root / "data/toppings.json");
    for (const auto &j : toppingData.at("toppings")) {
        Topping t{j.at("id"),           j.at("name"),       j.at("type"),   color(j.at("color")),
                  j.at("unlock_price"), j.at("unit_price"), j.at("initial")};
        if (t.id.empty() || t.name.empty() || !ids.insert(t.id).second || t.unlockPrice < 0 ||
            t.unitPrice < 0 || (t.type != "dry" && t.type != "fresh" && t.type != "sauce"))
            throw std::runtime_error("Topping khong hop le");
        toppings.push_back(t);
    }
    ids.clear();
    const auto customerData = read(root / "data/customers.json");
    for (const auto &j : customerData.at("customers")) {
        CustomerKind k{j.at("id"), j.at("name"), j.at("style"), color(j.at("color")),
                       j.at("patience_factor")};
        k.femaleVoice = j.value("female_voice", false);
        k.child = j.value("is_child", false);
        k.olderAdult = j.value("is_older_adult", false);
        k.special = j.value("is_special", false);
        k.spawnWeight = j.value("spawn_weight", 1.f);
        k.minTypes = j.value("order_min_types", integer("order_initial_types"));
        k.maxTypes = j.value("order_max_types", integer("order_max_types"));
        k.minQuantity = j.value("order_min_quantity", 1);
        k.maxQuantity = j.value("order_max_quantity", integer("order_max_quantity"));
        if (k.id.empty() || !ids.insert(k.id).second || k.patienceFactor <= 0 ||
            !std::isfinite(k.spawnWeight) || k.spawnWeight <= 0 ||
            k.minTypes < 3 || k.maxTypes < k.minTypes || k.maxTypes > integer("order_max_types") ||
            k.minQuantity < 1 || k.maxQuantity < k.minQuantity || k.maxQuantity > integer("order_max_quantity"))
            throw std::runtime_error("Khach khong hop le");
        customers.push_back(k);
    }
    ids.clear();
    const auto upgradeData = read(root / "data/upgrades.json");
    for (const auto &j : upgradeData.at("upgrades")) {
        Upgrade u{j.at("id"), j.at("name"), j.at("description"), j.at("effect"), {}};
        if (u.id.empty() || !ids.insert(u.id).second)
            throw std::runtime_error("Trung ID nang cap");
        for (const auto &l : j.at("levels")) {
            UpgradeLevel level{l.at("price"), l.at("value")};
            if (level.price < 0 || level.value < 0)
                throw std::runtime_error("Nang cap am");
            u.levels.push_back(level);
        }
        upgrades.push_back(u);
    }
    if (toppings.empty() || customers.empty())
        throw std::runtime_error("Du lieu rong");
    if (std::count_if(toppings.begin(), toppings.end(), [](const auto &t) { return t.initial; }) < 3)
        throw std::runtime_error("Can it nhat 3 topping ban dau");
    // Các số liệu gameplay phải hữu hạn; lỗi dữ liệu dừng ngay thay vì hỏng giữa ngày.
    for (auto it = balance.begin(); it != balance.end(); ++it)
        if (it.value().is_number() &&
            (!std::isfinite(it.value().get<float>()) || it.value().get<float>() < 0))
            throw std::runtime_error("Thong so can la so khong am: " + it.key());
    for (const auto &key : {"day_seconds", "spawn_min_seconds", "patience_min_seconds", "paper_stage_seconds", "sauce_seconds_per_spoon", "mix_hit_cooldown", "topping_charge_seconds"})
        if (number(key) <= 0)
            throw std::runtime_error("Thong so phai lon hon 0");
    if (integer("mix_hits") < 1 || integer("mix_hits") > 10 || integer("queue_slots") < 1 ||
        integer("queue_max") > 4 || integer("queue_slots") > integer("queue_max") ||
        integer("bowl_capacity") < 8 ||
        integer("order_initial_types") < 3 || integer("order_max_types") > 6 ||
        integer("order_max_types") < integer("order_initial_types") ||
        integer("order_max_quantity") < 1 || integer("order_max_quantity") > 5)
        throw std::runtime_error("Thong so vuot gioi han giao dien");
    if (number("difficulty_step_per_day") <= 0 || number("difficulty_max_bonus") > .65f)
        throw std::runtime_error("Do kho chi duoc tang toi da 65%");
    float total = 0;
    for (const auto &key : {"recipe", "paper", "mix", "speed"})
        total += balance.at("weights").at(key).get<float>();
    if (std::abs(total - 1.f) > .001f)
        throw std::runtime_error("Tong trong so phai bang 1");
}
const Topping &GameData::topping(const std::string &id) const {
    const auto it = std::find_if(toppings.begin(), toppings.end(), [&](const auto &t) { return t.id == id; });
    if (it == toppings.end())
        throw std::runtime_error("ID topping khong ton tai: " + id);
    return *it;
}
Progress Progress::fresh(const GameData &d) {
    Progress p;
    p.money = d.integer("initial_money");
    p.music = d.number("default_music");
    p.sfx = d.number("default_sfx");
    for (const auto &t : d.toppings)
        if (t.initial)
            p.unlocked.insert(t.id);
    return p;
}
bool Bowl::add(const std::string &id, int capacity) {
    if (static_cast<int>(portions.size()) >= capacity || mixed)
        return false;
    portions.push_back(id);
    return true;
}
bool Bowl::removeLast(std::string *removed) {
    if (portions.empty() || mixed)
        return false;
    if (removed)
        *removed = portions.back();
    portions.pop_back();
    return true;
}
int Bowl::count(const std::string &id) const {
    return static_cast<int>(std::count(portions.begin(), portions.end(), id));
}
bool operator==(const Bowl &bowl, const Order &order) {
    if (bowl.portions.empty())
        return false;
    int total = 0;
    for (const auto &item : order.recipe) {
        if (bowl.count(item.first) != item.second)
            return false;
        total += item.second;
    }
    return static_cast<int>(bowl.portions.size()) == total;
}
std::ostream &operator<<(std::ostream &out, const Receipt &r) {
    return out << "Công thức " << static_cast<int>(r.recipe * 100) << "% • Xé " << static_cast<int>(r.paper * 100)
               << "% • Trộn " << static_cast<int>(r.mix * 100) << "% • Tốc độ " << static_cast<int>(r.speed * 100) << "%";
}
int Receipt::stars() const {
    return std::clamp(static_cast<int>(std::lround(1.f + 4.f * std::clamp(score, 0.f, 1.f))), 1, 5);
}
const char *Receipt::review() const {
    const char *reviews[] = {"Chưa đúng vị, lần sau cố gắng nhé!", "Cần nêm nếm lại một chút.",
                            "Khá ổn, cảm ơn tiệm nhé!", "Rất ngon, mình sẽ ghé lại!", "Ngon tuyệt! Đúng ý mình luôn!"};
    return reviews[stars() - 1];
}
void ReviewSystem::record(Progress &progress, const Receipt &receipt, int day) {
    // These fictional online identities are separate from the in-store customer characters.
    static const char *authors[] = {"Thanh Thảo", "Tuấn Kiệt", "Bảo Ngọc", "Hoài Phương", "Khánh Vy",
        "Quốc Huy", "Ngọc Trâm", "Gia Bảo", "Thùy Dương", "Đức Anh", "Bích Ngân", "Hải Đăng",
        "Yến Nhi", "Phúc Khang", "Mai Chi", "Trọng Nhân", "Quỳnh Hương", "Thiên Phúc", "Hà My",
        "Nhật Nam", "Tú Uyên", "Bảo Châu", "Mộc Miên", "Đình Khoa", "Vân Khánh", "Xuân Quỳnh",
        "Thiên Ân", "Diệu Trang", "Ngọc Ánh", "Hữu Đạt", "Cẩm Tú", "Kim Oanh"};
    static const char *opening[5][4] = {
        {"Về nhà rồi mà mình vẫn hụt hẫng vì phần bánh tráng hôm nay.",
         "Lâu lắm mới thèm bánh tráng, tiếc là lần ghé này làm mình buồn thật sự.",
         "Mình đã rất mong chờ, nhưng ăn xong chỉ thấy tiếc một buổi đi ăn vặt.",
         "Ngồi xuống viết lại vì trải nghiệm hôm nay khiến mình thất vọng quá."},
        {"Ăn xong về nhà vẫn thấy hơi tiếc, mình đã kỳ vọng nhiều hơn ở tiệm.",
         "Mình muốn thích tiệm lắm, nhưng phần bánh hôm nay chưa làm mình vui.",
         "Tan buổi ghé quán mà cảm giác vẫn chưa trọn vẹn, tiếc ghê.",
         "Định tìm một quán ruột mới, nhưng lần đầu này mình chưa ưng lắm."},
        {"Về nhà nghĩ lại thì bữa ăn vặt hôm nay cũng ổn, dù chưa thật sự mê.",
         "Ghé thử vì thèm bánh tráng, ăn được nhưng mình vẫn mong một chút bất ngờ.",
         "Một buổi ăn vặt khá dễ chịu, mình viết vài lời để tiệm có thể làm tốt hơn.",
         "Mình vừa ghé tiệm về, cảm giác là ổn áp nhưng còn chỗ để ngon hơn nữa."},
        {"Về đến nhà vẫn nhớ phần bánh tráng vừa ăn, đúng là một buổi ghé quán vui vẻ!",
         "Tưởng chỉ ghé ăn cho đỡ thèm mà cuối cùng lại thấy thích tiệm mất rồi!",
         "Ăn xong thấy tâm trạng tốt hẳn, phải về viết vài lời khen tiệm mới được!",
         "Lần ghé này làm mình mỉm cười suốt đường về, món ăn vặt nhỏ mà vui ghê!"},
        {"Về tới nhà là phải lên viết ngay: phần bánh tráng hôm nay làm mình mê thật sự!",
         "Một phần bánh tráng mà cứu cả ngày mệt mỏi của mình, hạnh phúc quá đi!",
         "Ăn hết rồi vẫn còn thèm, lâu lắm mới gặp một tiệm khiến mình vui đến vậy!",
         "Không nghĩ một buổi ghé ăn vặt lại đáng nhớ như vậy, mình chính thức mê tiệm rồi!"}
    };
    static const char *closing[5][3] = {
        {"Mong tiệm đọc và sửa, chứ lần này mình chưa thể giới thiệu cho bạn bè.",
         "Mình góp ý thật lòng vì rất mong lần sau sẽ không phải buồn như vậy nữa.",
         "Hy vọng tiệm để tâm đến trải nghiệm của khách hơn, mình vẫn muốn có lý do quay lại."},
        {"Nếu tiệm cải thiện được thì mình sẵn sàng thử lại một lần nữa.",
         "Mong lần sau ghé mình sẽ có một đánh giá vui hơn dành cho tiệm.",
         "Viết ra vì vẫn muốn tiệm làm tốt hơn, hy vọng lời góp ý được lắng nghe."},
        {"Tiệm chăm chút thêm một chút là mình sẽ muốn quay lại thường xuyên hơn.",
         "Mình sẽ ghé lại thử thêm, mong lần tới có thể chấm cao hơn nhé.",
         "Hy vọng lần sau sẽ là một phần bánh tráng khiến mình nhớ mãi."},
        {"Đã lưu tiệm vào danh sách ăn vặt, lần sau rủ bạn ghé cùng!",
         "Cảm ơn tiệm vì một bữa ăn vặt thật dễ thương, mình sẽ quay lại nhé!",
         "Mình sẽ giới thiệu cho hội bạn mê ăn vặt, chúc tiệm ngày càng đông khách!"},
        {"Cho tiệm trọn năm sao, nhất định sẽ rủ cả hội bạn quay lại!",
         "Đã tìm được quán ruột mới rồi, cảm ơn tiệm vì phần bánh tráng quá đáng yêu!",
         "Tiệm cứ giữ phong độ này nhé, mình sẵn sàng ghé thêm nhiều lần nữa!"}
    };
    constexpr auto maxId = std::numeric_limits<std::uint64_t>::max();
    if (progress.nextReviewId == 0 || progress.nextReviewId == maxId) {
        std::uint64_t id = 1;
        for (auto &entry : progress.reviews) entry.id = id++;
        progress.nextReviewId = id;
    }
    const std::uint64_t id = progress.nextReviewId++;
    const int stars = receipt.stars();
    const auto variant = static_cast<std::size_t>((id - 1) % 4);
    const auto authorIndex = static_cast<std::size_t>(((id - 1) % 32) * 13 % 32);
    std::string author = authors[authorIndex];
    // Once the pool wraps, account suffixes keep each visit's online identity distinct.
    if (id > 32) author += " · " + std::to_string((id - 1) / 32 + 1);
    std::string detail;
    if (receipt.recipe < .65f)
        detail = "Phần topping khác điều mình mong đợi, nên ăn mà cứ thấy thiếu cảm giác trọn vẹn.";
    else if (receipt.mix < .55f)
        detail = "Bánh tráng còn chỗ đậm chỗ nhạt, giá mà được trộn đều tay hơn thì thích biết mấy.";
    else if (receipt.speed < .3f)
        detail = "Mình phải chờ hơi lâu nên cảm giác háo hức cũng vơi đi một chút.";
    else if (receipt.paper < .65f)
        detail = "Miếng bánh chưa thật vừa miệng, tiệm chăm chút bước xé thêm chút nữa nha.";
    else if (receipt.recipe < .95f)
        detail = "Topping gần đúng ý mình rồi, chỉ cần để ý yêu cầu kỹ hơn một chút là tuyệt.";
    else if (receipt.mix < .85f)
        detail = "Topping đúng ý nhưng vị chưa thật đều, mình mong lần tới sẽ hòa quyện hơn.";
    else {
        static const char *details[] = {"Topping đúng ý, bánh tráng trộn đều nên miếng nào cũng làm mình thích.",
            "Phần bánh được làm đúng những gì mình chọn, ăn đến miếng cuối vẫn thấy ngon.",
            "Topping và bánh tráng hòa quyện vừa miệng, đúng cảm giác mình tìm kiếm.",
            "Mình thích nhất là phần topping đúng ý và bánh tráng được trộn rất đều tay."};
        detail = details[variant];
    }
    std::string text = std::string(opening[stars - 1][variant]) + " " + detail + " " +
        closing[stars - 1][static_cast<std::size_t>((id / 4) % 3)];
    progress.reviews.push_back({id, std::move(author), std::move(text), "", std::clamp(day, 1, 10000), stars});
    if (progress.reviews.size() > 100)
        progress.reviews.erase(progress.reviews.begin(), progress.reviews.end() - 100);
}
bool ReviewSystem::reply(Progress &progress, std::uint64_t id, const std::string &text) {
    if (!validReply(text)) return false;
    const auto found = std::find_if(progress.reviews.begin(), progress.reviews.end(),
        [id](const auto &review) { return review.id == id; });
    if (found == progress.reviews.end()) return false;
    found->reply = text;
    return true;
}
float DayResult::rating() const {
    return served + left > 0 ? 1.f + 4.f * scoreSum / static_cast<float>(served + left) : 1.f;
}
int UpgradeSystem::level(const Progress &p, const std::string &id) {
    const auto it = p.levels.find(id);
    return it == p.levels.end() ? 0 : it->second;
}
float difficultyMultiplier(const GameData &data, int day) {
    return 1.f + std::min(data.number("difficulty_max_bonus"),
        static_cast<float>(std::max(0, day - 1)) * data.number("difficulty_step_per_day"));
}
float UpgradeSystem::effect(const GameData &d, const Progress &p, const std::string &effect) {
    float total = 0;
    for (const auto &u : d.upgrades) {
        const auto n = level(p, u.id);
        if (u.effect == effect && n > 0 && n <= static_cast<int>(u.levels.size()))
            total += u.levels[static_cast<std::size_t>(n - 1)].value;
    }
    return total;
}
bool UpgradeSystem::buy(const GameData &d, Progress &p, std::size_t index) {
    if (index >= d.upgrades.size())
        return false;
    const auto &u = d.upgrades[index];
    const auto n = level(p, u.id);
    if (n >= static_cast<int>(u.levels.size()) || p.money < u.levels[static_cast<std::size_t>(n)].price)
        return false;
    p.money -= u.levels[static_cast<std::size_t>(n)].price;
    p.levels[u.id] = n + 1;
    return true;
}
bool UpgradeSystem::unlock(const GameData &d, Progress &p, std::size_t index) {
    if (index >= d.toppings.size())
        return false;
    const auto &t = d.toppings[index];
    if (p.unlocked.count(t.id) || p.money < t.unlockPrice)
        return false;
    p.money -= t.unlockPrice;
    p.unlocked.insert(t.id);
    return true;
}
Receipt EconomySystem::score(const GameData &d, const Progress &p, const Customer &c, const Bowl &b) {
    // So sánh cả hợp hai tập: sai loại, thiếu và thừa đều bị trừ điểm.
    int required = 0, difference = 0;
    std::map<std::string, int> actual;
    for (const auto &id : b.portions)
        ++actual[id];
    for (const auto &pair : c.order.recipe) {
        required += pair.second;
        difference += std::abs(actual[pair.first] - pair.second);
        actual.erase(pair.first);
    }
    for (const auto &pair : actual)
        difference += pair.second;
    Receipt r;
    r.recipe =
        std::clamp(1.f - static_cast<float>(difference) /
                             static_cast<float>(std::max({1, required, static_cast<int>(b.portions.size())})),
                   0.f, 1.f);
    r.paper = std::clamp(b.paperQuality, 0.f, 1.f);
    r.mix = b.mixed ? std::clamp(b.mixQuality, 0.f, 1.f) : 0.f;
    r.speed = std::clamp(c.remaining / c.patience, 0.f, 1.f);
    // Trọng số và kinh tế lấy từ JSON, UI không tự tính một công thức khác.
    const auto &w = d.balance.at("weights");
    r.score = r.recipe * w.at("recipe").get<float>() + r.paper * w.at("paper").get<float>() +
              r.mix * w.at("mix").get<float>() + r.speed * w.at("speed").get<float>();
    const float floor = d.number("payment_floor");
    r.base = static_cast<int>(
                 std::round(static_cast<float>(c.order.price) * (floor + (1 - floor) * r.score) / 100.f)) *
             100;
    const float tips = d.number("tip_rate") + UpgradeSystem::effect(d, p, "decor") +
                       UpgradeSystem::effect(d, p, "lantern") + UpgradeSystem::effect(d, p, "curtain");
    r.tip =
        static_cast<int>(std::round(static_cast<float>(c.order.price) * tips * r.score * r.score / 100.f)) *
        100;
    if (d.customers.at(c.kind).special) r.tip = c.order.price;
    return r;
}
const std::vector<AchievementInfo> &AchievementSystem::all() {
    static const std::vector<AchievementInfo> list = {
        {"first_order", "Đơn đầu tiên", "Phục vụ vị khách đầu tiên", 1},
        {"regular_50", "Quán quen", "Phục vụ 50 lượt khách", 50},
        {"busy_200", "Tiệm đông khách", "Phục vụ 200 lượt khách", 200},
        {"five_star_10", "Năm sao lấp lánh", "Nhận 10 đơn đạt 5 sao", 10},
        {"combo_5", "Tay nghề lên hương", "Đạt combo x5", 5},
        {"combo_10", "Bàn tay vàng", "Đạt combo x10", 10},
        {"perfect_20", "Nhịp trộn hoàn hảo", "Trộn trúng Perfect 20 lần", 20},
        {"rush_8", "Chiến thần giờ cao điểm", "Phục vụ 8 khách trong giờ cao điểm", 8},
        {"rich", "Ông chủ nhỏ", "Kiếm tổng cộng 500.000 đ", 500000},
        {"perfect_day", "Ngày trọn vẹn", "Một ngày không ai bỏ về, đạt từ 4,5 sao", 1},
        {"goals_15", "Có chí thì nên", "Hoàn thành 15 nhiệm vụ trong ngày", 15},
        {"all_toppings", "Đủ vị đủ màu", "Mở khóa mọi loại topping", 1},
        {"max_upgrades", "Tiệm trong mơ", "Nâng cấp tiệm lên mức tối đa", 1},
        {"streak_7", "Khách quen mỗi ngày", "Nhận quà hằng ngày 7 ngày liên tiếp", 7},
    };
    return list;
}
int AchievementSystem::current(const GameData &d, const Progress &p, const std::string &id) {
    if (id == "first_order" || id == "regular_50" || id == "busy_200") return p.served;
    if (id == "five_star_10") return p.fiveStars;
    if (id == "combo_5" || id == "combo_10") return p.bestCombo;
    if (id == "perfect_20") return p.perfectMixes;
    if (id == "rush_8") return p.rushServed;
    if (id == "rich") return p.lifetime;
    if (id == "perfect_day") return p.perfectDays;
    if (id == "goals_15") return p.goalsDone;
    if (id == "streak_7") return p.dailyStreak;
    if (id == "all_toppings") {
        int missing = 0;
        for (const auto &t : d.toppings) missing += p.unlocked.count(t.id) ? 0 : 1;
        return missing == 0 ? 1 : 0;
    }
    if (id == "max_upgrades") {
        for (const auto &u : d.upgrades)
            if (UpgradeSystem::level(p, u.id) < static_cast<int>(u.levels.size())) return 0;
        return 1;
    }
    return 0;
}
std::vector<std::string> AchievementSystem::check(const GameData &d, Progress &p) {
    std::vector<std::string> opened;
    for (const auto &a : all())
        if (!p.achievements.count(a.id) && current(d, p, a.id) >= a.target) {
            p.achievements.insert(a.id);
            opened.push_back(a.id);
        }
    return opened;
}
std::int64_t DailyReward::today() {
    const std::time_t now = std::time(nullptr);
    const std::tm *local = std::localtime(&now);
    // Thuật toán days-from-civil: không phụ thuộc hệ điều hành hay múi giờ.
    std::int64_t y = local->tm_year + 1900;
    const int m = local->tm_mon + 1, d = local->tm_mday;
    y -= m <= 2;
    const std::int64_t era = (y >= 0 ? y : y - 399) / 400;
    const auto yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * static_cast<unsigned>(m + (m > 2 ? -3 : 9)) + 2) / 5 + static_cast<unsigned>(d) - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<std::int64_t>(doe) - 719468;
}
bool DailyReward::canClaim(const Progress &p, std::int64_t today) {
    return p.lastDaily != today;
}
int DailyReward::nextIndex(const Progress &p, std::int64_t today) {
    const int streak = (p.lastDaily >= 0 && p.lastDaily == today - 1) ? p.dailyStreak : 0;
    return streak % 7;
}
int DailyReward::amount(const GameData &d, int index) {
    static const int fallback[] = {10000, 15000, 20000, 25000, 35000, 45000, 80000};
    const auto it = d.balance.find("daily_rewards");
    if (it != d.balance.end() && it->is_array() && it->size() == 7 && (*it)[static_cast<std::size_t>(index % 7)].is_number())
        return std::max(0, (*it)[static_cast<std::size_t>(index % 7)].get<int>());
    return fallback[index % 7];
}
int DailyReward::claim(const GameData &d, Progress &p, std::int64_t today) {
    if (!canClaim(p, today)) return 0;
    const int index = nextIndex(p, today);
    const int reward = amount(d, index);
    p.dailyStreak = index + 1;
    p.lastDaily = today;
    p.money += reward;
    AchievementSystem::check(d, p);
    return reward;
}
std::vector<DayGoal> DaySession::makeGoals(const GameData &d, int day) {
    const int n = std::max(1, day);
    const auto scale = [&](int base, int step, int cap) { return std::min(cap, base + n / step); };
    std::vector<DayGoal> pool = {
        {"serve", "Phục vụ khách", scale(4, 2, 12), 0, 6000 + std::min(n, 20) * 500},
        {"stars", "Đơn đạt từ 4 sao", scale(2, 3, 8), 0, 8000 + std::min(n, 20) * 400},
        {"tips", "Tiền tip kiếm được", std::min(60000, 6000 + n * 2000), 0, 8000 + std::min(n, 20) * 400},
        {"combo", "Đạt combo", scale(3, 4, 7), 0, 10000 + std::min(n, 20) * 500},
        {"perfect", "Trộn trúng Perfect", scale(3, 3, 12), 0, 7000 + std::min(n, 20) * 400},
        {"fast", "Giao món khi khách còn kiên nhẫn", scale(3, 3, 8), 0, 9000 + std::min(n, 20) * 400},
        {"nolose", "Không để khách nào bỏ về", 1, 0, 12000 + std::min(n, 20) * 600},
    };
    (void)d;
    std::mt19937 rng(static_cast<unsigned>(n) * 2654435761u + 12345u);
    std::shuffle(pool.begin(), pool.end(), rng);
    pool.resize(3);
    // Mục tiêu dễ nhất hiện đầu danh sách để người chơi có động lực.
    std::sort(pool.begin(), pool.end());
    return pool;
}
DaySession::DaySession(const GameData &d, Progress &p, unsigned seed)
    : data_(d), progress_(p), random_(seed), reviewRandom_(seed ^ 0xA511E9B3u),
      remaining_(d.number("day_seconds")) {
    result_.day = p.day;
    result_.goals = makeGoals(d, p.day);
    // Giờ cao điểm dùng bộ sinh riêng để không làm lệch chuỗi khách theo seed.
    std::mt19937 rushRandom(seed * 2654435761u + 17u);
    const float day = d.number("day_seconds");
    rushBegin_ = day * std::uniform_real_distribution<float>(.42f, .62f)(rushRandom);
    rushEnd_ = rushBegin_ - d.numberOr("rush_seconds", 25.f);
    spawn();
    spawnTimer_ = std::max(d.number("spawn_min_seconds"), d.number("spawn_seconds") / difficultyMultiplier(d, p.day));
}
int DaySession::capacity() const {
    int levels = 0;
    for (const auto &u : data_.upgrades)
        if (u.effect == "bowl")
            levels += UpgradeSystem::level(progress_, u.id);
    return data_.integer("bowl_capacity") + levels * data_.integer("bowl_capacity_per_level");
}
void DaySession::spawn() {
    const int slots = std::min(data_.integer("queue_max"),
                               data_.integer("queue_slots") +
                                   static_cast<int>(UpgradeSystem::effect(data_, progress_, "queue")));
    if (static_cast<int>(customers_.size()) >= slots)
        return;
    Customer c;
    c.id = nextId_++;
    std::vector<float> weights;
    for (const auto &kind : data_.customers) weights.push_back(kind.spawnWeight);
    c.kind = std::discrete_distribution<std::size_t>(weights.begin(), weights.end())(random_);
    c.patience = (std::max(data_.number("patience_min_seconds"),
                           data_.number("patience_seconds") / difficultyMultiplier(data_, result_.day)) +
                  UpgradeSystem::effect(data_, progress_, "patience")) *
                 data_.customers[c.kind].patienceFactor;
    c.remaining = c.patience;
    std::vector<std::string> available;
    for (const auto &t : data_.toppings)
        if (progress_.unlocked.count(t.id))
            available.push_back(t.id);
    std::shuffle(available.begin(), available.end(), random_);
    const auto &kind = data_.customers[c.kind];
    // Start with one spoon per preferred type. Extra workload has a strict daily budget.
    const int baseTypes = std::min(kind.minTypes, static_cast<int>(available.size()));
    const int typeLimit = std::min(kind.maxTypes, static_cast<int>(available.size()));
    // Thêm vài muỗng ngẫu nhiên để đơn có lúc 2-3 muỗng một loại ngay từ đầu; ngày càng cao càng có thể nhiều hơn.
    const int extraMax = std::min(2, 1 + (result_.day - 1) / 3);
    const int extra = std::uniform_int_distribution<int>(0, extraMax)(random_);
    const int budget = std::min(capacity(), static_cast<int>(std::floor(
        baseTypes * difficultyMultiplier(data_, result_.day) + .0001f)) + extra);
    int types = baseTypes;
    for (int i = 0; i < types; ++i) c.order.recipe[available[static_cast<std::size_t>(i)]] = 1;
    for (int amount = baseTypes; amount < budget; ++amount) {
        std::vector<std::string> choices;
        for (const auto &item : c.order.recipe)
            if (item.second < kind.maxQuantity) choices.push_back(item.first);
        // A new type and another spoon both cost one unit of the same workload budget.
        if (types < typeLimit && (choices.empty() || std::bernoulli_distribution(.5)(random_))) {
            c.order.recipe[available[static_cast<std::size_t>(types++)]] = 1;
        } else if (!choices.empty()) {
            const auto index = std::uniform_int_distribution<std::size_t>(0, choices.size() - 1)(random_);
            ++c.order.recipe[choices[index]];
        } else break;
    }
    int price = data_.integer("base_price");
    for (const auto &item : c.order.recipe) price += data_.topping(item.first).unitPrice * item.second;
    c.order.price =
        static_cast<int>(std::round(static_cast<float>(price) *
                                    (1.f + UpgradeSystem::effect(data_, progress_, "bowl")) / 100.f)) *
        100;
    customers_.push_back(c);
}
void DaySession::update(float dt) {
    if (ended_)
        return;
    dt = std::max(0.f, dt);
    remaining_ -= dt;
    for (auto &c : customers_) {
        c.remaining -= dt;
        c.arrival = std::min(1.f, c.arrival + dt * 4);
    }
    const auto old = customers_.size();
    customers_.erase(
        std::remove_if(customers_.begin(), customers_.end(), [](const auto &c) { return c.remaining <= 0; }),
        customers_.end());
    result_.left += static_cast<int>(old - customers_.size());
    if (old != customers_.size()) combo_ = 0;
    if (remaining_ <= 0) {
        finish();
        return;
    }
    const bool inRush = remaining_ <= rushBegin_ && remaining_ > rushEnd_;
    if (rushActive_ && !inRush) rushFinished_ = true;
    rushActive_ = inRush;
    // Giờ cao điểm: khách đến dày hơn (giới hạn vẫn là số chỗ chờ).
    const float speedUp = rushActive_ ? data_.numberOr("rush_spawn_factor", 1.8f) : 1.f;
    spawnTimer_ -= dt * speedUp;
    if (spawnTimer_ <= 0) {
        spawn();
        spawnTimer_ = std::max(data_.number("spawn_min_seconds"),
                               data_.number("spawn_seconds") / difficultyMultiplier(data_, result_.day));
    }
}
Receipt DaySession::serve(std::size_t index, const Bowl &bowl) {
    if (ended_ || index >= customers_.size() || !bowl.mixed)
        return {};
    auto receipt = EconomySystem::score(data_, progress_, customers_[index], bowl);
    const bool special = data_.customers.at(customers_[index].kind).special;
    // Combo: các đơn từ 4 sao liên tiếp; bỏ khách hoặc đơn dưới 4 sao sẽ ngắt chuỗi.
    combo_ = receipt.stars() >= 4 ? combo_ + 1 : 0;
    receipt.combo = combo_;
    receipt.rush = rushActive_;
    if (!special) {
        const float step = data_.numberOr("combo_step", .04f);
        const int maxSteps = static_cast<int>(data_.numberOr("combo_max", 8.f));
        if (combo_ >= 2)
            receipt.comboBonus = static_cast<int>(std::round(static_cast<float>(customers_[index].order.price) * step *
                                     static_cast<float>(std::min(combo_ - 1, maxSteps)) / 100.f)) * 100;
        if (rushActive_)
            receipt.rushBonus = static_cast<int>(std::round(static_cast<float>(receipt.base) *
                                     data_.numberOr("rush_bonus", .2f) / 100.f)) * 100;
        receipt.tip += receipt.comboBonus + receipt.rushBonus;
    }
    result_.revenue += receipt.base;
    result_.tips += receipt.tip;
    result_.comboBonus += receipt.comboBonus;
    result_.rushBonus += receipt.rushBonus;
    result_.bestCombo = std::max(result_.bestCombo, combo_);
    ++result_.served;
    result_.scoreSum += receipt.score;
    if (receipt.stars() == 5) { ++result_.fiveStars; ++progress_.fiveStars; }
    if (rushActive_) { ++result_.rushServed; ++progress_.rushServed; }
    if (receipt.speed >= .5f) ++result_.fastServed;
    result_.perfectMixes += bowl.perfectHits;
    progress_.perfectMixes += bowl.perfectHits;
    progress_.bestCombo = std::max(progress_.bestCombo, combo_);
    progress_.money += receipt.base + receipt.tip;
    progress_.lifetime += receipt.base + receipt.tip;
    ++progress_.served;
    for (auto &goal : result_.goals) {
        if (goal.id == "serve") ++goal.progress;
        else if (goal.id == "stars" && receipt.stars() >= 4) ++goal.progress;
        else if (goal.id == "tips") goal.progress += receipt.tip;
        else if (goal.id == "combo") goal.progress = std::max(goal.progress, combo_);
        else if (goal.id == "perfect") goal.progress += bowl.perfectHits;
        else if (goal.id == "fast" && receipt.speed >= .5f) ++goal.progress;
    }
    // Only some guests post online; keep this independent of order/spawn randomness.
    if (std::bernoulli_distribution(.4)(reviewRandom_))
        ReviewSystem::record(progress_, receipt, result_.day);
    customers_.erase(customers_.begin() + static_cast<std::ptrdiff_t>(index));
    for (auto &id : AchievementSystem::check(data_, progress_)) {
        result_.newAchievements.push_back(id);
        unlockedQueue_.push_back(std::move(id));
    }
    return receipt;
}
void DaySession::finish() {
    // Chốt ngày đúng một lần, kể cả nhiều sự kiện cùng xảy ra ở frame cuối.
    if (ended_)
        return;
    ended_ = true;
    remaining_ = 0;
    result_.left += static_cast<int>(customers_.size());
    customers_.clear();
    combo_ = 0;
    rushActive_ = false;
    progress_.bestRating = std::max(progress_.bestRating, result_.rating());
    result_.perfectDay = result_.left == 0 && result_.served >= 3 && result_.rating() >= 4.5f;
    if (result_.perfectDay) ++progress_.perfectDays;
    // Nhiệm vụ "không ai bỏ về" chỉ chốt được khi hết ngày.
    for (auto &goal : result_.goals)
        if (goal.id == "nolose" && result_.left == 0 && result_.served >= 3) goal.progress = 1;
    for (const auto &goal : result_.goals)
        if (goal.done()) {
            result_.goalBonus += goal.reward;
            ++result_.goalsDone;
        }
    progress_.money += result_.goalBonus;
    progress_.lifetime += result_.goalBonus;
    progress_.goalsDone += result_.goalsDone;
    for (auto &id : AchievementSystem::check(data_, progress_)) {
        result_.newAchievements.push_back(id);
        unlockedQueue_.push_back(std::move(id));
    }
    ++progress_.day;
}
std::string moneyText(int amount) {
    std::string text = std::to_string(amount);
    for (int i = static_cast<int>(text.size()) - 3; i > 0; i -= 3)
        text.insert(static_cast<std::size_t>(i), ".");
    return text + " đ";
}
std::string goalText(const DayGoal &g) {
    const auto n = std::to_string(g.target);
    if (g.id == "serve") return "Phục vụ " + n + " khách";
    if (g.id == "stars") return "Giao " + n + " đơn từ 4 sao trở lên";
    if (g.id == "tips") return "Kiếm " + moneyText(g.target) + " tiền tip";
    if (g.id == "combo") return "Đạt combo x" + n;
    if (g.id == "perfect") return "Trộn trúng Perfect " + n + " lần";
    if (g.id == "fast") return "Giao nhanh " + n + " đơn (khách còn hơn nửa kiên nhẫn)";
    if (g.id == "nolose") return "Không để khách nào bỏ về (ít nhất 3 khách)";
    return g.text;
}
std::string goalShort(const DayGoal &g) {
    if (g.id == "serve") return "Phục vụ";
    if (g.id == "stars") return "Đơn 4 sao+";
    if (g.id == "tips") return "Tiền tip";
    if (g.id == "combo") return "Combo";
    if (g.id == "perfect") return "Perfect";
    if (g.id == "fast") return "Giao nhanh";
    if (g.id == "nolose") return "Không bỏ về";
    return g.text;
}
std::string goalProgressText(const DayGoal &g) {
    if (g.id == "tips")
        return std::to_string(std::min(g.progress, g.target) / 1000) + "k/" + std::to_string(g.target / 1000) + "k";
    if (g.id == "nolose") return g.done() ? "Đạt" : "";
    return std::to_string(std::min(g.progress, g.target)) + "/" + std::to_string(g.target);
}
} // namespace tiem

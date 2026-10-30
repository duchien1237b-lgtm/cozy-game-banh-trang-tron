#include "gameplay/Model.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
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
int Bowl::count(const std::string &id) const {
    return static_cast<int>(std::count(portions.begin(), portions.end(), id));
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
DaySession::DaySession(const GameData &d, Progress &p, unsigned seed)
    : data_(d), progress_(p), random_(seed), remaining_(d.number("day_seconds")) {
    result_.day = p.day;
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
    const int budget = std::min(capacity(), static_cast<int>(std::floor(
        baseTypes * difficultyMultiplier(data_, result_.day) + .0001f)));
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
    if (remaining_ <= 0) {
        finish();
        return;
    }
    spawnTimer_ -= dt;
    if (spawnTimer_ <= 0) {
        spawn();
        spawnTimer_ = std::max(data_.number("spawn_min_seconds"),
                               data_.number("spawn_seconds") / difficultyMultiplier(data_, result_.day));
    }
}
Receipt DaySession::serve(std::size_t index, const Bowl &bowl) {
    if (ended_ || index >= customers_.size() || !bowl.mixed)
        return {};
    const auto receipt = EconomySystem::score(data_, progress_, customers_[index], bowl);
    result_.revenue += receipt.base;
    result_.tips += receipt.tip;
    ++result_.served;
    result_.scoreSum += receipt.score;
    progress_.money += receipt.base + receipt.tip;
    progress_.lifetime += receipt.base + receipt.tip;
    ++progress_.served;
    ReviewSystem::record(progress_, receipt, result_.day);
    customers_.erase(customers_.begin() + static_cast<std::ptrdiff_t>(index));
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
    progress_.bestRating = std::max(progress_.bestRating, result_.rating());
    ++progress_.day;
}
std::string moneyText(int amount) {
    std::string text = std::to_string(amount);
    for (int i = static_cast<int>(text.size()) - 3; i > 0; i -= 3)
        text.insert(static_cast<std::size_t>(i), ".");
    return text + " đ";
}
} // namespace tiem

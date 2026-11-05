#pragma once
#include "gameplay/Model.hpp"
namespace tiem {
// Kho nguyên liệu của tiệm. Mỗi ngày người chơi nhập hàng trước khi mở cửa; khi thêm topping vào tô
// sẽ trừ kho. Cuối ngày đồ tươi hỏng, đồ khô và nước sốt được để dành sang hôm sau.
class InventorySystem {
  public:
    struct Spoilage {
        int portions{}, value{};
    };
    static int unitCost(const GameData &, const Topping &);  // giá nhập một muỗng
    static int rushCost(const GameData &, const Topping &);  // giá nhập gấp giữa ngày
    static int maxStock(const GameData &);
    static int stock(const Progress &, const std::string &id);
    // Mua số muỗng amount (bị giới hạn bởi sức chứa kho). Trả số thực mua, 0 nếu thiếu tiền/không mở khóa.
    static int buy(const GameData &, Progress &, const std::string &id, int amount, bool rush = false);
    static int cost(const GameData &, const std::map<std::string, int> &cart);
    // Mua cả giỏ một lần; không đủ tiền thì không mua gì.
    static bool buyCart(const GameData &, Progress &, const std::map<std::string, int> &cart);
    // Lấy tối đa amount muỗng khỏi kho, trả số lấy được.
    static int take(Progress &, const std::string &id, int amount);
    static void restore(const GameData &, Progress &, const std::string &id, int amount);
    static bool spoils(const Topping &t) { return t.type == "fresh"; }
    // Hết ngày: bỏ đồ tươi còn lại, trả số muỗng và giá trị đã mất.
    static Spoilage spoil(const GameData &, Progress &);
    static int expectedCustomers(const GameData &, const Progress &, const WeatherPlan &);
    // Số muỗng nên có cho mỗi topping đã mở theo thời tiết và lượng khách dự kiến.
    static std::map<std::string, int> demand(const GameData &, const Progress &, const WeatherPlan &);
    // Giỏ gợi ý = nhu cầu trừ tồn kho (không âm), cắt bớt nếu không đủ tiền.
    static std::map<std::string, int> suggest(const GameData &, const Progress &, const WeatherPlan &);
};
} // namespace tiem

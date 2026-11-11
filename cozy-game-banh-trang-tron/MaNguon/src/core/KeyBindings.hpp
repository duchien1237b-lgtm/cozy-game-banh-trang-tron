#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <map>
#include <string>
#include <vector>

namespace tiem {
struct KeyBindings {
    struct Action { const char *id; const char *label; std::vector<sf::Keyboard::Key> keys; };
    std::map<std::string, int> custom;
    static const std::vector<Action> &actions() {
        using K = sf::Keyboard;
        static const std::vector<Action> list = {
            {"primary", "Trộn / Đánh nhịp / Giao món", {K::Space}},
            {"previous_customer", "Chọn khách trước", {K::Left, K::A}},
            {"next_customer", "Chọn khách tiếp theo", {K::Right, K::D, K::Tab}},
            {"undo", "Bỏ muỗng cuối", {K::BackSpace, K::Z}},
            {"previous_tray", "Trang khay trước", {K::Q, K::PageUp}},
            {"next_tray", "Trang khay tiếp theo", {K::E, K::PageDown}},
            {"pause", "Tạm dừng / Tiếp tục", {K::Escape}},
            {"fullscreen", "Bật / tắt toàn màn hình", {K::F11}},
            {"start", "Mở quán / Tiếp tục ngày", {K::Enter}},
            {"tray1", "Chọn khay 1", {K::Num1, K::Numpad1}},
            {"tray2", "Chọn khay 2", {K::Num2, K::Numpad2}},
            {"tray3", "Chọn khay 3", {K::Num3, K::Numpad3}},
            {"tray4", "Chọn khay 4", {K::Num4, K::Numpad4}},
            {"tray5", "Chọn khay 5", {K::Num5, K::Numpad5}},
            {"tray6", "Chọn khay 6", {K::Num6, K::Numpad6}}
        };
        return list;
    }
    static bool allowed(int key) {
        using K = sf::Keyboard;
        return key >= 0 && key < K::KeyCount && key != K::LControl && key != K::RControl &&
            key != K::LShift && key != K::RShift && key != K::LAlt && key != K::RAlt &&
            key != K::LSystem && key != K::RSystem;
    }
    bool matches(const std::string &id, sf::Keyboard::Key key) const {
        if (key == sf::Keyboard::Unknown) return false;
        const auto found = custom.find(id);
        if (found != custom.end()) return found->second == key;
        for (const auto &action : actions()) if (id == action.id)
            for (const auto candidate : action.keys) if (candidate == key) return true;
        return false;
    }
    static std::string name(sf::Keyboard::Key key) {
        using K = sf::Keyboard;
        if (key >= K::A && key <= K::Z) return std::string(1, static_cast<char>('A' + key - K::A));
        if (key >= K::Num0 && key <= K::Num9) return std::to_string(key - K::Num0);
        if (key >= K::Numpad0 && key <= K::Numpad9) return "Num " + std::to_string(key - K::Numpad0);
        if (key >= K::F1 && key <= K::F15) return "F" + std::to_string(key - K::F1 + 1);
        switch (key) {
        case K::Space: return "Space"; case K::Enter: return "Enter"; case K::Escape: return "Esc";
        case K::Tab: return "Tab"; case K::BackSpace: return "Backspace";
        case K::Left: return "Left"; case K::Right: return "Right"; case K::Up: return "Up"; case K::Down: return "Down";
        case K::PageUp: return "Page Up"; case K::PageDown: return "Page Down";
        case K::Home: return "Home"; case K::End: return "End"; case K::Insert: return "Insert"; case K::Delete: return "Delete";
        case K::LBracket: return "["; case K::RBracket: return "]"; case K::Semicolon: return ";";
        case K::Comma: return ","; case K::Period: return "."; case K::Quote: return "'"; case K::Slash: return "/";
        case K::Backslash: return "\\"; case K::Tilde: return "`"; case K::Equal: return "="; case K::Hyphen: return "-";
        case K::Add: return "Num +"; case K::Subtract: return "Num -"; case K::Multiply: return "Num *";
        case K::Divide: return "Num /"; case K::Pause: return "Pause"; case K::Menu: return "Menu";
        default: return "?";
        }
    }
    std::string label(const std::string &id, bool all = false) const {
        const auto found = custom.find(id);
        if (found != custom.end()) return name(static_cast<sf::Keyboard::Key>(found->second));
        for (const auto &action : actions()) if (id == action.id) {
            std::string text;
            for (const auto key : action.keys) { if (!text.empty()) text += " / "; text += name(key); if (!all) break; }
            return text;
        }
        return "?";
    }
    std::string assign(const std::string &id, sf::Keyboard::Key key) {
        if (!allowed(key) || key == sf::Keyboard::Escape) return "Chọn một phím đơn; Esc dùng để hủy.";
        if (id == "fullscreen" && (key < sf::Keyboard::F1 || key > sf::Keyboard::F15))
            return "Toàn màn hình dùng phím F1–F15 để không ảnh hưởng khi gõ phản hồi.";
        bool known = false;
        for (const auto &action : actions()) {
            if (id == action.id) known = true;
            else if (matches(action.id, key)) return "Phím đang dùng cho: " + std::string(action.label);
        }
        if (!known) return "Thao tác không hợp lệ.";
        custom[id] = key;
        return {};
    }
    bool valid() const {
        std::map<int, std::string> occupied;
        for (const auto &entry : custom) {
            bool known = false;
            for (const auto &a : actions()) if (entry.first == a.id) known = true;
            if (!known || !allowed(entry.second) || (entry.second == sf::Keyboard::Escape && entry.first != "pause")) return false;
            if (entry.first == "fullscreen" && (entry.second < sf::Keyboard::F1 || entry.second > sf::Keyboard::F15)) return false;
        }
        for (const auto &a : actions()) for (int key = 0; key < sf::Keyboard::KeyCount; ++key)
            if (matches(a.id, static_cast<sf::Keyboard::Key>(key))) {
                if (occupied.count(key)) return false;
                occupied[key] = a.id;
            }
        return true;
    }
};
} // namespace tiem

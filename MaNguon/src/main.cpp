#include "core/Game.hpp"
#include "core/Utf8.hpp"
#include <exception>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif

int main(int, char**) {
    try {
        tiem::Game game;
        game.run();
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Khong the khoi dong game: " << error.what() << '\n';
#ifdef _WIN32
        // Bản Windows không có console; lỗi tài nguyên vẫn phải đọc được.
        const auto message = tiem::utf8(std::string("Không thể mở game.\n\n") + error.what()).toWideString();
        MessageBoxW(nullptr, message.c_str(), L"Bánh Tráng Trộn Tiệm", MB_OK | MB_ICONERROR);
#endif
        return 1;
    }
}

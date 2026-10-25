#pragma once
#include <filesystem>
#include <string>

namespace tiem {
struct AppConfig {
    std::string title;
    unsigned int width{};
    unsigned int height{};
    unsigned int fpsLimit{};
    std::string font;
    static AppConfig load(const std::filesystem::path &file);
};
} // namespace tiem

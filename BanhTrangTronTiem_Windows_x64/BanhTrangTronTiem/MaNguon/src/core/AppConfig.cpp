#include "core/AppConfig.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace tiem {
AppConfig AppConfig::load(const std::filesystem::path &file) {
    try {
        std::ifstream input(file);
        if (!input)
            throw std::runtime_error("khong mo duoc file");
        nlohmann::json data;
        input >> data;
        AppConfig result;
        result.title = data.at("title").get<std::string>();
        result.font = data.at("font").get<std::string>();
        const auto &window = data.at("window");
        const auto width = window.at("width").get<int>();
        const auto height = window.at("height").get<int>();
        const auto fps = window.at("fps_limit").get<int>();
        if (width < 640 || width > 3840 || height < 360 || height > 2160 || fps < 15 || fps > 240 ||
            result.title.empty() || result.font.empty()) {
            throw std::runtime_error("gia tri cau hinh ngoai khoang cho phep");
        }
        result.width = static_cast<unsigned int>(width);
        result.height = static_cast<unsigned int>(height);
        result.fpsLimit = static_cast<unsigned int>(fps);
        return result;
    } catch (const std::exception &error) {
        throw std::runtime_error("Loi cau hinh " + file.u8string() + ": " + error.what());
    }
}
} // namespace tiem

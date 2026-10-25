#pragma once
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics.hpp>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace tiem {
class ResourceManager {
  public:
    ResourceManager(std::filesystem::path root, std::string defaultFont);
    const sf::Font &font(const std::string &path);
    const sf::Font &uiFont();
    const sf::Texture *texture(const std::string &path);
    const sf::SoundBuffer *sound(const std::string &path);
    void drawImageOrPlaceholder(sf::RenderTarget &target, const std::string &path, sf::FloatRect bounds,
                                sf::Color color, const std::string &label);

  private:
    std::vector<sf::Uint8> readBytes(const std::string &relative) const;
    struct FontAsset {
        // SFML giữ tham chiếu đến bộ nhớ font: bytes phải sống lâu hơn font.
        std::vector<sf::Uint8> bytes;
        sf::Font font;
    };
    std::filesystem::path root_;
    std::string defaultFont_;
    std::unordered_map<std::string, std::unique_ptr<FontAsset>> fonts_;
    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures_;
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> sounds_;
};
} // namespace tiem

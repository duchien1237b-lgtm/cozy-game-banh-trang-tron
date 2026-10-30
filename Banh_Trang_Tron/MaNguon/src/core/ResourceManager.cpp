#include "core/ResourceManager.hpp"
#include "ui/Draw.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace tiem {
ResourceManager::ResourceManager(std::filesystem::path root, std::string defaultFont)
    : root_(std::move(root)), defaultFont_(std::move(defaultFont)) {}

std::vector<sf::Uint8> ResourceManager::readBytes(const std::string &relative) const {
    // std::filesystem mở đường dẫn Unicode đúng trên cả Windows và Unix.
    std::ifstream input(root_ / std::filesystem::u8path(relative), std::ios::binary);
    if (!input)
        return {};
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

const sf::Font &ResourceManager::font(const std::string &path) {
    const auto found = fonts_.find(path);
    if (found != fonts_.end())
        return found->second->font;
    auto asset = std::make_unique<FontAsset>();
    asset->bytes = readBytes(path);
    if (asset->bytes.empty() || !asset->font.loadFromMemory(asset->bytes.data(), asset->bytes.size()))
        throw std::runtime_error("Thieu/hong font: " + path + ". Hay giai nen du thu muc assets.");
    const auto &result = asset->font;
    fonts_.emplace(path, std::move(asset));
    return result;
}
const sf::Font &ResourceManager::uiFont() {
    return font("assets/fonts/Nunito.ttf");
}
const sf::Font &ResourceManager::titleFont() {
    return font("assets/fonts/Baloo2.ttf");
}
const sf::Font &ResourceManager::dialogFont() {
    return font("assets/fonts/Mali-Regular.ttf");
}
const sf::Font &ResourceManager::smallFont() {
    return font(defaultFont_);
}

const sf::Texture *ResourceManager::texture(const std::string &path) {
    const auto found = textures_.find(path);
    if (found != textures_.end())
        return found->second.get();
    const auto bytes = readBytes(path);
    std::unique_ptr<sf::Texture> asset;
    if (!bytes.empty()) {
        asset = std::make_unique<sf::Texture>();
        if (!asset->loadFromMemory(bytes.data(), bytes.size()))
            asset.reset();
        else
            asset->setSmooth(true);
    }
    if (!asset)
        std::clog << "Anh placeholder: " << path << '\n';
    const auto *result = asset.get();
    // Cache cả lần nạp thất bại, tránh đọc ổ đĩa và ghi log mỗi frame.
    textures_.emplace(path, std::move(asset));
    return result;
}

const sf::SoundBuffer *ResourceManager::sound(const std::string &path) {
    const auto found = sounds_.find(path);
    if (found != sounds_.end())
        return found->second.get();
    const auto bytes = readBytes(path);
    std::unique_ptr<sf::SoundBuffer> asset;
    if (!bytes.empty()) {
        asset = std::make_unique<sf::SoundBuffer>();
        if (!asset->loadFromMemory(bytes.data(), bytes.size()))
            asset.reset();
    }
    const auto *result = asset.get();
    sounds_.emplace(path, std::move(asset));
    return result;
}

void ResourceManager::drawImageOrPlaceholder(sf::RenderTarget &target, const std::string &path,
                                             sf::FloatRect bounds, sf::Color color,
                                             const std::string &label) {
    if (const auto *image = texture(path)) {
        sf::Sprite sprite(*image);
        const auto size = image->getSize();
        sprite.setPosition(bounds.left, bounds.top);
        sprite.setScale(bounds.width / static_cast<float>(size.x),
                        bounds.height / static_cast<float>(size.y));
        target.draw(sprite);
    } else {
        ui::panel(target, bounds, color, 18.f);
        ui::text(target, uiFont(), label, 18,
                 {bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f}, {79, 56, 45}, true);
    }
}
} // namespace tiem

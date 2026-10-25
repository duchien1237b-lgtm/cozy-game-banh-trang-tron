#include "core/AudioManager.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>
namespace tiem {
AudioManager::AudioManager(ResourceManager &r, std::filesystem::path root)
    : resources_(r), root_(std::move(root)) {}
void AudioManager::music(const std::string &scene) {
    if (scene_ == scene)
        return;
    music_.stop();
    scene_ = scene;
    // SFML stream giữ vùng nhớ đầu vào trong suốt thời gian phát nhạc.
    std::ifstream stream(root_ / "assets/audio/music" / (scene + ".wav"), std::ios::binary);
    std::vector<char> bytes{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    if (bytes.empty())
        return;
    // Đóng stream cũ trước khi thay bộ nhớ bằng cách mở file mới từ vùng nhớ còn sống.
    if (music_.openFromMemory(bytes.data(), bytes.size())) {
        musicBytes_.swap(bytes);
        music_.setLoop(true);
        music_.setVolume(musicVolume_);
        music_.play();
    }
}
void AudioManager::play(const std::string &name) {
    if (volume_ <= 0)
        return;
    if (const auto *buffer = resources_.sound("assets/audio/sfx/" + name + ".wav")) {
        if (sounds_.size() >= 24)
            sounds_.erase(sounds_.begin());
        auto sound = std::make_unique<sf::Sound>(*buffer);
        sound->setVolume(volume_);
        sound->play();
        sounds_.push_back(std::move(sound));
    }
}
void AudioManager::update(float musicVolume, float soundVolume) {
    volume_ = soundVolume;
    musicVolume_ = musicVolume;
    music_.setVolume(musicVolume_);
    sounds_.erase(std::remove_if(sounds_.begin(), sounds_.end(),
                                 [](const auto &s) { return s->getStatus() == sf::Sound::Stopped; }),
                  sounds_.end());
    for (auto &s : sounds_)
        s->setVolume(volume_);
}
} // namespace tiem

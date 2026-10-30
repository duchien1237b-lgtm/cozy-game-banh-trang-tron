#include "core/AudioManager.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
namespace tiem {
AudioManager::AudioManager(ResourceManager &r, std::filesystem::path root)
    : resources_(r), root_(std::move(root)) {}
void AudioManager::music(const std::string &scene) {
    if (scene_ == scene)
        return;
    music_.stop();
    ambience_.stop();
    charging(false);
    if (scene == "playing") {
        if (const auto *buffer = resources_.sound("assets/audio/sfx/sound-effect/people-background-noise.mp3")) {
            ambience_.setBuffer(*buffer);
            ambience_.setLoop(true);
            ambience_.setVolume(volume_ * .35f);
            ambience_.play();
        }
    }
    scene_ = scene;
    // SFML stream giữ vùng nhớ đầu vào trong suốt thời gian phát nhạc.
    for (const auto *extension : {".mp3", ".wav"}) {
        std::ifstream stream(root_ / "assets/audio/music" / (scene + extension), std::ios::binary);
        std::vector<char> bytes{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
        if (bytes.empty()) continue;
        // SFML's new stream reads from bytes; swap keeps those bytes alive after this function returns.
        if (music_.openFromMemory(bytes.data(), bytes.size())) {
            musicBytes_.swap(bytes);
            music_.setLoop(true);
            music_.setVolume(musicVolume_);
            music_.play();
            return;
        }
    }
    // Allow a later retry if the requested file was missing or unreadable.
    scene_.clear();
}
void AudioManager::play(const std::string &name) {
    if (volume_ <= 0)
        return;
    static const std::map<std::string, std::pair<std::string, std::string>> supplied = {
        {"ui_click", {"selecting-button.mp3", "ui_click.wav"}},
        {"ingredient_pop", {"pop.mp3", "topping_drop.wav"}},
        {"topping_select", {"choose-topping.mp3", "ui_click.wav"}},
        {"paper_tear", {"tear-ricepaper.mp3", "paper_tear.wav"}},
        {"mix_perfect", {"mix/perfect-mixed.mp3", "perfect.wav"}},
        {"mix_good", {"mix/good-mixed.mp3", "mix.wav"}},
        {"mix_miss", {"mix/miss-mixed.mp3", "mix.wav"}},
        {"serve_order", {"giao-mon.mp3", "coin_receive.wav"}},
        {"customer_happy", {"perfect-mixed.mp3", "perfect.wav"}},
        {"female_customer_leave", {"female_customer_leave.mp3", "customer_leave.wav"}},
        {"male_customer_leave", {"male_customer_leave.mp3", "customer_leave.wav"}},
        {"day_end", {"day_end.mp3", "day_end.wav"}},
        {"upgrade_buy", {"purchased.mp3", "upgrade_buy.wav"}}
    };
    const auto found = supplied.find(name);
    std::string path = found == supplied.end() ? "assets/audio/sfx/" + name + ".wav"
                                               : "assets/audio/sfx/sound-effect/" + found->second.first;
    const auto *buffer = resources_.sound(path);
    if (!buffer && found != supplied.end()) {
        path = "assets/audio/sfx/" + found->second.second;
        buffer = resources_.sound(path);
    }
    if (buffer) {
        if (sounds_.size() >= 24)
            sounds_.erase(sounds_.begin());
        auto sound = std::make_unique<sf::Sound>(*buffer);
        sound->setVolume(volume_);
        sound->play();
        sounds_.push_back(std::move(sound));
        lastEffectPath_ = path;
        if (name == "ingredient_pop") ++ingredientPopCount_;
    }
}
void AudioManager::charging(bool active, float seconds) {
    if (!active) {
        charging_.stop();
        chargingActive_ = false;
        return;
    }
    if (chargingActive_) return;
    chargingActive_ = true;
    if (const auto *buffer = resources_.sound("assets/audio/sfx/sound-effect/charging_sound.mp3")) {
        charging_.setBuffer(*buffer);
        charging_.setLoop(false);
        charging_.setPitch(std::clamp(buffer->getDuration().asSeconds() / std::max(.1f, seconds), .5f, 2.f));
        charging_.setVolume(volume_ * .85f);
        charging_.play();
    }
}
void AudioManager::update(float musicVolume, float soundVolume) {
    volume_ = soundVolume;
    musicVolume_ = musicVolume;
    music_.setVolume(musicVolume_);
    ambience_.setVolume(volume_ * .35f);
    charging_.setVolume(volume_ * .85f);
    sounds_.erase(std::remove_if(sounds_.begin(), sounds_.end(),
                                 [](const auto &s) { return s->getStatus() == sf::Sound::Stopped; }),
                  sounds_.end());
    for (auto &s : sounds_)
        s->setVolume(volume_);
}
} // namespace tiem

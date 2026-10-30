#pragma once
#include "core/ResourceManager.hpp"
#include <SFML/Audio.hpp>
namespace tiem {
class AudioManager {
  public:
    AudioManager(ResourceManager &, std::filesystem::path root);
    void music(const std::string &scene);
    void play(const std::string &name);
    void charging(bool active, float seconds = 2.4f);
    void update(float musicVolume, float soundVolume);
    const std::string &musicScene() const { return scene_; }
    sf::SoundSource::Status musicStatus() const { return music_.getStatus(); }
    sf::Time musicDuration() const { return music_.getDuration(); }
    bool musicLoops() const { return music_.getLoop(); }
    sf::SoundSource::Status ambienceStatus() const { return ambience_.getStatus(); }
    float ambienceVolume() const { return ambience_.getVolume(); }
    bool ambienceLoops() const { return ambience_.getLoop(); }
    unsigned ingredientPopCount() const { return ingredientPopCount_; }
    sf::SoundSource::Status chargingStatus() const { return charging_.getStatus(); }
    float chargingVolume() const { return charging_.getVolume(); }
    float chargingPitch() const { return charging_.getPitch(); }
    const std::string &lastEffectPath() const { return lastEffectPath_; }

  private:
    ResourceManager &resources_;
    std::filesystem::path root_;
    std::vector<char> musicBytes_;
    sf::Music music_;
    sf::Sound ambience_;
    sf::Sound charging_;
    bool chargingActive_{false};
    std::vector<std::unique_ptr<sf::Sound>> sounds_;
    std::string scene_, lastEffectPath_;
    unsigned ingredientPopCount_{};
    float volume_{60}, musicVolume_{32};
};
} // namespace tiem

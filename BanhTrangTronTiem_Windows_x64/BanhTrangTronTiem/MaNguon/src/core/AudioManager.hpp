#pragma once
#include "core/ResourceManager.hpp"
#include <SFML/Audio.hpp>
namespace tiem {
class AudioManager {
  public:
    AudioManager(ResourceManager &, std::filesystem::path root);
    void music(const std::string &scene);
    void play(const std::string &name);
    void update(float musicVolume, float soundVolume);

  private:
    ResourceManager &resources_;
    std::filesystem::path root_;
    std::vector<char> musicBytes_;
    sf::Music music_;
    std::vector<std::unique_ptr<sf::Sound>> sounds_;
    std::string scene_;
    float volume_{60}, musicVolume_{32};
};
} // namespace tiem

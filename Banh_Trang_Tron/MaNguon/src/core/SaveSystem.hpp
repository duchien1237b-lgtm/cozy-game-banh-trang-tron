#pragma once
#include "gameplay/Model.hpp"
namespace tiem {
class SaveSystem {
  public:
    explicit SaveSystem(std::filesystem::path folder = defaultFolder());
    Progress load(const GameData &);
    bool write(const Progress &);
    const std::string &error() const {
        return error_;
    }
    static std::filesystem::path defaultFolder();

  private:
    std::filesystem::path folder_;
    std::string error_;
};
struct GameServices {
    GameData data;
    SaveSystem saves;
    Progress progress;
    DayResult lastDay;
    bool hasResult{false};
    explicit GameServices(const std::filesystem::path &root,
                          const std::filesystem::path &saveFolder = SaveSystem::defaultFolder())
        : data(root), saves(saveFolder), progress(saves.load(data)) {}
    bool save() {
        return saves.write(progress);
    }
    bool restart() {
        auto fresh = Progress::fresh(data);
        fresh.music = progress.music;
        fresh.sfx = progress.sfx;
        if (!saves.write(fresh)) return false;
        progress = std::move(fresh);
        lastDay = {};
        hasResult = false;
        return true;
    }
};
} // namespace tiem

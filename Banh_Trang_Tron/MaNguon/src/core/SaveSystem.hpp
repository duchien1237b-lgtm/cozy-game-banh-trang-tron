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
    // Số liệu ngày đang chơi, để màn tạm dừng hiển thị mà không phải biết PlayingState.
    DayResult liveDay;
    int liveCombo{};
    bool hasResult{false};
    bool dailyPrompted{false};
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
        fresh.fullscreen = progress.fullscreen;
        // Quà hằng ngày và hướng dẫn thuộc về người chơi, không phải tiến trình quán.
        fresh.dailyStreak = progress.dailyStreak;
        fresh.lastDaily = progress.lastDaily;
        fresh.tutorialDone = progress.tutorialDone;
        fresh.achievements = progress.achievements;
        if (!saves.write(fresh)) return false;
        progress = std::move(fresh);
        lastDay = {};
        hasResult = false;
        return true;
    }
};
} // namespace tiem

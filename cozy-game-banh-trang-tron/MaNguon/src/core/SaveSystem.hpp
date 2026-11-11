#pragma once
#include "gameplay/Model.hpp"
#include <optional>
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
    // Màn hướng dẫn mở từ menu (xem lại) thì quay về menu thay vì vào ngày chơi.
    bool guideFromMenu{false};
    bool capturingKey{false};
    std::optional<Progress> dayCheckpoint;
    void beginDay() { dayCheckpoint = progress; }
    void leaveDay() {
        if (!dayCheckpoint) return;
        if (progress.day == dayCheckpoint->day) {
            preserveSettings(*dayCheckpoint);
            progress = std::move(*dayCheckpoint);
        }
        dayCheckpoint.reset();
    }
    explicit GameServices(const std::filesystem::path &root,
                          const std::filesystem::path &saveFolder = SaveSystem::defaultFolder())
        : data(root), saves(saveFolder), progress(saves.load(data)) {}
    bool save() {
        // All callers (settings, window close, autosave) use the same day boundary.
        if (dayCheckpoint && progress.day > dayCheckpoint->day) dayCheckpoint.reset();
        if (dayCheckpoint) {
            preserveSettings(*dayCheckpoint);
            return saves.write(*dayCheckpoint);
        }
        return saves.write(progress);
    }
    void preserveSettings(Progress &saved) const {
        saved.keys = progress.keys;
        saved.music = progress.music;
        saved.sfx = progress.sfx;
        saved.fullscreen = progress.fullscreen;
        saved.tutorialDone = progress.tutorialDone;
        saved.guideDone = progress.guideDone;
    }
    bool restart() {
        auto fresh = Progress::fresh(data);
        fresh.keys = progress.keys;
        fresh.music = progress.music;
        fresh.sfx = progress.sfx;
        fresh.fullscreen = progress.fullscreen;
        // Quà hằng ngày và hướng dẫn thuộc về người chơi, không phải tiến trình quán.
        fresh.dailyStreak = progress.dailyStreak;
        fresh.lastDaily = progress.lastDaily;
        fresh.tutorialDone = progress.tutorialDone;
        fresh.guideDone = progress.guideDone;
        fresh.achievements = progress.achievements;
        if (!saves.write(fresh)) return false;
        progress = std::move(fresh);
        dayCheckpoint.reset();
        lastDay = {};
        hasResult = false;
        return true;
    }
};
} // namespace tiem

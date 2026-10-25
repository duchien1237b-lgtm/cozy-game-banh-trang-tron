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
    explicit GameServices(const std::filesystem::path &root) : data(root), progress(saves.load(data)) {}
    bool save() {
        return saves.write(progress);
    }
};
} // namespace tiem

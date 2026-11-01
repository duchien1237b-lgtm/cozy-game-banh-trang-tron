#pragma once
#include "core/State.hpp"
#include <map>
#include <memory>
#include <vector>

namespace tiem {
class StateStack {
  public:
    using Factory = std::function<std::unique_ptr<State>()>;
    void registerFactory(StateID id, Factory factory);
    void push(StateID id);
    void pop();
    void replace(StateID id);
    void clear();
    void handleEvent(const sf::Event &event, Pointer mouse);
    void update(float dt);
    void render(sf::RenderTarget &target);
    void applyPendingChanges();
    bool empty() const {
        return entries_.empty();
    }
    std::size_t size() const {
        return entries_.size();
    }
    std::optional<StateID> top() const;
    // Tăng mỗi khi đổi màn (replace/clear) để Game chạy hiệu ứng chuyển cảnh.
    unsigned sceneChanges() const {
        return sceneChanges_;
    }

  private:
    enum class Action { Push, Pop, Replace, Clear };
    struct Change {
        Action action;
        StateID id;
    };
    struct Entry {
        StateID id;
        std::unique_ptr<State> state;
    };
    std::unique_ptr<State> create(StateID id);
    std::map<StateID, Factory> factories_;
    std::vector<Entry> entries_;
    std::vector<Change> pending_;
    unsigned sceneChanges_{};
};
} // namespace tiem

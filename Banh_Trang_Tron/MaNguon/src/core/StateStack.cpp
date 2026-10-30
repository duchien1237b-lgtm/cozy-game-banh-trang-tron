#include "core/StateStack.hpp"
#include <stdexcept>
#include <utility>

namespace tiem {
void StateStack::registerFactory(StateID id, Factory factory) {
    factories_[id] = std::move(factory);
}
void StateStack::push(StateID id) {
    pending_.push_back({Action::Push, id});
}
void StateStack::pop() {
    pending_.push_back({Action::Pop, StateID::MainMenu});
}
void StateStack::replace(StateID id) {
    pending_.push_back({Action::Replace, id});
}
void StateStack::clear() {
    pending_.push_back({Action::Clear, StateID::MainMenu});
}

std::unique_ptr<State> StateStack::create(StateID id) {
    const auto found = factories_.find(id);
    if (found == factories_.end())
        throw std::logic_error("State chua duoc dang ky");
    auto result = found->second();
    if (!result)
        throw std::logic_error("Factory tra ve state rong");
    return result;
}

void StateStack::applyPendingChanges() {
    // Chỉ đổi stack sau vòng duyệt: callback không tự hủy state đang chạy.
    auto changes = std::move(pending_);
    pending_.clear();
    for (const auto &change : changes) {
        if (change.action == Action::Clear) {
            entries_.clear();
            ++sceneChanges_;
        }
        else if (change.action == Action::Pop) {
            if (!entries_.empty())
                entries_.pop_back();
        } else {
            auto next = create(change.id);
            if (change.action == Action::Replace) ++sceneChanges_;
            if (change.action == Action::Replace && !entries_.empty())
                entries_.pop_back();
            if (!entries_.empty())
                entries_.back().state->onCovered();
            entries_.push_back({change.id, std::move(next)});
        }
    }
}

void StateStack::handleEvent(const sf::Event &event, Pointer mouse) {
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it)
        if (!it->state->handleEvent(event, mouse))
            break;
    applyPendingChanges();
}
void StateStack::update(float dt) {
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it)
        if (!it->state->update(dt))
            break;
    applyPendingChanges();
}
void StateStack::render(sf::RenderTarget &target) {
    std::size_t start = entries_.size();
    while (start > 0) {
        --start;
        if (!entries_[start].state->transparent())
            break;
    }
    for (auto i = start; i < entries_.size(); ++i)
        entries_[i].state->render(target);
}
std::optional<StateID> StateStack::top() const {
    if (entries_.empty())
        return std::nullopt;
    return entries_.back().id;
}
} // namespace tiem

#include "nemisis/player/PlayerCommandQueue.hpp"

#include <algorithm>

namespace nemisis::player {

PlayerCommandQueue::PlayerCommandQueue(std::size_t capacity)
    : capacity_(std::max<std::size_t>(1, capacity)) {
}

bool PlayerCommandQueue::push(PlayerInputCommand command) {
    if (!commands_.empty()) {
        auto& latestCommand = commands_.back();
        if (command.tick < latestCommand.tick) {
            return false;
        }
        if (command.tick == latestCommand.tick) {
            latestCommand = command;
            return true;
        }
    }

    while (commands_.size() >= capacity_) {
        commands_.pop_front();
    }

    commands_.push_back(command);
    return true;
}

std::size_t PlayerCommandQueue::acknowledgeThrough(std::uint64_t tick) {
    std::size_t removed = 0;
    while (!commands_.empty() && commands_.front().tick <= tick) {
        commands_.pop_front();
        ++removed;
    }
    return removed;
}

void PlayerCommandQueue::clear() {
    commands_.clear();
}

std::vector<PlayerInputCommand> PlayerCommandQueue::pendingCommands() const {
    return {commands_.begin(), commands_.end()};
}

const PlayerInputCommand* PlayerCommandQueue::latest() const {
    if (commands_.empty()) {
        return nullptr;
    }
    return &commands_.back();
}

std::size_t PlayerCommandQueue::size() const {
    return commands_.size();
}

std::size_t PlayerCommandQueue::capacity() const {
    return capacity_;
}

bool PlayerCommandQueue::empty() const {
    return commands_.empty();
}

} // namespace nemisis::player

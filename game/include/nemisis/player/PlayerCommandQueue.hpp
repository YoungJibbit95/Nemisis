#pragma once

#include "nemisis/player/PlayerInputCommand.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace nemisis::player {

class PlayerCommandQueue final {
public:
    explicit PlayerCommandQueue(std::size_t capacity = 128);

    [[nodiscard]] bool push(PlayerInputCommand command);
    [[nodiscard]] std::size_t acknowledgeThrough(std::uint64_t tick);
    void clear();

    [[nodiscard]] std::vector<PlayerInputCommand> pendingCommands() const;
    [[nodiscard]] const PlayerInputCommand* latest() const;
    [[nodiscard]] std::size_t size() const;
    [[nodiscard]] std::size_t capacity() const;
    [[nodiscard]] bool empty() const;

private:
    std::size_t capacity_ = 128;
    std::deque<PlayerInputCommand> commands_;
};

} // namespace nemisis::player

#pragma once

#include "nemisis/net/SnapshotMessages.hpp"

#include "novacore/net/InterpolationBuffer.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace nemisis::net {

inline constexpr std::size_t kPlayerSnapshotInterpolationCapacity = 128;
inline constexpr std::uint64_t kDefaultSnapshotInterpolationDelayTicks = 2;

struct InterpolatedPlayerSnapshot final {
    player::PlayerId playerId = 0;
    std::uint64_t renderTick = 0;
    std::uint64_t fromTick = 0;
    std::uint64_t toTick = 0;
    float alpha = 0.0F;
    AuthoritativePlayerState state{};
    bool valid = false;
    bool exact = false;
    bool clamped = false;
};

struct PlayerSnapshotInterpolationStats final {
    std::size_t storedSnapshots = 0;
    std::uint64_t newestServerTick = 0;
    std::uint64_t oldestServerTick = 0;
    bool hasNewestServerTick = false;
    bool hasOldestServerTick = false;
};

class PlayerSnapshotInterpolator final {
public:
    [[nodiscard]] bool store(PlayerSnapshotState snapshot);
    [[nodiscard]] std::optional<PlayerSnapshotState> find(std::uint64_t serverTick) const;
    [[nodiscard]] InterpolatedPlayerSnapshot sample(std::uint64_t renderTick) const;
    [[nodiscard]] InterpolatedPlayerSnapshot sampleDelayed(
        std::uint64_t newestServerTick,
        std::uint64_t delayTicks = kDefaultSnapshotInterpolationDelayTicks) const;
    [[nodiscard]] std::size_t pruneThrough(std::uint64_t serverTick);
    [[nodiscard]] std::size_t rollbackAfter(std::uint64_t serverTick);
    void clear();

    [[nodiscard]] PlayerSnapshotInterpolationStats stats() const;

private:
    [[nodiscard]] InterpolatedPlayerSnapshot interpolate(
        std::uint64_t renderTick,
        const novacore::net::InterpolationBuffer<
            PlayerSnapshotState,
            kPlayerSnapshotInterpolationCapacity>::Sample& sample) const;

    novacore::net::InterpolationBuffer<PlayerSnapshotState, kPlayerSnapshotInterpolationCapacity> snapshots_;
};

} // namespace nemisis::net

#pragma once

#include "nemisis/dev/DebugTarget.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace nemisis::dev {

struct DevTargetLane final {
    std::string id;
    std::string displayName;
    DebugTargetState target{};
    float respawnSeconds = 0.0F;
};

struct DevTargetRangeState final {
    std::vector<DevTargetLane> lanes;
    std::size_t activeLaneIndex = 0;
};

struct DevTargetRangeHitResult final {
    bool hit = false;
    std::size_t laneIndex = 0;
    std::string laneId;
    std::string laneName;
    DebugTargetHitResult targetHit{};
};

[[nodiscard]] DevTargetRangeState makeDefaultDevTargetRange();

void ensureDevTargetRange(DevTargetRangeState& range);
void resetDevTargetRange(DevTargetRangeState& range);

[[nodiscard]] const DevTargetLane* activeTargetLane(const DevTargetRangeState& range);
[[nodiscard]] DevTargetLane* activeTargetLane(DevTargetRangeState& range);
[[nodiscard]] const DebugTargetState* activeTarget(const DevTargetRangeState& range);

[[nodiscard]] DevTargetRangeHitResult applyShotToDevTargetRange(
    DevTargetRangeState& range,
    const weapons::ShotTraceResult& shot);

void beginTargetLaneRespawn(
    DevTargetRangeState& range,
    std::size_t laneIndex,
    float respawnDelaySeconds);

[[nodiscard]] std::uint32_t tickDevTargetRangeRespawns(
    DevTargetRangeState& range,
    float deltaSeconds);

[[nodiscard]] float nextTargetRespawnSeconds(const DevTargetRangeState& range);
[[nodiscard]] std::size_t totalTargetCount(const DevTargetRangeState& range);
[[nodiscard]] std::size_t aliveTargetCount(const DevTargetRangeState& range);
[[nodiscard]] std::size_t eliminatedTargetCount(const DevTargetRangeState& range);

} // namespace nemisis::dev

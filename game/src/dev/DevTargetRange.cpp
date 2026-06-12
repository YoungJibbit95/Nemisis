#include "nemisis/dev/DevTargetRange.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace nemisis::dev {

namespace {

[[nodiscard]] float consume(float value, float deltaSeconds) {
    return std::max(0.0F, value - std::max(0.0F, deltaSeconds));
}

[[nodiscard]] DevTargetLane makeLane(
    std::string id,
    std::string displayName,
    novacore::math::Vec3 position,
    float maxHealth,
    float radiusMeters) {
    DevTargetLane lane{};
    lane.id = std::move(id);
    lane.displayName = std::move(displayName);
    lane.target.position = position;
    lane.target.maxHealth = maxHealth;
    lane.target.health = maxHealth;
    lane.target.radiusMeters = radiusMeters;
    return lane;
}

[[nodiscard]] std::size_t clampActiveIndex(const DevTargetRangeState& range) {
    if (range.lanes.empty()) {
        return 0U;
    }
    return std::min(range.activeLaneIndex, range.lanes.size() - 1U);
}

} // namespace

DevTargetRangeState makeDefaultDevTargetRange() {
    DevTargetRangeState range{};
    range.lanes.reserve(4U);
    range.lanes.push_back(makeLane(
        "left_15m",
        "LEFT 15M",
        {-5.5F, 1.45F, 15.0F},
        150.0F,
        0.72F));
    range.lanes.push_back(makeLane(
        "center_20m",
        "CENTER 20M",
        {0.0F, 1.65F, 18.0F},
        150.0F,
        0.85F));
    range.lanes.push_back(makeLane(
        "right_25m",
        "RIGHT 25M",
        {5.5F, 1.55F, 20.0F},
        150.0F,
        0.72F));
    range.lanes.push_back(makeLane(
        "upper_22m",
        "UPPER 22M",
        {0.0F, 2.60F, 16.5F},
        125.0F,
        0.58F));
    range.activeLaneIndex = 1U;
    return range;
}

void ensureDevTargetRange(DevTargetRangeState& range) {
    if (range.lanes.empty()) {
        range = makeDefaultDevTargetRange();
        return;
    }
    range.activeLaneIndex = clampActiveIndex(range);
}

void resetDevTargetRange(DevTargetRangeState& range) {
    ensureDevTargetRange(range);
    for (auto& lane : range.lanes) {
        resetDebugTarget(lane.target);
        lane.respawnSeconds = 0.0F;
    }
    range.activeLaneIndex = clampActiveIndex(range);
}

const DevTargetLane* activeTargetLane(const DevTargetRangeState& range) {
    if (range.lanes.empty()) {
        return nullptr;
    }
    return &range.lanes[clampActiveIndex(range)];
}

DevTargetLane* activeTargetLane(DevTargetRangeState& range) {
    ensureDevTargetRange(range);
    if (range.lanes.empty()) {
        return nullptr;
    }
    return &range.lanes[clampActiveIndex(range)];
}

const DebugTargetState* activeTarget(const DevTargetRangeState& range) {
    const auto* lane = activeTargetLane(range);
    return lane == nullptr ? nullptr : &lane->target;
}

DevTargetRangeHitResult applyShotToDevTargetRange(
    DevTargetRangeState& range,
    const weapons::ShotTraceResult& shot) {
    ensureDevTargetRange(range);

    DevTargetRangeHitResult result{};
    float bestDistance = std::numeric_limits<float>::max();
    std::size_t bestIndex = range.lanes.size();

    for (std::size_t index = 0; index < range.lanes.size(); ++index) {
        const auto& lane = range.lanes[index];
        const auto trace = traceShotToDebugTarget(lane.target, shot);
        if (!trace.hit || trace.distanceMeters >= bestDistance) {
            continue;
        }
        bestDistance = trace.distanceMeters;
        bestIndex = index;
    }

    if (bestIndex >= range.lanes.size()) {
        return result;
    }

    auto& lane = range.lanes[bestIndex];
    range.activeLaneIndex = bestIndex;
    result.hit = true;
    result.laneIndex = bestIndex;
    result.laneId = lane.id;
    result.laneName = lane.displayName;
    result.targetHit = applyShotToDebugTarget(lane.target, shot);
    return result;
}

void beginTargetLaneRespawn(
    DevTargetRangeState& range,
    std::size_t laneIndex,
    float respawnDelaySeconds) {
    ensureDevTargetRange(range);
    if (laneIndex >= range.lanes.size()) {
        return;
    }
    range.lanes[laneIndex].respawnSeconds = std::max(0.0F, respawnDelaySeconds);
}

std::uint32_t tickDevTargetRangeRespawns(
    DevTargetRangeState& range,
    float deltaSeconds) {
    ensureDevTargetRange(range);

    std::uint32_t respawned = 0;
    for (auto& lane : range.lanes) {
        if (lane.respawnSeconds <= 0.0F) {
            continue;
        }

        lane.respawnSeconds = consume(lane.respawnSeconds, deltaSeconds);
        if (lane.respawnSeconds <= 0.0F) {
            resetDebugTarget(lane.target);
            ++respawned;
        }
    }
    return respawned;
}

float nextTargetRespawnSeconds(const DevTargetRangeState& range) {
    float next = 0.0F;
    for (const auto& lane : range.lanes) {
        if (lane.respawnSeconds <= 0.0F) {
            continue;
        }
        next = next <= 0.0F ? lane.respawnSeconds : std::min(next, lane.respawnSeconds);
    }
    return next;
}

std::size_t totalTargetCount(const DevTargetRangeState& range) {
    return range.lanes.size();
}

std::size_t aliveTargetCount(const DevTargetRangeState& range) {
    return static_cast<std::size_t>(std::count_if(
        range.lanes.begin(),
        range.lanes.end(),
        [](const DevTargetLane& lane) {
            return !lane.target.eliminated;
        }));
}

std::size_t eliminatedTargetCount(const DevTargetRangeState& range) {
    return totalTargetCount(range) - aliveTargetCount(range);
}

} // namespace nemisis::dev

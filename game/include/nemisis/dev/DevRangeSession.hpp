#pragma once

#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerHealth.hpp"
#include "nemisis/weapons/WeaponSimulation.hpp"

#include <cstdint>
#include <string>

namespace nemisis::dev {

struct DevRangeScoreboard final {
    std::uint32_t shotsFired = 0;
    std::uint32_t shotsHit = 0;
    std::uint32_t targetsEliminated = 0;
    std::uint32_t rangeResets = 0;
    std::uint32_t playerRespawns = 0;
    std::uint32_t currentStreak = 0;
    std::uint32_t bestStreak = 0;
    float damageDealt = 0.0F;
};

struct DevRangeSessionState final {
    DevRangeScoreboard score{};
    float targetRespawnSeconds = 0.0F;
    float playerRespawnSeconds = 0.0F;
    float eventTextSeconds = 0.0F;
    float playerRegenDelaySeconds = 0.0F;
    std::string eventText;
};

struct DevRangeSessionTuning final {
    float targetRespawnDelaySeconds = 1.5F;
    float playerRespawnDelaySeconds = 1.75F;
    float eventTextSeconds = 1.4F;
    float playerRegenDelaySeconds = 3.0F;
    float playerRegenPerSecond = 12.0F;
};

void resetDevRangeSession(DevRangeSessionState& session);

void recordRangeReset(DevRangeSessionState& session, const DevRangeSessionTuning& tuning = {});

void recordShotResult(
    DevRangeSessionState& session,
    const weapons::FireResult& fire,
    const DebugTargetHitResult& hit,
    const DevRangeSessionTuning& tuning = {});

void beginTargetRespawn(DevRangeSessionState& session, const DevRangeSessionTuning& tuning = {});

[[nodiscard]] bool tickTargetRespawn(
    DevRangeSessionState& session,
    float deltaSeconds);

void recordPlayerDamage(
    DevRangeSessionState& session,
    const player::DamageResult& damage,
    const DevRangeSessionTuning& tuning = {});

void beginPlayerRespawn(
    DevRangeSessionState& session,
    player::PlayerHealthComponent& health,
    const DevRangeSessionTuning& tuning = {});

[[nodiscard]] bool tickPlayerRespawn(
    DevRangeSessionState& session,
    player::PlayerHealthComponent& health,
    float deltaSeconds);

void tickPlayerRegen(
    DevRangeSessionState& session,
    player::PlayerHealthComponent& health,
    float deltaSeconds,
    const DevRangeSessionTuning& tuning = {});

void tickSessionFeedback(DevRangeSessionState& session, float deltaSeconds);

[[nodiscard]] float devRangeAccuracy(const DevRangeScoreboard& score);

} // namespace nemisis::dev

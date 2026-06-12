#include "nemisis/dev/DevRangeSession.hpp"

#include <algorithm>
#include <sstream>
#include <utility>

namespace nemisis::dev {

namespace {

[[nodiscard]] float consume(float value, float deltaSeconds) {
    return std::max(0.0F, value - std::max(0.0F, deltaSeconds));
}

void setEvent(DevRangeSessionState& session, std::string text, const DevRangeSessionTuning& tuning) {
    session.eventText = std::move(text);
    session.eventTextSeconds = std::max(0.0F, tuning.eventTextSeconds);
}

} // namespace

void resetDevRangeSession(DevRangeSessionState& session) {
    session = {};
}

void recordRangeReset(DevRangeSessionState& session, const DevRangeSessionTuning& tuning) {
    ++session.score.rangeResets;
    session.targetRespawnSeconds = 0.0F;
    session.playerRespawnSeconds = 0.0F;
    session.playerRegenDelaySeconds = 0.0F;
    session.score.currentStreak = 0;
    setEvent(session, "Range reset", tuning);
}

void recordShotResult(
    DevRangeSessionState& session,
    const weapons::FireResult& fire,
    const DebugTargetHitResult& hit,
    const DevRangeSessionTuning& tuning) {
    if (!fire.fired) {
        return;
    }

    ++session.score.shotsFired;
    if (!hit.hit) {
        session.score.currentStreak = 0;
        setEvent(session, "Miss", tuning);
        return;
    }

    ++session.score.shotsHit;
    session.score.damageDealt += hit.damageApplied;
    ++session.score.currentStreak;
    session.score.bestStreak = std::max(session.score.bestStreak, session.score.currentStreak);

    if (hit.eliminated) {
        ++session.score.targetsEliminated;
        std::ostringstream stream;
        stream << "Target eliminated  streak " << session.score.currentStreak;
        setEvent(session, stream.str(), tuning);
    } else {
        std::ostringstream stream;
        stream << "Hit -" << static_cast<int>(hit.damageApplied);
        setEvent(session, stream.str(), tuning);
    }
}

void beginTargetRespawn(DevRangeSessionState& session, const DevRangeSessionTuning& tuning) {
    session.targetRespawnSeconds = std::max(session.targetRespawnSeconds, tuning.targetRespawnDelaySeconds);
}

bool tickTargetRespawn(
    DevRangeSessionState& session,
    float deltaSeconds) {
    if (session.targetRespawnSeconds <= 0.0F) {
        return false;
    }

    session.targetRespawnSeconds = consume(session.targetRespawnSeconds, deltaSeconds);
    return session.targetRespawnSeconds <= 0.0F;
}

void recordPlayerDamage(
    DevRangeSessionState& session,
    const player::DamageResult& damage,
    const DevRangeSessionTuning& tuning) {
    if (damage.damageApplied <= 0.0F) {
        return;
    }

    session.playerRegenDelaySeconds = tuning.playerRegenDelaySeconds;
    if (damage.eliminated) {
        session.score.currentStreak = 0;
        setEvent(session, "Player down", tuning);
    } else {
        std::ostringstream stream;
        stream << "Armor hit -" << static_cast<int>(damage.damageApplied);
        setEvent(session, stream.str(), tuning);
    }
}

void beginPlayerRespawn(
    DevRangeSessionState& session,
    player::PlayerHealthComponent& health,
    const DevRangeSessionTuning& tuning) {
    health.eliminated = true;
    health.health = 0.0F;
    session.playerRespawnSeconds = std::max(session.playerRespawnSeconds, tuning.playerRespawnDelaySeconds);
}

bool tickPlayerRespawn(
    DevRangeSessionState& session,
    player::PlayerHealthComponent& health,
    float deltaSeconds) {
    if (session.playerRespawnSeconds <= 0.0F) {
        return false;
    }

    session.playerRespawnSeconds = consume(session.playerRespawnSeconds, deltaSeconds);
    if (session.playerRespawnSeconds > 0.0F) {
        return false;
    }

    player::resetHealth(health);
    ++session.score.playerRespawns;
    session.playerRegenDelaySeconds = 0.0F;
    return true;
}

void tickPlayerRegen(
    DevRangeSessionState& session,
    player::PlayerHealthComponent& health,
    float deltaSeconds,
    const DevRangeSessionTuning& tuning) {
    if (!player::isAlive(health)) {
        return;
    }
    session.playerRegenDelaySeconds = consume(session.playerRegenDelaySeconds, deltaSeconds);
    if (session.playerRegenDelaySeconds > 0.0F || health.health >= health.maxHealth) {
        return;
    }

    health.health = std::min(
        health.maxHealth,
        health.health + (std::max(0.0F, tuning.playerRegenPerSecond) * std::max(0.0F, deltaSeconds)));
}

void tickSessionFeedback(DevRangeSessionState& session, float deltaSeconds) {
    session.eventTextSeconds = consume(session.eventTextSeconds, deltaSeconds);
    if (session.eventTextSeconds <= 0.0F) {
        session.eventText.clear();
    }
}

float devRangeAccuracy(const DevRangeScoreboard& score) {
    if (score.shotsFired == 0U) {
        return 0.0F;
    }
    return static_cast<float>(score.shotsHit) / static_cast<float>(score.shotsFired);
}

} // namespace nemisis::dev

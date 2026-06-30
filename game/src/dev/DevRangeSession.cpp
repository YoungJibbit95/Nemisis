#include "nemisis/dev/DevRangeSession.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <utility>

namespace nemisis::dev {

namespace {

constexpr float kDefaultDrillTimeLimitSeconds = 60.0F;

constexpr std::array<DevRangeDrillRules, kDevRangeDrillVariantCount> kDrillRules{
    DevRangeDrillRules{
        DevRangeDrillVariant::Precision,
        "PRECISION",
        "ACCURACY",
        70.0F,
        90.0F,
        1.00F,
        8.0F,
        320.0F,
        260.0F,
        2.2F,
        80.0F,
        0.20F,
        60.0F,
        1.0F,
        1.72F,
        0.42F,
        0.08F,
        0.04F,
        0.18F,
        3.8F,
    },
    DevRangeDrillRules{
        DevRangeDrillVariant::RecoilControl,
        "RECOIL CONTROL",
        "RECOIL",
        60.0F,
        55.0F,
        0.55F,
        5.0F,
        260.0F,
        180.0F,
        1.8F,
        70.0F,
        0.85F,
        90.0F,
        1.0F,
        1.78F,
        0.12F,
        0.58F,
        0.04F,
        0.10F,
        2.9F,
    },
    DevRangeDrillRules{
        DevRangeDrillVariant::SpeedClear,
        "SPEED CLEAR",
        "TTK",
        42.0F,
        62.0F,
        0.62F,
        6.0F,
        540.0F,
        150.0F,
        1.15F,
        170.0F,
        0.12F,
        35.0F,
        1.0F,
        1.86F,
        0.08F,
        0.06F,
        0.58F,
        0.14F,
        4.6F,
    },
};

[[nodiscard]] std::size_t variantIndex(DevRangeDrillVariant variant) {
    const auto index = static_cast<std::size_t>(variant);
    return index < kDevRangeDrillVariantCount ? index : 0U;
}

[[nodiscard]] DevRangeDrillVariant safeVariant(DevRangeDrillVariant variant) {
    return variantIndex(variant) == static_cast<std::size_t>(variant)
        ? variant
        : DevRangeDrillVariant::Precision;
}

[[nodiscard]] const DevRangeDrillRules& rulesForVariant(DevRangeDrillVariant variant) {
    return kDrillRules[variantIndex(variant)];
}

[[nodiscard]] float consume(float value, float deltaSeconds) {
    return std::max(0.0F, value - std::max(0.0F, deltaSeconds));
}

void setEvent(DevRangeSessionState& session, std::string text, const DevRangeSessionTuning& tuning) {
    session.eventText = std::move(text);
    session.eventTextSeconds = std::max(0.0F, tuning.eventTextSeconds);
}

[[nodiscard]] std::uint32_t roundedScore(float value) {
    return static_cast<std::uint32_t>(std::max(0.0F, std::round(value)));
}

[[nodiscard]] float drillTimeLimitSeconds(
    const DevRangeSessionTuning& tuning,
    DevRangeDrillVariant variant) {
    if (std::abs(tuning.drillTimeLimitSeconds - kDefaultDrillTimeLimitSeconds) > 0.001F) {
        return std::max(1.0F, tuning.drillTimeLimitSeconds);
    }
    return std::max(1.0F, rulesForVariant(variant).timeLimitSeconds);
}

[[nodiscard]] float shotErrorDegrees(const weapons::FireResult& fire, bool hit) {
    const float recoil =
        (std::abs(fire.recoilPitchOffsetDegrees) * 0.75F) +
        (std::abs(fire.recoilYawOffsetDegrees) * 1.10F);
    const float spread = std::max(0.0F, fire.movementSpreadDegrees) * 0.60F;
    const float missPenalty = hit ? 0.0F : 1.10F;
    return recoil + spread + missPenalty;
}

[[nodiscard]] float recoilControlFromAverage(float averageErrorDegrees, const DevRangeDrillRules& rules) {
    const float softCap = std::max(0.25F, rules.recoilErrorSoftCapDegrees);
    return std::clamp(100.0F * (1.0F - (std::max(0.0F, averageErrorDegrees) / softCap)), 0.0F, 100.0F);
}

void preserveCurrentVariantBest(DevRangeDrillState& drill) {
    const auto index = variantIndex(drill.variant);
    drill.bestScoreByVariant[index] = std::max(drill.bestScoreByVariant[index], drill.score);
    drill.bestScore = std::max(drill.bestScore, drill.score);
}

void restartDrill(DevRangeSessionState& session, const DevRangeSessionTuning& tuning) {
    const auto variant = safeVariant(session.drill.variant);
    const auto bestScore = session.drill.bestScore;
    auto bestScoreByVariant = session.drill.bestScoreByVariant;
    const auto bestTtk = session.drill.bestTtkSeconds;
    session.drill = {};
    session.drill.variant = variant;
    session.drill.bestScoreByVariant = bestScoreByVariant;
    session.drill.bestScore = std::max(bestScore, session.drill.bestScoreByVariant[variantIndex(variant)]);
    session.drill.bestTtkSeconds = bestTtk;
    session.drill.timeLimitSeconds = drillTimeLimitSeconds(tuning, variant);
    session.drill.timeRemainingSeconds = session.drill.timeLimitSeconds;
    session.drill.status = DevRangeDrillStatus::Active;
    session.laneScores.clear();
}

[[nodiscard]] DevRangeLaneScore* findLaneScore(
    DevRangeSessionState& session,
    std::size_t laneIndex) {
    const auto it = std::find_if(
        session.laneScores.begin(),
        session.laneScores.end(),
        [laneIndex](const DevRangeLaneScore& lane) {
            return lane.laneIndex == laneIndex;
        });
    return it == session.laneScores.end() ? nullptr : &*it;
}

[[nodiscard]] const DevRangeLaneScore* findLaneScore(
    const DevRangeSessionState& session,
    std::size_t laneIndex) {
    const auto it = std::find_if(
        session.laneScores.begin(),
        session.laneScores.end(),
        [laneIndex](const DevRangeLaneScore& lane) {
            return lane.laneIndex == laneIndex;
        });
    return it == session.laneScores.end() ? nullptr : &*it;
}

void resetLaneTargetWindow(DevRangeLaneScore& lane) {
    lane.currentTargetShots = 0;
    lane.currentTargetHits = 0;
    lane.currentTargetDamage = 0.0F;
    lane.currentTargetFirstHitSeconds = -1.0F;
}

void resetLatestScoreTelemetry(DevRangeDrillState& drill) {
    drill.scoring.latestRawPoints = 0.0F;
    drill.scoring.latestBonusPoints = 0.0F;
    drill.scoring.latestScaledPoints = 0.0F;
    drill.scoring.latestPenaltyPoints = 0.0F;
    drill.scoring.latestScoreDelta = 0;
}

void updateRecoilControl(
    DevRangeDrillState& drill,
    const weapons::FireResult& fire,
    bool hit,
    const DevRangeDrillRules& rules) {
    drill.latestRecoilErrorDegrees = shotErrorDegrees(fire, hit);
    if (drill.shotsFired == 0U) {
        drill.averageRecoilErrorDegrees = drill.latestRecoilErrorDegrees;
    } else {
        const float previousWeight = static_cast<float>(drill.shotsFired - 1U);
        drill.averageRecoilErrorDegrees =
            ((drill.averageRecoilErrorDegrees * previousWeight) + drill.latestRecoilErrorDegrees) /
            static_cast<float>(drill.shotsFired);
    }
    drill.recoilControlScore = recoilControlFromAverage(drill.averageRecoilErrorDegrees, rules);
    drill.recoilHud.valid = true;
    drill.recoilHud.hit = hit;
    drill.recoilHud.shotIndex = fire.shotIndex;
    drill.recoilHud.pitchDegrees = fire.recoilPitchOffsetDegrees;
    drill.recoilHud.yawDegrees = fire.recoilYawOffsetDegrees;
    drill.recoilHud.spreadDegrees = fire.movementSpreadDegrees;
    drill.recoilHud.errorDegrees = drill.latestRecoilErrorDegrees;
    drill.recoilHud.averageErrorDegrees = drill.averageRecoilErrorDegrees;
    drill.recoilHud.control01 = std::clamp(drill.recoilControlScore / 100.0F, 0.0F, 1.0F);
}

void updateScoreMultiplierTelemetry(DevRangeSessionState& session, const DevRangeDrillRules& rules) {
    auto& scoring = session.drill.scoring;
    scoring.accuracyFactor = devRangeDrillAccuracy(session.drill);
    scoring.recoilFactor = std::clamp(session.drill.recoilControlScore / 100.0F, 0.0F, 1.0F);
    scoring.speedFactor = session.drill.timeLimitSeconds > 0.0F
        ? std::clamp(session.drill.timeRemainingSeconds / session.drill.timeLimitSeconds, 0.0F, 1.0F)
        : 0.0F;
    scoring.streakFactor = std::clamp(static_cast<float>(session.score.currentStreak) / 10.0F, 0.0F, 1.0F);
    scoring.scoreMultiplier = std::clamp(
        rules.minScoreMultiplier +
            (scoring.accuracyFactor * rules.accuracyMultiplierScale) +
            (scoring.recoilFactor * rules.recoilMultiplierScale) +
            (scoring.speedFactor * rules.speedMultiplierScale) +
            (scoring.streakFactor * rules.streakMultiplierScale),
        rules.minScoreMultiplier,
        rules.maxScoreMultiplier);
}

[[nodiscard]] int addDrillScore(DevRangeDrillState& drill, int points) {
    const auto previousScore = drill.score;
    if (points < 0) {
        const auto penalty = static_cast<std::uint32_t>(-points);
        drill.score = penalty >= drill.score ? 0U : drill.score - penalty;
    } else {
        drill.score += static_cast<std::uint32_t>(points);
    }
    drill.bestScore = std::max(drill.bestScore, drill.score);
    drill.bestScoreByVariant[variantIndex(drill.variant)] =
        std::max(drill.bestScoreByVariant[variantIndex(drill.variant)], drill.score);
    return static_cast<int>(drill.score) - static_cast<int>(previousScore);
}

void addDrillPenalty(DevRangeDrillState& drill, float points) {
    const int penalty = static_cast<int>(std::round(std::max(0.0F, points)));
    const int delta = addDrillScore(drill, -penalty);
    drill.scoring.latestPenaltyPoints += static_cast<float>(-delta);
    drill.scoring.latestScoreDelta += delta;
}

void addPositiveDrillScore(DevRangeDrillState& drill, float rawPoints, bool bonus) {
    const float safeRaw = std::max(0.0F, rawPoints);
    const int scaledPoints = static_cast<int>(roundedScore(safeRaw * drill.scoring.scoreMultiplier));
    const int delta = addDrillScore(drill, scaledPoints);
    if (bonus) {
        drill.scoring.latestBonusPoints += safeRaw;
    } else {
        drill.scoring.latestRawPoints += safeRaw;
    }
    drill.scoring.latestScaledPoints += static_cast<float>(scaledPoints);
    drill.scoring.latestScoreDelta += delta;
}

void appendScoreDelta(std::ostringstream& stream, const DevRangeDrillState& drill) {
    if (drill.scoring.latestScoreDelta == 0) {
        return;
    }
    stream << ' ';
    if (drill.scoring.latestScoreDelta > 0) {
        stream << '+';
    }
    stream << drill.scoring.latestScoreDelta
           << " x" << std::fixed << std::setprecision(2) << drill.scoring.scoreMultiplier;
}

void recordLaneTtk(
    DevRangeSessionState& session,
    DevRangeLaneScore& lane,
    const DevRangeShotScoreContext& context) {
    const float firstHit = lane.currentTargetFirstHitSeconds >= 0.0F
        ? lane.currentTargetFirstHitSeconds
        : session.drill.elapsedSeconds;
    const float ttk = std::max(0.0F, session.drill.elapsedSeconds - firstHit);
    lane.latestTtkSeconds = ttk;
    lane.bestTtkSeconds = lane.bestTtkSeconds < 0.0F ? ttk : std::min(lane.bestTtkSeconds, ttk);
    lane.latestDistanceMeters = std::max(0.0F, context.distanceMeters);
    session.drill.latestTtkSeconds = ttk;
    session.drill.bestTtkSeconds = session.drill.bestTtkSeconds < 0.0F
        ? ttk
        : std::min(session.drill.bestTtkSeconds, ttk);
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
    preserveCurrentVariantBest(session.drill);
    restartDrill(session, tuning);
    setEvent(session, "Range reset", tuning);
}

void setDevRangeDrillVariant(
    DevRangeSessionState& session,
    DevRangeDrillVariant variant,
    const DevRangeSessionTuning& tuning) {
    const auto nextVariant = safeVariant(variant);
    preserveCurrentVariantBest(session.drill);
    session.drill.variant = nextVariant;
    restartDrill(session, tuning);
    setEvent(session, "Drill " + std::string(devRangeDrillVariantName(nextVariant)), tuning);
}

void cycleDevRangeDrillVariant(
    DevRangeSessionState& session,
    const DevRangeSessionTuning& tuning) {
    const auto currentIndex = variantIndex(session.drill.variant);
    const auto nextIndex = (currentIndex + 1U) % kDevRangeDrillVariantCount;
    setDevRangeDrillVariant(session, static_cast<DevRangeDrillVariant>(nextIndex), tuning);
}

void ensureDevRangeLaneScore(
    DevRangeSessionState& session,
    std::size_t laneIndex,
    std::string_view laneId,
    std::string_view laneName) {
    if (auto* existing = findLaneScore(session, laneIndex); existing != nullptr) {
        existing->laneId = std::string(laneId);
        existing->laneName = std::string(laneName);
        return;
    }

    DevRangeLaneScore lane{};
    lane.laneIndex = laneIndex;
    lane.laneId = std::string(laneId);
    lane.laneName = std::string(laneName);
    session.laneScores.push_back(std::move(lane));
    std::sort(
        session.laneScores.begin(),
        session.laneScores.end(),
        [](const DevRangeLaneScore& lhs, const DevRangeLaneScore& rhs) {
            return lhs.laneIndex < rhs.laneIndex;
        });
}

void recordShotResult(
    DevRangeSessionState& session,
    const weapons::FireResult& fire,
    const DebugTargetHitResult& hit,
    const DevRangeSessionTuning& tuning) {
    recordShotResult(session, fire, hit, DevRangeShotScoreContext{}, tuning);
}

void recordShotResult(
    DevRangeSessionState& session,
    const weapons::FireResult& fire,
    const DebugTargetHitResult& hit,
    const DevRangeShotScoreContext& context,
    const DevRangeSessionTuning& tuning) {
    if (!fire.fired) {
        return;
    }

    const auto& rules = rulesForVariant(session.drill.variant);
    resetLatestScoreTelemetry(session.drill);
    ++session.score.shotsFired;
    ++session.drill.shotsFired;
    updateRecoilControl(session.drill, fire, hit.hit, rules);

    DevRangeLaneScore* lane = nullptr;
    if (context.laneKnown) {
        ensureDevRangeLaneScore(session, context.laneIndex, context.laneId, context.laneName);
        lane = findLaneScore(session, context.laneIndex);
        if (lane != nullptr) {
            ++lane->shotsFired;
            ++lane->currentTargetShots;
            lane->latestDistanceMeters = std::max(0.0F, context.distanceMeters);
        }
    }

    if (!hit.hit) {
        session.score.currentStreak = 0;
        ++session.drill.misses;
        updateScoreMultiplierTelemetry(session, rules);
        if (session.drill.status == DevRangeDrillStatus::Active) {
            addDrillPenalty(session.drill, rules.missPenaltyPoints);
        }
        std::ostringstream stream;
        stream << "Miss";
        appendScoreDelta(stream, session.drill);
        setEvent(session, stream.str(), tuning);
        return;
    }

    ++session.score.shotsHit;
    ++session.drill.shotsHit;
    session.score.damageDealt += hit.damageApplied;
    ++session.score.currentStreak;
    session.score.bestStreak = std::max(session.score.bestStreak, session.score.currentStreak);
    updateScoreMultiplierTelemetry(session, rules);

    if (lane != nullptr) {
        ++lane->shotsHit;
        ++lane->currentTargetHits;
        lane->damageDealt += hit.damageApplied;
        lane->currentTargetDamage += hit.damageApplied;
        lane->latestDistanceMeters = hit.distanceMeters > 0.0F ? hit.distanceMeters : lane->latestDistanceMeters;
        if (lane->currentTargetFirstHitSeconds < 0.0F) {
            lane->currentTargetFirstHitSeconds = session.drill.elapsedSeconds;
        }
    }

    if (session.drill.status == DevRangeDrillStatus::Active) {
        const float hitPoints =
            rules.hitBasePoints +
            (std::min(110.0F, std::max(0.0F, hit.damageApplied)) * rules.damagePointScale) +
            (static_cast<float>(session.score.currentStreak) * rules.streakPointScale) +
            (session.drill.recoilControlScore * rules.recoilPointScale);
        addPositiveDrillScore(session.drill, hitPoints, false);
    }

    if (hit.eliminated) {
        ++session.score.targetsEliminated;
        ++session.drill.targetsEliminated;
        if (lane != nullptr) {
            ++lane->targetsEliminated;
            recordLaneTtk(session, *lane, context);
            const bool perfectClear = lane->currentTargetShots > 0U &&
                lane->currentTargetShots == lane->currentTargetHits &&
                lane->currentTargetDamage >= std::max(1.0F, context.targetMaxHealth);
            if (perfectClear) {
                ++session.drill.perfectLaneClears;
                if (session.drill.status == DevRangeDrillStatus::Active) {
                    addPositiveDrillScore(session.drill, rules.perfectClearBonus, true);
                }
            }
            resetLaneTargetWindow(*lane);
        }
        if (session.drill.status == DevRangeDrillStatus::Active) {
            float ttkBonus = 0.0F;
            if (session.drill.latestTtkSeconds >= 0.0F) {
                ttkBonus = std::max(
                    0.0F,
                    (rules.ttkBonusWindowSeconds - session.drill.latestTtkSeconds) * rules.ttkBonusScale);
            }
            addPositiveDrillScore(session.drill, rules.eliminationBasePoints + ttkBonus, true);
        }
        std::ostringstream stream;
        stream << "Target eliminated  streak " << session.score.currentStreak;
        if (session.drill.latestTtkSeconds >= 0.0F) {
            stream << "  TTK " << session.drill.latestTtkSeconds << "s";
        }
        appendScoreDelta(stream, session.drill);
        setEvent(session, stream.str(), tuning);
    } else {
        std::ostringstream stream;
        stream << "Hit -" << static_cast<int>(hit.damageApplied);
        appendScoreDelta(stream, session.drill);
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

void tickDevRangeDrill(
    DevRangeSessionState& session,
    float deltaSeconds,
    const DevRangeSessionTuning& tuning) {
    if (session.drill.timeLimitSeconds <= 0.0F) {
        restartDrill(session, tuning);
    }
    if (session.drill.status != DevRangeDrillStatus::Active) {
        return;
    }

    const float consumed = std::max(0.0F, deltaSeconds);
    session.drill.elapsedSeconds += consumed;
    session.drill.timeRemainingSeconds = consume(session.drill.timeRemainingSeconds, consumed);
    if (session.drill.timeRemainingSeconds > 0.0001F) {
        return;
    }

    session.drill.timeRemainingSeconds = 0.0F;
    session.drill.status = DevRangeDrillStatus::Complete;
    preserveCurrentVariantBest(session.drill);
    std::ostringstream stream;
    stream << "Drill complete " << devRangeDrillVariantName(session.drill.variant) << " score " << session.drill.score;
    setEvent(session, stream.str(), tuning);
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

float devRangeDrillAccuracy(const DevRangeDrillState& drill) {
    if (drill.shotsFired == 0U) {
        return 0.0F;
    }
    return static_cast<float>(drill.shotsHit) / static_cast<float>(drill.shotsFired);
}

float devRangeLaneAccuracy(const DevRangeLaneScore& lane) {
    if (lane.shotsFired == 0U) {
        return 0.0F;
    }
    return static_cast<float>(lane.shotsHit) / static_cast<float>(lane.shotsFired);
}

float devRangeDrillProgress(const DevRangeDrillState& drill) {
    if (drill.timeLimitSeconds <= 0.0F) {
        return 0.0F;
    }
    return std::clamp(1.0F - (drill.timeRemainingSeconds / drill.timeLimitSeconds), 0.0F, 1.0F);
}

float devRangeDrillScoreMultiplier(const DevRangeDrillState& drill) {
    return drill.scoring.scoreMultiplier;
}

float devRangeRecoilControl01(const DevRangeDrillState& drill) {
    return std::clamp(drill.recoilControlScore / 100.0F, 0.0F, 1.0F);
}

std::string_view devRangeDrillStatusName(DevRangeDrillStatus status) {
    switch (status) {
    case DevRangeDrillStatus::Active:
        return "Active";
    case DevRangeDrillStatus::Complete:
        return "Complete";
    }
    return "Unknown";
}

std::string_view devRangeDrillVariantName(DevRangeDrillVariant variant) {
    return rulesForVariant(variant).name;
}

std::string_view devRangeDrillObjectiveLabel(DevRangeDrillVariant variant) {
    return rulesForVariant(variant).objectiveLabel;
}

const DevRangeDrillRules& devRangeDrillRules(DevRangeDrillVariant variant) {
    return rulesForVariant(variant);
}

const DevRangeLaneScore* activeDevRangeLaneScore(
    const DevRangeSessionState& session,
    std::size_t laneIndex) {
    return findLaneScore(session, laneIndex);
}

} // namespace nemisis::dev

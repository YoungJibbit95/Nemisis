#include "nemisis/weapons/WeaponSimulation.hpp"

#include <algorithm>

namespace nemisis::weapons {

namespace {

constexpr float kDryFireCooldownSeconds = 0.15F;
constexpr float kBurstResetSeconds = 0.28F;

[[nodiscard]] float consumeTimer(float value, float fixedDeltaSeconds) {
    return std::max(0.0F, value - std::max(0.0F, fixedDeltaSeconds));
}

[[nodiscard]] float secondsBetweenShots(const WeaponDefinition& definition) {
    if (definition.fireRateRpm == 0) {
        return 0.0F;
    }

    return 60.0F / static_cast<float>(definition.fireRateRpm);
}

[[nodiscard]] float approach(float current, float target, float maxDelta) {
    if (current < target) {
        return std::min(target, current + maxDelta);
    }
    return std::max(target, current - maxDelta);
}

[[nodiscard]] float clampAbs(float value, float limit) {
    return std::clamp(value, -std::max(0.0F, limit), std::max(0.0F, limit));
}

[[nodiscard]] float movementSpread(const WeaponDefinition& definition, const FireRequest& request) {
    const float movementScale = request.adsHeld
        ? definition.adsMovementSpreadPerMeter
        : definition.hipMovementSpreadPerMeter;
    float spread = std::max(0.0F, request.movementSpeed) * movementScale;
    if (request.airborne) {
        spread += definition.airborneSpreadPenaltyDegrees;
    }
    if (request.sprinting) {
        spread += definition.sprintSpreadPenaltyDegrees;
    }
    return spread;
}

void updateAds(const WeaponDefinition& definition, WeaponRuntimeState& state, const FireRequest& request) {
    const float target = request.adsHeld && !state.reloading ? 1.0F : 0.0F;
    const float adsTime = std::max(0.001F, definition.adsTimeSeconds);
    state.adsAlpha = approach(
        state.adsAlpha,
        target,
        std::max(0.0F, request.fixedDeltaSeconds) / adsTime);
}

void recoverRecoil(const WeaponDefinition& definition, WeaponRuntimeState& state, float fixedDeltaSeconds) {
    const float recovery = std::max(0.0F, definition.recoilRecoveryDegreesPerSecond) * std::max(0.0F, fixedDeltaSeconds);
    state.recoilPitchOffsetDegrees = approach(state.recoilPitchOffsetDegrees, 0.0F, recovery);
    state.recoilYawOffsetDegrees = approach(state.recoilYawOffsetDegrees, 0.0F, recovery * 0.7F);
}

[[nodiscard]] float deterministicYawSign(std::uint32_t shotIndex) {
    constexpr std::uint32_t kLeftRightPattern = 0b01011010U;
    return ((kLeftRightPattern >> (shotIndex % 8U)) & 1U) == 0U ? -1.0F : 1.0F;
}

void addShotRecoil(const WeaponDefinition& definition, WeaponRuntimeState& state, FireResult& result) {
    const float recoilScale = 1.0F - ((1.0F - definition.adsRecoilScale) * std::clamp(state.adsAlpha, 0.0F, 1.0F));
    const float burstFactor = 1.0F + (std::min<std::uint32_t>(state.burstShotCount, 8U) * 0.055F);
    const float yawSign = deterministicYawSign(state.shotIndex);
    const float yawNoise = (static_cast<float>((state.shotIndex * 17U) % 7U) - 3.0F) * definition.recoilNoiseDegrees;

    state.recoilPitchOffsetDegrees = std::clamp(
        state.recoilPitchOffsetDegrees + (definition.recoilPitchPerShotDegrees * burstFactor * recoilScale),
        0.0F,
        std::max(0.0F, definition.recoilMaxPitchDegrees));
    state.recoilYawOffsetDegrees = clampAbs(
        state.recoilYawOffsetDegrees +
            ((definition.recoilYawPerShotDegrees * yawSign * burstFactor) + yawNoise) * recoilScale,
        definition.recoilMaxYawDegrees);

    result.viewKickPitchDegrees = definition.viewKickDegrees * recoilScale;
}

void finishReload(const WeaponDefinition& definition, WeaponRuntimeState& state, FireResult& result) {
    state.ammoInMagazine = definition.magazineSize;
    state.reloadTimeRemaining = 0.0F;
    state.reloadProgress = 1.0F;
    state.reloading = false;
    result.completedReload = true;
}

void startReload(const WeaponDefinition& definition, WeaponRuntimeState& state, FireResult& result) {
    result.startedReload = true;

    if (definition.reloadTimeSeconds <= 0.0F) {
        finishReload(definition, state, result);
        return;
    }

    state.reloadTimeRemaining = definition.reloadTimeSeconds;
    state.reloadProgress = 0.0F;
    state.reloading = true;
}

} // namespace

FireResult simulateWeaponTick(
    const WeaponDefinition& definition,
    WeaponRuntimeState& state,
    FireRequest request) {
    FireResult result{};

    if (state.weaponId.empty()) {
        state.weaponId = definition.id;
    }

    state.ammoInMagazine = std::min(state.ammoInMagazine, definition.magazineSize);
    state.fireCooldownRemaining = consumeTimer(state.fireCooldownRemaining, request.fixedDeltaSeconds);
    state.timeSinceLastShotSeconds += std::max(0.0F, request.fixedDeltaSeconds);
    if (state.timeSinceLastShotSeconds >= kBurstResetSeconds && state.fireCooldownRemaining <= 0.0F) {
        state.burstShotCount = 0;
    }

    updateAds(definition, state, request);
    recoverRecoil(definition, state, request.fixedDeltaSeconds);

    if (state.reloading) {
        state.reloadTimeRemaining = consumeTimer(state.reloadTimeRemaining, request.fixedDeltaSeconds);
        if (definition.reloadTimeSeconds > 0.0F) {
            state.reloadProgress = std::clamp(
                1.0F - (state.reloadTimeRemaining / definition.reloadTimeSeconds),
                0.0F,
                1.0F);
        }
        if (state.reloadTimeRemaining <= 0.0F) {
            finishReload(definition, state, result);
        }
    }

    const bool canReload = !state.reloading && state.ammoInMagazine < definition.magazineSize;
    if (request.reloadPressed && canReload) {
        startReload(definition, state, result);
    }

    const bool canAttemptShot = request.triggerHeld && !state.reloading && definition.fireRateRpm > 0;
    if (canAttemptShot && state.fireCooldownRemaining <= 0.0F) {
        if (state.ammoInMagazine == 0) {
            result.dryFire = true;
            state.fireCooldownRemaining = kDryFireCooldownSeconds;
        } else {
            --state.ammoInMagazine;
            ++state.shotIndex;
            ++state.burstShotCount;
            state.timeSinceLastShotSeconds = 0.0F;
            result.fired = true;
            state.fireCooldownRemaining = secondsBetweenShots(definition);
            addShotRecoil(definition, state, result);
        }
    }

    result.ammoInMagazine = state.ammoInMagazine;
    result.shotIndex = state.shotIndex;
    result.burstShotCount = state.burstShotCount;
    result.fireCooldownRemaining = state.fireCooldownRemaining;
    result.adsAlpha = state.adsAlpha;
    result.recoilPitchOffsetDegrees = state.recoilPitchOffsetDegrees;
    result.recoilYawOffsetDegrees = state.recoilYawOffsetDegrees;
    result.reloadProgress = state.reloadProgress;
    result.movementSpreadDegrees = movementSpread(definition, request);
    return result;
}

} // namespace nemisis::weapons

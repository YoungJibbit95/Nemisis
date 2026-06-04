#include "nemisis/weapons/WeaponSimulation.hpp"

#include <algorithm>

namespace nemisis::weapons {

namespace {

constexpr float kDryFireCooldownSeconds = 0.15F;

[[nodiscard]] float consumeTimer(float value, float fixedDeltaSeconds) {
    return std::max(0.0F, value - std::max(0.0F, fixedDeltaSeconds));
}

[[nodiscard]] float secondsBetweenShots(const WeaponDefinition& definition) {
    if (definition.fireRateRpm == 0) {
        return 0.0F;
    }

    return 60.0F / static_cast<float>(definition.fireRateRpm);
}

void finishReload(const WeaponDefinition& definition, WeaponRuntimeState& state, FireResult& result) {
    state.ammoInMagazine = definition.magazineSize;
    state.reloadTimeRemaining = 0.0F;
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

    if (state.reloading) {
        state.reloadTimeRemaining = consumeTimer(state.reloadTimeRemaining, request.fixedDeltaSeconds);
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
            result.fired = true;
            state.fireCooldownRemaining = secondsBetweenShots(definition);
        }
    }

    result.ammoInMagazine = state.ammoInMagazine;
    result.shotIndex = state.shotIndex;
    result.fireCooldownRemaining = state.fireCooldownRemaining;
    return result;
}

} // namespace nemisis::weapons

#include "nemisis/weapons/WeaponSimulation.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

nemisis::weapons::WeaponDefinition testWeapon() {
    nemisis::weapons::WeaponDefinition definition{};
    definition.id = "test_ar";
    definition.displayName = "Test AR";
    definition.magazineSize = 3;
    definition.fireRateRpm = 600;
    definition.adsTimeSeconds = 0.15F;
    definition.reloadTimeSeconds = 0.25F;
    definition.damage = nemisis::weapons::DamageProfile{30.0F, 24.0F, 18.0F, 1.35F};
    return definition;
}

void testFireConsumesAmmoAndStartsCooldown() {
    const auto definition = testWeapon();
    nemisis::weapons::WeaponRuntimeState state{};
    state.weaponId = definition.id;
    state.ammoInMagazine = definition.magazineSize;

    nemisis::weapons::FireRequest request{};
    request.triggerHeld = true;
    request.fixedDeltaSeconds = 1.0F / 60.0F;

    const auto result = nemisis::weapons::simulateWeaponTick(definition, state, request);

    expect(result.fired, "trigger fires when ammo and cooldown are available");
    expect(state.ammoInMagazine == 2, "fire consumes one round");
    expect(state.shotIndex == 1, "fire increments shot index");
    expect(state.fireCooldownRemaining > 0.09F, "fire starts cooldown from rpm");
}

void testCooldownBlocksImmediateSecondShot() {
    const auto definition = testWeapon();
    nemisis::weapons::WeaponRuntimeState state{};
    state.weaponId = definition.id;
    state.ammoInMagazine = definition.magazineSize;

    nemisis::weapons::FireRequest request{};
    request.triggerHeld = true;
    request.fixedDeltaSeconds = 0.0F;
    (void)nemisis::weapons::simulateWeaponTick(definition, state, request);

    const auto blockedResult = nemisis::weapons::simulateWeaponTick(definition, state, request);

    expect(!blockedResult.fired, "cooldown blocks immediate second shot");
    expect(state.ammoInMagazine == 2, "blocked shot does not consume ammo");
    expect(state.shotIndex == 1, "blocked shot does not increment shot index");
}

void testReloadCompletesAfterReloadTime() {
    const auto definition = testWeapon();
    nemisis::weapons::WeaponRuntimeState state{};
    state.weaponId = definition.id;
    state.ammoInMagazine = 1;

    nemisis::weapons::FireRequest startReload{};
    startReload.reloadPressed = true;
    const auto startResult = nemisis::weapons::simulateWeaponTick(definition, state, startReload);

    expect(startResult.startedReload, "reload starts when magazine is not full");
    expect(state.reloading, "state enters reload");
    expect(state.ammoInMagazine == 1, "reload start does not instantly refill with timed reload");

    nemisis::weapons::FireRequest finishReload{};
    finishReload.fixedDeltaSeconds = definition.reloadTimeSeconds;
    const auto finishResult = nemisis::weapons::simulateWeaponTick(definition, state, finishReload);

    expect(finishResult.completedReload, "reload completes after reload time");
    expect(!state.reloading, "state exits reload");
    expect(state.ammoInMagazine == definition.magazineSize, "reload refills magazine");
}

void testDryFireWhenEmpty() {
    const auto definition = testWeapon();
    nemisis::weapons::WeaponRuntimeState state{};
    state.weaponId = definition.id;
    state.ammoInMagazine = 0;

    nemisis::weapons::FireRequest request{};
    request.triggerHeld = true;
    const auto result = nemisis::weapons::simulateWeaponTick(definition, state, request);

    expect(result.dryFire, "empty trigger emits dry fire");
    expect(!result.fired, "dry fire does not fire shot");
    expect(state.ammoInMagazine == 0, "dry fire keeps magazine empty");
}

} // namespace

int main() {
    testFireConsumesAmmoAndStartsCooldown();
    testCooldownBlocksImmediateSecondShot();
    testReloadCompletesAfterReloadTime();
    testDryFireWhenEmpty();

    if (failures > 0) {
        std::cerr << failures << " weapon simulation test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis weapon simulation tests passed\n";
    return EXIT_SUCCESS;
}

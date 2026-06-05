#pragma once

#include <cstdint>
#include <string>

namespace nemisis::weapons {

enum class WeaponClass {
    AssaultRifle,
    Smg,
    Shotgun,
    Sidearm
};

struct DamageProfile final {
    float closeDamage = 0.0F;
    float midDamage = 0.0F;
    float longDamage = 0.0F;
    float headMultiplier = 1.0F;
};

struct WeaponDefinition final {
    std::string id;
    std::string displayName;
    WeaponClass weaponClass = WeaponClass::AssaultRifle;
    std::uint16_t magazineSize = 0;
    std::uint16_t fireRateRpm = 0;
    float adsTimeSeconds = 0.0F;
    float reloadTimeSeconds = 0.0F;
    float maxRangeMeters = 80.0F;
    float hipSpreadDegrees = 1.25F;
    float adsSpreadDegrees = 0.25F;
    float recoilPitchPerShotDegrees = 0.08F;
    float recoilYawPerShotDegrees = 0.03F;
    DamageProfile damage{};
};

struct WeaponRuntimeState final {
    std::string weaponId;
    std::uint16_t ammoInMagazine = 0;
    std::uint32_t shotIndex = 0;
    float fireCooldownRemaining = 0.0F;
    float reloadTimeRemaining = 0.0F;
    bool reloading = false;
};

struct FireRequest final {
    bool triggerHeld = false;
    bool reloadPressed = false;
    float fixedDeltaSeconds = 1.0F / 60.0F;
};

struct FireResult final {
    bool fired = false;
    bool startedReload = false;
    bool completedReload = false;
    bool dryFire = false;
    std::uint16_t ammoInMagazine = 0;
    std::uint32_t shotIndex = 0;
    float fireCooldownRemaining = 0.0F;
};

} // namespace nemisis::weapons

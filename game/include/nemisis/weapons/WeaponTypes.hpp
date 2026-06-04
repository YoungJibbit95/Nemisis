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
    DamageProfile damage{};
};

} // namespace nemisis::weapons


#include "nemisis/weapons/WeaponSystem.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace nemisis::weapons {

namespace {

WeaponClass weaponClassFromString(const std::string& value) {
    if (value == "smg") {
        return WeaponClass::Smg;
    }
    if (value == "shotgun") {
        return WeaponClass::Shotgun;
    }
    if (value == "sidearm") {
        return WeaponClass::Sidearm;
    }
    return WeaponClass::AssaultRifle;
}

float numberOr(const novacore::core::ConfigDocument& document, const std::string& key, float fallback) {
    return static_cast<float>(document.numberOr(key, fallback));
}

std::uint16_t intOrU16(const novacore::core::ConfigDocument& document, const std::string& key, std::uint16_t fallback) {
    return static_cast<std::uint16_t>(document.intOr(key, fallback));
}

} // namespace

void WeaponSystem::clear() {
    weapons_.clear();
}

void WeaponSystem::registerWeapon(WeaponDefinition definition) {
    weapons_.insert_or_assign(definition.id, std::move(definition));
}

void WeaponSystem::registerPrototypeLoadout() {
    registerWeapon(WeaponDefinition{
        "ar_01",
        "Prototype AR",
        WeaponClass::AssaultRifle,
        30,
        720,
        0.18F,
        1.9F,
        90.0F,
        1.1F,
        0.2F,
        0.075F,
        0.025F,
        DamageProfile{28.0F, 24.0F, 19.0F, 1.35F},
        9.0F,
        4.2F,
        2.2F,
        0.030F,
        0.70F,
        0.034F,
        0.016F,
        0.42F,
        0.24F,
        0.88F,
        0.28F,
    });

    registerWeapon(WeaponDefinition{
        "smg_01",
        "Prototype SMG",
        WeaponClass::Smg,
        32,
        900,
        0.14F,
        1.65F,
        55.0F,
        1.45F,
        0.3F,
        0.06F,
        0.035F,
        DamageProfile{23.0F, 18.0F, 14.0F, 1.25F},
        10.5F,
        3.8F,
        2.8F,
        0.044F,
        0.74F,
        0.040F,
        0.020F,
        0.48F,
        0.30F,
        0.90F,
        0.24F,
    });

    registerWeapon(WeaponDefinition{
        "shotgun_01",
        "Prototype Shotgun",
        WeaponClass::Shotgun,
        6,
        80,
        0.2F,
        2.4F,
        35.0F,
        4.5F,
        2.4F,
        0.2F,
        0.08F,
        DamageProfile{90.0F, 45.0F, 20.0F, 1.1F},
        6.0F,
        5.8F,
        3.0F,
        0.070F,
        0.82F,
        0.055F,
        0.030F,
        0.70F,
        0.40F,
        0.92F,
        0.65F,
    });

    registerWeapon(WeaponDefinition{
        "sidearm_01",
        "Prototype Sidearm",
        WeaponClass::Sidearm,
        15,
        420,
        0.12F,
        1.35F,
        65.0F,
        1.0F,
        0.18F,
        0.04F,
        0.02F,
        DamageProfile{26.0F, 22.0F, 18.0F, 1.4F},
        11.5F,
        2.6F,
        1.4F,
        0.018F,
        0.68F,
        0.026F,
        0.012F,
        0.36F,
        0.16F,
        0.86F,
        0.20F,
    });
}

bool WeaponSystem::loadFromConfig(const novacore::core::ConfigDocument& document) {
    clear();

    for (int index = 0;; ++index) {
        const std::string prefix = "weapons." + std::to_string(index);
        const auto id = document.stringValue(prefix + ".id");
        if (!id.has_value()) {
            break;
        }

        WeaponDefinition definition{};
        definition.id = *id;
        definition.displayName = document.stringOr(prefix + ".display_name", definition.id);
        definition.weaponClass = weaponClassFromString(document.stringOr(prefix + ".class", "assault_rifle"));
        definition.magazineSize = intOrU16(document, prefix + ".magazine_size", 0);
        definition.fireRateRpm = intOrU16(document, prefix + ".fire_rate_rpm", 0);
        definition.adsTimeSeconds = numberOr(document, prefix + ".ads_time", 0.0F);
        definition.reloadTimeSeconds = numberOr(document, prefix + ".reload_time", 1.8F);
        definition.maxRangeMeters = numberOr(document, prefix + ".max_range", 80.0F);
        definition.hipSpreadDegrees = numberOr(document, prefix + ".spread.hip_degrees", 1.25F);
        definition.adsSpreadDegrees = numberOr(document, prefix + ".spread.ads_degrees", 0.25F);
        definition.recoilPitchPerShotDegrees = numberOr(document, prefix + ".recoil.pitch_per_shot_degrees", 0.08F);
        definition.recoilYawPerShotDegrees = numberOr(document, prefix + ".recoil.yaw_per_shot_degrees", 0.03F);
        definition.recoilRecoveryDegreesPerSecond = numberOr(document, prefix + ".recoil.recovery_degrees_per_second", definition.recoilRecoveryDegreesPerSecond);
        definition.recoilMaxPitchDegrees = numberOr(document, prefix + ".recoil.max_pitch_degrees", definition.recoilMaxPitchDegrees);
        definition.recoilMaxYawDegrees = numberOr(document, prefix + ".recoil.max_yaw_degrees", definition.recoilMaxYawDegrees);
        definition.recoilNoiseDegrees = numberOr(document, prefix + ".recoil.noise_degrees", definition.recoilNoiseDegrees);
        definition.adsRecoilScale = numberOr(document, prefix + ".recoil.ads_scale", definition.adsRecoilScale);
        definition.viewKickDegrees = numberOr(document, prefix + ".recoil.view_kick_degrees", definition.viewKickDegrees);
        definition.hipMovementSpreadPerMeter = numberOr(document, prefix + ".spread.hip_movement_per_meter", definition.hipMovementSpreadPerMeter);
        definition.adsMovementSpreadPerMeter = numberOr(document, prefix + ".spread.ads_movement_per_meter", definition.adsMovementSpreadPerMeter);
        definition.airborneSpreadPenaltyDegrees = numberOr(document, prefix + ".spread.airborne_penalty_degrees", definition.airborneSpreadPenaltyDegrees);
        definition.sprintSpreadPenaltyDegrees = numberOr(document, prefix + ".spread.sprint_penalty_degrees", definition.sprintSpreadPenaltyDegrees);
        definition.adsFovMultiplier = numberOr(document, prefix + ".ads_fov_multiplier", definition.adsFovMultiplier);

        definition.damage.closeDamage = numberOr(document, prefix + ".damage.close", 0.0F);
        definition.damage.midDamage = numberOr(document, prefix + ".damage.mid", definition.damage.closeDamage);
        definition.damage.longDamage = numberOr(document, prefix + ".damage.long", definition.damage.midDamage);
        definition.damage.headMultiplier = numberOr(document, prefix + ".damage.head_multiplier", 1.0F);

        const auto pellets = document.intValue(prefix + ".pellets");
        const auto damagePerPellet = document.numberValue(prefix + ".damage_per_pellet");
        if (pellets.has_value() && damagePerPellet.has_value()) {
            definition.damage.closeDamage = static_cast<float>(*pellets * *damagePerPellet);
            definition.damage.midDamage = definition.damage.closeDamage * 0.5F;
            definition.damage.longDamage = definition.damage.closeDamage * 0.25F;
        }

        registerWeapon(std::move(definition));
    }

    return weaponCount() > 0;
}

const WeaponDefinition* WeaponSystem::findWeapon(std::string_view id) const {
    auto it = weapons_.find(std::string(id));
    if (it == weapons_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::size_t WeaponSystem::weaponCount() const {
    return weapons_.size();
}

} // namespace nemisis::weapons

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
        DamageProfile{28.0F, 24.0F, 19.0F, 1.35F},
    });

    registerWeapon(WeaponDefinition{
        "smg_01",
        "Prototype SMG",
        WeaponClass::Smg,
        32,
        900,
        0.14F,
        DamageProfile{23.0F, 18.0F, 14.0F, 1.25F},
    });

    registerWeapon(WeaponDefinition{
        "shotgun_01",
        "Prototype Shotgun",
        WeaponClass::Shotgun,
        6,
        80,
        0.2F,
        DamageProfile{90.0F, 45.0F, 20.0F, 1.1F},
    });

    registerWeapon(WeaponDefinition{
        "sidearm_01",
        "Prototype Sidearm",
        WeaponClass::Sidearm,
        15,
        420,
        0.12F,
        DamageProfile{26.0F, 22.0F, 18.0F, 1.4F},
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

#include "nemisis/weapons/WeaponSystem.hpp"

#include <utility>

namespace nemisis::weapons {

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


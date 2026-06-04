#pragma once

#include "nemisis/weapons/WeaponTypes.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace nemisis::weapons {

class WeaponSystem final {
public:
    void registerWeapon(WeaponDefinition definition);
    void registerPrototypeLoadout();

    [[nodiscard]] const WeaponDefinition* findWeapon(std::string_view id) const;
    [[nodiscard]] std::size_t weaponCount() const;

private:
    std::unordered_map<std::string, WeaponDefinition> weapons_;
};

} // namespace nemisis::weapons


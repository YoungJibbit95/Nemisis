#pragma once

#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/core/ConfigDocument.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nemisis::weapons {

class WeaponSystem final {
public:
    void clear();
    void registerWeapon(WeaponDefinition definition);
    void registerPrototypeLoadout();
    bool loadFromConfig(const novacore::core::ConfigDocument& document);

    [[nodiscard]] const WeaponDefinition* findWeapon(std::string_view id) const;
    [[nodiscard]] std::size_t weaponCount() const;

private:
    std::unordered_map<std::string, WeaponDefinition> weapons_;
};

} // namespace nemisis::weapons

#pragma once

#include "nemisis/weapons/WeaponTypes.hpp"

namespace nemisis::weapons {

[[nodiscard]] FireResult simulateWeaponTick(
    const WeaponDefinition& definition,
    WeaponRuntimeState& state,
    FireRequest request);

} // namespace nemisis::weapons

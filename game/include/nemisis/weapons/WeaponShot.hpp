#pragma once

#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/math/Types.hpp"

#include <cstdint>

namespace nemisis::weapons {

struct ShotTraceRequest final {
    novacore::math::Vec3 origin{};
    novacore::math::Vec3 forward{0.0F, 0.0F, 1.0F};
    std::uint32_t seed = 0;
    std::uint32_t shotIndex = 0;
    float movementSpeed = 0.0F;
    float adsAlpha = 0.0F;
    float recoilPitchDegrees = 0.0F;
    float recoilYawDegrees = 0.0F;
    bool ads = false;
    bool airborne = false;
    bool sprinting = false;
};

struct ShotTraceResult final {
    novacore::math::Vec3 origin{};
    novacore::math::Vec3 direction{0.0F, 0.0F, 1.0F};
    float rangeMeters = 0.0F;
    float damage = 0.0F;
    float spreadDegrees = 0.0F;
    float recoilPitchDegrees = 0.0F;
    float recoilYawDegrees = 0.0F;
    std::uint32_t seed = 0;
    std::uint32_t shotIndex = 0;
};

[[nodiscard]] ShotTraceResult buildShotTrace(
    const WeaponDefinition& definition,
    const ShotTraceRequest& request);

} // namespace nemisis::weapons

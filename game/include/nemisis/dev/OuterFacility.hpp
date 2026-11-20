#pragma once

#include "nemisis/dev/DevTargetRange.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"

#include <array>
#include <string_view>

namespace nemisis::dev {

// Separate from the Dev Range: GameApp selects both world and target layout.
inline constexpr std::string_view kOuterFacilityWorldId = "outer_facility_01";
inline constexpr std::array<novacore::math::Vec3, 4> kFacilityGroveCenters{{
    {-28,0,-20}, {28,0,-20}, {-28,0,20}, {28,0,20},
}};
inline constexpr std::array<novacore::math::Vec3, 6> kFacilityTreeOffsets{{
    {-2.5F,0,-2}, {0,0,-2.5F}, {2.5F,0,-2},
    {-2.5F,0,2}, {0,0,2.5F}, {2.5F,0,2},
}};
[[nodiscard]] GreyboxWorld createOuterFacilityWorld();
[[nodiscard]] DevTargetRangeState makeOuterFacilityTargets();

} // namespace nemisis::dev

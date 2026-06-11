#pragma once

#include "novacore/math/Types.hpp"

#include <array>
#include <string>
#include <vector>

namespace nemisis::dev {

enum class GreyboxPrimitiveKind {
    Floor,
    Wall,
    Ramp,
    Cover,
    Spawn,
    RangeMarker,
    Target
};

enum class GreyboxRampDirection {
    None,
    PositiveZ,
    NegativeZ,
    PositiveX,
    NegativeX
};

struct GreyboxPrimitive final {
    std::string id;
    GreyboxPrimitiveKind kind = GreyboxPrimitiveKind::Floor;
    novacore::math::Vec3 center{};
    novacore::math::Vec3 halfExtents{1.0F, 1.0F, 1.0F};
    std::array<float, 4> color{0.30F, 0.36F, 0.38F, 1.0F};
    bool blocksMovement = false;
    GreyboxRampDirection rampDirection = GreyboxRampDirection::None;
    float stepOverrideHeight = 0.0F;
};

struct GreyboxWorld final {
    std::string id;
    novacore::math::Vec3 boundsHalfExtents{22.0F, 4.0F, 16.0F};
    novacore::math::Vec3 playerSpawn{0.0F, 0.0F, 0.0F};
    std::vector<GreyboxPrimitive> primitives;
};

[[nodiscard]] GreyboxWorld createDevRangeGreyboxWorld();
[[nodiscard]] const GreyboxPrimitive* findPrimitive(const GreyboxWorld& world, const std::string& id);
[[nodiscard]] std::size_t countPrimitivesByKind(const GreyboxWorld& world, GreyboxPrimitiveKind kind);

} // namespace nemisis::dev

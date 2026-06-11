#pragma once

#include "nemisis/dev/GreyboxWorld.hpp"

#include "novacore/math/Types.hpp"

#include <cstddef>
#include <string>

namespace nemisis::dev {

struct GreyboxCollisionQuery final {
    novacore::math::Vec3 position{};
    float radius = 0.42F;
    float height = 1.80F;
    float maxStepHeight = 0.42F;
    float snapDownDistance = 0.35F;
    float walkableSlopeCosine = 0.68F;
};

struct GreyboxCollisionResult final {
    novacore::math::Vec3 position{};
    novacore::math::Vec3 correction{};
    novacore::math::Vec3 groundNormal{0.0F, 1.0F, 0.0F};
    float groundHeight = 0.0F;
    std::size_t hitCount = 0;
    bool grounded = false;
    bool blocked = false;
    bool stepped = false;
    bool onRamp = false;
    std::string lastPrimitiveId;
    std::string groundPrimitiveId;
    GreyboxPrimitiveKind groundKind = GreyboxPrimitiveKind::Floor;
};

[[nodiscard]] GreyboxCollisionResult resolveGreyboxPlayerCollision(
    const GreyboxWorld& world,
    GreyboxCollisionQuery query);

} // namespace nemisis::dev

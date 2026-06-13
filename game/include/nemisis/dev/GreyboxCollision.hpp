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
    float wallProbeDistance = 0.48F;
    bool enableGroundSnap = true;
    bool enableStepUp = true;
    novacore::math::Vec3 mantleForward{};
    float mantleMaxDistance = 1.25F;
    float mantleMinHeight = 0.44F;
    float mantleMaxHeight = 1.45F;
};

struct GreyboxCollisionResult final {
    novacore::math::Vec3 position{};
    novacore::math::Vec3 correction{};
    novacore::math::Vec3 groundNormal{0.0F, 1.0F, 0.0F};
    novacore::math::Vec3 wallNormal{};
    novacore::math::Vec3 wallTangent{};
    float groundHeight = 0.0F;
    float wallDistance = 0.0F;
    float mantleDistance = 0.0F;
    float mantleHeight = 0.0F;
    std::size_t hitCount = 0;
    bool grounded = false;
    bool blocked = false;
    bool stepped = false;
    bool onRamp = false;
    bool nearWallRunSurface = false;
    bool nearSlideSurface = false;
    bool mantleCandidate = false;
    std::string lastPrimitiveId;
    std::string groundPrimitiveId;
    std::string wallPrimitiveId;
    std::string mantlePrimitiveId;
    GreyboxPrimitiveKind groundKind = GreyboxPrimitiveKind::Floor;
    GreyboxPrimitiveKind wallKind = GreyboxPrimitiveKind::Wall;
    GreyboxPrimitiveKind mantleKind = GreyboxPrimitiveKind::Ledge;
    novacore::math::Vec3 mantleObstaclePoint{};
    novacore::math::Vec3 mantleTargetPosition{};
    novacore::math::Vec3 mantleNormal{};
};

[[nodiscard]] GreyboxCollisionResult resolveGreyboxPlayerCollision(
    const GreyboxWorld& world,
    GreyboxCollisionQuery query);

} // namespace nemisis::dev

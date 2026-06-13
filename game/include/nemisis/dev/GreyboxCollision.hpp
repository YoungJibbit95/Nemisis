#pragma once

#include "nemisis/dev/GreyboxWorld.hpp"

#include "novacore/math/Types.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace nemisis::dev {

struct GreyboxCollisionQuery final {
    novacore::math::Vec3 position{};
    novacore::math::Vec3 previousPosition{};
    float radius = 0.42F;
    float height = 1.80F;
    float maxStepHeight = 0.42F;
    float snapDownDistance = 0.35F;
    float walkableSlopeCosine = 0.68F;
    float wallProbeDistance = 0.48F;
    int maxSweepIterations = 4;
    bool useSweep = false;
    bool enableGroundSnap = true;
    bool enableStepUp = true;
    novacore::math::Vec3 mantleForward{};
    float mantleMaxDistance = 1.25F;
    float mantleMinHeight = 0.44F;
    float mantleMaxHeight = 1.45F;
};

enum class GreyboxContactRole {
    Ground,
    Step,
    Wall,
    Bounds,
    Sweep,
};

struct GreyboxContact final {
    std::string primitiveId;
    GreyboxPrimitiveKind kind = GreyboxPrimitiveKind::Wall;
    GreyboxContactRole role = GreyboxContactRole::Wall;
    novacore::math::Vec3 point{};
    novacore::math::Vec3 normal{};
    float distance = 0.0F;
    float fraction = 1.0F;
    float penetrationDepth = 0.0F;
    bool blocking = false;
    bool walkable = false;
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
    float sweepFraction = 1.0F;
    std::size_t hitCount = 0;
    std::size_t sweepIterations = 0;
    bool grounded = false;
    bool blocked = false;
    bool stepped = false;
    bool onRamp = false;
    bool nearWallRunSurface = false;
    bool nearSlideSurface = false;
    bool mantleCandidate = false;
    bool swept = false;
    bool sweepHit = false;
    std::string lastPrimitiveId;
    std::string groundPrimitiveId;
    std::string wallPrimitiveId;
    std::string mantlePrimitiveId;
    std::string sweepPrimitiveId;
    GreyboxPrimitiveKind groundKind = GreyboxPrimitiveKind::Floor;
    GreyboxPrimitiveKind wallKind = GreyboxPrimitiveKind::Wall;
    GreyboxPrimitiveKind mantleKind = GreyboxPrimitiveKind::Ledge;
    GreyboxPrimitiveKind sweepKind = GreyboxPrimitiveKind::Wall;
    novacore::math::Vec3 sweepStartPosition{};
    novacore::math::Vec3 requestedDisplacement{};
    novacore::math::Vec3 appliedDisplacement{};
    novacore::math::Vec3 remainingDisplacement{};
    novacore::math::Vec3 sweepNormal{};
    novacore::math::Vec3 mantleObstaclePoint{};
    novacore::math::Vec3 mantleTargetPosition{};
    novacore::math::Vec3 mantleNormal{};
    std::vector<GreyboxContact> contacts;
    std::vector<GreyboxContact> sweepContacts;
};

[[nodiscard]] GreyboxCollisionResult resolveGreyboxPlayerCollision(
    const GreyboxWorld& world,
    GreyboxCollisionQuery query);
[[nodiscard]] const char* greyboxContactRoleName(GreyboxContactRole role);

} // namespace nemisis::dev

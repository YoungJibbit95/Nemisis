#include "nemisis/dev/GreyboxCollision.hpp"

#include "novacore/physics/CharacterController.hpp"

#include <algorithm>

namespace nemisis::dev {

namespace {

[[nodiscard]] novacore::physics::RampDirection toPhysicsRampDirection(GreyboxRampDirection direction) {
    switch (direction) {
    case GreyboxRampDirection::PositiveZ:
        return novacore::physics::RampDirection::PositiveZ;
    case GreyboxRampDirection::NegativeZ:
        return novacore::physics::RampDirection::NegativeZ;
    case GreyboxRampDirection::PositiveX:
        return novacore::physics::RampDirection::PositiveX;
    case GreyboxRampDirection::NegativeX:
        return novacore::physics::RampDirection::NegativeX;
    case GreyboxRampDirection::None:
        break;
    }
    return novacore::physics::RampDirection::None;
}

[[nodiscard]] novacore::physics::SurfaceKind toPhysicsSurfaceKind(const GreyboxPrimitive& primitive) {
    switch (primitive.kind) {
    case GreyboxPrimitiveKind::Floor:
        return novacore::physics::SurfaceKind::Floor;
    case GreyboxPrimitiveKind::Wall:
        return novacore::physics::SurfaceKind::Wall;
    case GreyboxPrimitiveKind::Ramp:
        return primitive.id.find("slide") != std::string::npos
            ? novacore::physics::SurfaceKind::Slide
            : novacore::physics::SurfaceKind::Ramp;
    case GreyboxPrimitiveKind::Cover:
        return novacore::physics::SurfaceKind::Cover;
    case GreyboxPrimitiveKind::Ledge:
        return novacore::physics::SurfaceKind::Ledge;
    case GreyboxPrimitiveKind::WallRunPanel:
        return novacore::physics::SurfaceKind::WallRun;
    case GreyboxPrimitiveKind::Spawn:
    case GreyboxPrimitiveKind::RangeMarker:
    case GreyboxPrimitiveKind::Target:
        return novacore::physics::SurfaceKind::Trigger;
    }
    return novacore::physics::SurfaceKind::Wall;
}

[[nodiscard]] GreyboxPrimitiveKind fallbackKindForPhysics(novacore::physics::SurfaceKind kind) {
    switch (kind) {
    case novacore::physics::SurfaceKind::Floor:
        return GreyboxPrimitiveKind::Floor;
    case novacore::physics::SurfaceKind::Wall:
        return GreyboxPrimitiveKind::Wall;
    case novacore::physics::SurfaceKind::Ramp:
    case novacore::physics::SurfaceKind::Slide:
        return GreyboxPrimitiveKind::Ramp;
    case novacore::physics::SurfaceKind::Cover:
        return GreyboxPrimitiveKind::Cover;
    case novacore::physics::SurfaceKind::WallRun:
        return GreyboxPrimitiveKind::WallRunPanel;
    case novacore::physics::SurfaceKind::Ledge:
        return GreyboxPrimitiveKind::Ledge;
    case novacore::physics::SurfaceKind::Trigger:
        return GreyboxPrimitiveKind::RangeMarker;
    }
    return GreyboxPrimitiveKind::Wall;
}

[[nodiscard]] novacore::physics::PhysicsWorld buildPhysicsWorld(const GreyboxWorld& world) {
    novacore::physics::PhysicsWorld physicsWorld;
    physicsWorld.setBounds(world.boundsHalfExtents);

    for (const auto& primitive : world.primitives) {
        if (!primitive.blocksMovement) {
            continue;
        }

        physicsWorld.addStaticCollider(novacore::physics::StaticCollider{
            primitive.id,
            toPhysicsSurfaceKind(primitive),
            primitive.center,
            primitive.halfExtents,
            primitive.blocksMovement,
            toPhysicsRampDirection(primitive.rampDirection),
            primitive.stepOverrideHeight,
        });
    }

    return physicsWorld;
}

[[nodiscard]] GreyboxPrimitiveKind kindForPrimitiveId(
    const GreyboxWorld& world,
    const std::string& id,
    novacore::physics::SurfaceKind fallback) {
    if (const auto* primitive = findPrimitive(world, id)) {
        return primitive->kind;
    }
    return fallbackKindForPhysics(fallback);
}

} // namespace

GreyboxCollisionResult resolveGreyboxPlayerCollision(
    const GreyboxWorld& world,
    GreyboxCollisionQuery query) {
    const auto physicsWorld = buildPhysicsWorld(world);
    novacore::physics::CharacterResolveResult resolved{};
    novacore::physics::CharacterSweepResult swept{};
    if (query.useSweep) {
        novacore::physics::CharacterSweepQuery sweepQuery{};
        sweepQuery.startPosition = query.previousPosition;
        sweepQuery.desiredDisplacement = query.position - query.previousPosition;
        sweepQuery.radius = query.radius;
        sweepQuery.height = query.height;
        sweepQuery.maxStepHeight = query.maxStepHeight;
        sweepQuery.snapDownDistance = query.snapDownDistance;
        sweepQuery.walkableSlopeCosine = query.walkableSlopeCosine;
        sweepQuery.wallProbeDistance = query.wallProbeDistance;
        sweepQuery.maxIterations = query.maxSweepIterations;
        sweepQuery.enableGroundSnap = query.enableGroundSnap;
        sweepQuery.enableStepUp = query.enableStepUp;
        swept = physicsWorld.sweepCharacter(sweepQuery);
        resolved = swept.resolve;
    } else {
        novacore::physics::CharacterQuery physicsQuery{};
        physicsQuery.position = query.position;
        physicsQuery.radius = query.radius;
        physicsQuery.height = query.height;
        physicsQuery.maxStepHeight = query.maxStepHeight;
        physicsQuery.snapDownDistance = query.snapDownDistance;
        physicsQuery.walkableSlopeCosine = query.walkableSlopeCosine;
        physicsQuery.wallProbeDistance = query.wallProbeDistance;
        physicsQuery.enableGroundSnap = query.enableGroundSnap;
        physicsQuery.enableStepUp = query.enableStepUp;
        resolved = physicsWorld.resolveCharacter(physicsQuery);
    }

    GreyboxCollisionResult result{};
    result.position = resolved.position;
    result.correction = resolved.correction;
    result.groundNormal = resolved.groundNormal;
    result.wallNormal = resolved.wallNormal;
    result.wallTangent = resolved.wallTangent;
    result.groundHeight = resolved.groundHeight;
    result.wallDistance = resolved.wallDistance;
    result.hitCount = resolved.hitCount;
    result.sweepFraction = swept.firstHitFraction;
    result.sweepIterations = swept.iterationCount;
    result.grounded = resolved.grounded;
    result.blocked = resolved.blocked;
    result.stepped = resolved.stepped;
    result.onRamp = resolved.onRamp;
    result.nearWallRunSurface = resolved.nearWallRunSurface;
    result.nearSlideSurface = resolved.nearSlideSurface;
    result.lastPrimitiveId = resolved.lastColliderId;
    result.groundPrimitiveId = resolved.groundColliderId;
    result.wallPrimitiveId = resolved.wallColliderId;
    result.swept = query.useSweep;
    result.sweepHit = swept.hit;
    result.sweepPrimitiveId = swept.hitColliderId;
    result.groundKind = kindForPrimitiveId(world, resolved.groundColliderId, resolved.groundKind);
    result.wallKind = kindForPrimitiveId(world, resolved.wallColliderId, resolved.wallKind);
    result.sweepKind = kindForPrimitiveId(world, swept.hitColliderId, swept.hitKind);
    result.sweepStartPosition = swept.startPosition;
    result.requestedDisplacement = query.useSweep ? swept.desiredDisplacement : novacore::math::Vec3{};
    result.appliedDisplacement = query.useSweep ? swept.appliedDisplacement : novacore::math::Vec3{};
    result.remainingDisplacement = query.useSweep ? swept.remainingDisplacement : novacore::math::Vec3{};
    result.sweepNormal = swept.hitNormal;

    const auto mantle = physicsWorld.probeMantle(novacore::physics::MantleProbe{
        result.position,
        query.mantleForward,
        query.radius,
        query.height,
        query.mantleMaxDistance,
        query.mantleMinHeight,
        query.mantleMaxHeight,
        0.18F,
    });
    result.mantleCandidate = mantle.hit;
    result.mantleObstaclePoint = mantle.obstaclePoint;
    result.mantleTargetPosition = mantle.targetPosition;
    result.mantleNormal = mantle.normal;
    result.mantleDistance = mantle.distance;
    result.mantleHeight = mantle.height;
    result.mantlePrimitiveId = mantle.colliderId;
    result.mantleKind = kindForPrimitiveId(world, mantle.colliderId, mantle.kind);
    return result;
}

} // namespace nemisis::dev

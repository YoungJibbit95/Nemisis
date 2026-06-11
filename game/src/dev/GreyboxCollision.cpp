#include "nemisis/dev/GreyboxCollision.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace nemisis::dev {

namespace {

[[nodiscard]] float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] float primitiveMinX(const GreyboxPrimitive& primitive) {
    return primitive.center.x - primitive.halfExtents.x;
}

[[nodiscard]] float primitiveMaxX(const GreyboxPrimitive& primitive) {
    return primitive.center.x + primitive.halfExtents.x;
}

[[nodiscard]] float primitiveMinY(const GreyboxPrimitive& primitive) {
    return primitive.center.y - primitive.halfExtents.y;
}

[[nodiscard]] float primitiveMaxY(const GreyboxPrimitive& primitive) {
    return primitive.center.y + primitive.halfExtents.y;
}

[[nodiscard]] float primitiveMinZ(const GreyboxPrimitive& primitive) {
    return primitive.center.z - primitive.halfExtents.z;
}

[[nodiscard]] float primitiveMaxZ(const GreyboxPrimitive& primitive) {
    return primitive.center.z + primitive.halfExtents.z;
}

[[nodiscard]] bool verticalRangesOverlap(float playerFeetY, float playerHeight, const GreyboxPrimitive& primitive) {
    const float playerMinY = playerFeetY;
    const float playerMaxY = playerFeetY + playerHeight;
    return playerMaxY >= primitiveMinY(primitive) && playerMinY <= primitiveMaxY(primitive);
}

[[nodiscard]] bool horizontalPointInsideExpanded(
    novacore::math::Vec3 position,
    const GreyboxPrimitive& primitive,
    float expansion) {
    return position.x >= primitiveMinX(primitive) - expansion &&
        position.x <= primitiveMaxX(primitive) + expansion &&
        position.z >= primitiveMinZ(primitive) - expansion &&
        position.z <= primitiveMaxZ(primitive) + expansion;
}

[[nodiscard]] novacore::math::Vec3 normalizeOrUp(novacore::math::Vec3 value) {
    const float lengthSquared = value.lengthSquared();
    if (lengthSquared <= 0.000001F) {
        return {0.0F, 1.0F, 0.0F};
    }

    const float invLength = 1.0F / std::sqrt(lengthSquared);
    return {value.x * invLength, value.y * invLength, value.z * invLength};
}

[[nodiscard]] float rampT(const GreyboxPrimitive& primitive, novacore::math::Vec3 position) {
    switch (primitive.rampDirection) {
    case GreyboxRampDirection::PositiveZ:
        return clamp01((position.z - primitiveMinZ(primitive)) / std::max(0.001F, primitive.halfExtents.z * 2.0F));
    case GreyboxRampDirection::NegativeZ:
        return 1.0F - clamp01((position.z - primitiveMinZ(primitive)) / std::max(0.001F, primitive.halfExtents.z * 2.0F));
    case GreyboxRampDirection::PositiveX:
        return clamp01((position.x - primitiveMinX(primitive)) / std::max(0.001F, primitive.halfExtents.x * 2.0F));
    case GreyboxRampDirection::NegativeX:
        return 1.0F - clamp01((position.x - primitiveMinX(primitive)) / std::max(0.001F, primitive.halfExtents.x * 2.0F));
    case GreyboxRampDirection::None:
        break;
    }
    return 0.0F;
}

[[nodiscard]] float surfaceHeightAt(const GreyboxPrimitive& primitive, novacore::math::Vec3 position) {
    if (primitive.kind != GreyboxPrimitiveKind::Ramp || primitive.rampDirection == GreyboxRampDirection::None) {
        return primitiveMaxY(primitive);
    }

    const float low = primitiveMinY(primitive);
    const float high = primitiveMaxY(primitive);
    return low + (high - low) * rampT(primitive, position);
}

[[nodiscard]] novacore::math::Vec3 rampNormal(const GreyboxPrimitive& primitive) {
    if (primitive.rampDirection == GreyboxRampDirection::None) {
        return {0.0F, 1.0F, 0.0F};
    }

    const float rise = primitive.halfExtents.y * 2.0F;
    switch (primitive.rampDirection) {
    case GreyboxRampDirection::PositiveZ: {
        const float run = std::max(0.001F, primitive.halfExtents.z * 2.0F);
        return normalizeOrUp({0.0F, 1.0F, -(rise / run)});
    }
    case GreyboxRampDirection::NegativeZ: {
        const float run = std::max(0.001F, primitive.halfExtents.z * 2.0F);
        return normalizeOrUp({0.0F, 1.0F, rise / run});
    }
    case GreyboxRampDirection::PositiveX: {
        const float run = std::max(0.001F, primitive.halfExtents.x * 2.0F);
        return normalizeOrUp({-(rise / run), 1.0F, 0.0F});
    }
    case GreyboxRampDirection::NegativeX: {
        const float run = std::max(0.001F, primitive.halfExtents.x * 2.0F);
        return normalizeOrUp({rise / run, 1.0F, 0.0F});
    }
    case GreyboxRampDirection::None:
        break;
    }
    return {0.0F, 1.0F, 0.0F};
}

void recordCorrection(
    GreyboxCollisionResult& result,
    novacore::math::Vec3 before,
    std::string primitiveId) {
    const auto delta = result.position - before;
    if (delta.lengthSquared() <= 0.000001F) {
        return;
    }

    result.correction = result.correction + delta;
    result.blocked = true;
    ++result.hitCount;
    result.lastPrimitiveId = std::move(primitiveId);
}

void recordGround(
    GreyboxCollisionResult& result,
    const GreyboxPrimitive& primitive,
    float groundHeight,
    novacore::math::Vec3 normal,
    bool stepped) {
    const auto before = result.position;
    result.position.y = groundHeight;
    result.groundHeight = groundHeight;
    result.groundNormal = normalizeOrUp(normal);
    result.grounded = true;
    result.stepped = result.stepped || stepped;
    result.onRamp = result.onRamp || primitive.kind == GreyboxPrimitiveKind::Ramp;
    result.groundPrimitiveId = primitive.id;
    result.groundKind = primitive.kind;

    const auto delta = result.position - before;
    if (delta.lengthSquared() > 0.000001F) {
        result.correction = result.correction + delta;
        ++result.hitCount;
        result.lastPrimitiveId = primitive.id;
    }
}

[[nodiscard]] bool canSnapToSurface(
    const GreyboxCollisionResult& result,
    const GreyboxPrimitive& primitive,
    const GreyboxCollisionQuery& query,
    float groundHeight,
    novacore::math::Vec3 normal) {
    if (!horizontalPointInsideExpanded(result.position, primitive, query.radius)) {
        return false;
    }

    const float verticalDelta = groundHeight - result.position.y;
    if (verticalDelta > std::max(query.maxStepHeight, primitive.stepOverrideHeight)) {
        return false;
    }
    if (verticalDelta < -query.snapDownDistance) {
        return false;
    }
    return normalizeOrUp(normal).y >= query.walkableSlopeCosine;
}

void resolveGroundSurfaces(
    GreyboxCollisionResult& result,
    const GreyboxWorld& world,
    const GreyboxCollisionQuery& query) {
    const GreyboxPrimitive floorPrimitive{
        "floor_main",
        GreyboxPrimitiveKind::Floor,
        {0.0F, -0.05F, 0.0F},
        {world.boundsHalfExtents.x, 0.05F, world.boundsHalfExtents.z},
        {0.16F, 0.19F, 0.20F, 1.0F},
        true,
        GreyboxRampDirection::None,
        0.0F,
    };

    float bestHeight = 0.0F;
    novacore::math::Vec3 bestNormal{0.0F, 1.0F, 0.0F};
    const GreyboxPrimitive* bestPrimitive = &floorPrimitive;
    bool bestStepped = false;

    for (const auto& primitive : world.primitives) {
        if (!primitive.blocksMovement) {
            continue;
        }
        if (primitive.kind != GreyboxPrimitiveKind::Ramp && primitive.kind != GreyboxPrimitiveKind::Cover) {
            continue;
        }

        const auto normal = primitive.kind == GreyboxPrimitiveKind::Ramp
            ? rampNormal(primitive)
            : novacore::math::Vec3{0.0F, 1.0F, 0.0F};
        const float height = surfaceHeightAt(primitive, result.position);
        if (!canSnapToSurface(result, primitive, query, height, normal)) {
            continue;
        }

        if (bestPrimitive == &floorPrimitive || height > bestHeight) {
            bestHeight = height;
            bestNormal = normal;
            bestPrimitive = &primitive;
            bestStepped = primitive.kind == GreyboxPrimitiveKind::Cover && height > 0.001F;
        }
    }

    if (bestPrimitive != nullptr) {
        recordGround(result, *bestPrimitive, bestHeight, bestNormal, bestStepped);
    }
}

void resolveAgainstExpandedAabb(
    GreyboxCollisionResult& result,
    const GreyboxPrimitive& primitive,
    const GreyboxCollisionQuery& query) {
    if (!primitive.blocksMovement ||
        primitive.kind == GreyboxPrimitiveKind::Floor ||
        primitive.kind == GreyboxPrimitiveKind::Ramp) {
        return;
    }
    if (!verticalRangesOverlap(result.position.y, query.height, primitive)) {
        return;
    }

    const float topY = primitiveMaxY(primitive);
    if (topY <= result.position.y + std::max(query.maxStepHeight, primitive.stepOverrideHeight) &&
        topY >= result.position.y - query.snapDownDistance &&
        horizontalPointInsideExpanded(result.position, primitive, query.radius)) {
        recordGround(result, primitive, topY, {0.0F, 1.0F, 0.0F}, true);
        return;
    }

    const float minX = primitiveMinX(primitive) - query.radius;
    const float maxX = primitiveMaxX(primitive) + query.radius;
    const float minZ = primitiveMinZ(primitive) - query.radius;
    const float maxZ = primitiveMaxZ(primitive) + query.radius;

    if (result.position.x < minX || result.position.x > maxX ||
        result.position.z < minZ || result.position.z > maxZ) {
        return;
    }

    const auto before = result.position;
    const std::array<float, 4> pushes{
        minX - result.position.x,
        maxX - result.position.x,
        minZ - result.position.z,
        maxZ - result.position.z,
    };

    std::size_t best = 0;
    float bestMagnitude = std::abs(pushes[0]);
    for (std::size_t index = 1; index < pushes.size(); ++index) {
        const float magnitude = std::abs(pushes[index]);
        if (magnitude < bestMagnitude) {
            best = index;
            bestMagnitude = magnitude;
        }
    }

    if (best < 2) {
        result.position.x += pushes[best];
    } else {
        result.position.z += pushes[best];
    }
    recordCorrection(result, before, primitive.id);
}

void resolveBounds(
    GreyboxCollisionResult& result,
    const GreyboxWorld& world,
    float radius) {
    const auto before = result.position;
    result.position.x = std::clamp(
        result.position.x,
        -world.boundsHalfExtents.x + radius,
        world.boundsHalfExtents.x - radius);
    result.position.z = std::clamp(
        result.position.z,
        -world.boundsHalfExtents.z + radius,
        world.boundsHalfExtents.z - radius);
    recordCorrection(result, before, "world_bounds");
}

void resolveFloor(GreyboxCollisionResult& result) {
    if (result.position.y > 0.0F) {
        return;
    }

    const auto before = result.position;
    result.position.y = 0.0F;
    result.groundHeight = 0.0F;
    result.groundNormal = {0.0F, 1.0F, 0.0F};
    result.grounded = true;
    result.groundPrimitiveId = "floor_main";
    result.groundKind = GreyboxPrimitiveKind::Floor;
    recordCorrection(result, before, "floor_main");
}

} // namespace

GreyboxCollisionResult resolveGreyboxPlayerCollision(
    const GreyboxWorld& world,
    GreyboxCollisionQuery query) {
    query.radius = std::max(0.01F, query.radius);
    query.height = std::max(0.01F, query.height);
    query.maxStepHeight = std::max(0.0F, query.maxStepHeight);
    query.snapDownDistance = std::max(0.0F, query.snapDownDistance);
    query.walkableSlopeCosine = std::clamp(query.walkableSlopeCosine, 0.0F, 1.0F);

    GreyboxCollisionResult result{};
    result.position = query.position;

    resolveFloor(result);
    resolveGroundSurfaces(result, world, query);
    resolveBounds(result, world, query.radius);

    for (const auto& primitive : world.primitives) {
        resolveAgainstExpandedAabb(result, primitive, query);
    }

    return result;
}

} // namespace nemisis::dev

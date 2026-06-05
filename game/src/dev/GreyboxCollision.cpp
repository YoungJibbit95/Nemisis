#include "nemisis/dev/GreyboxCollision.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace nemisis::dev {

namespace {

[[nodiscard]] bool verticalRangesOverlap(float playerFeetY, float playerHeight, const GreyboxPrimitive& primitive) {
    const float playerMinY = playerFeetY;
    const float playerMaxY = playerFeetY + playerHeight;
    const float primitiveMinY = primitive.center.y - primitive.halfExtents.y;
    const float primitiveMaxY = primitive.center.y + primitive.halfExtents.y;
    return playerMaxY >= primitiveMinY && playerMinY <= primitiveMaxY;
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

void resolveAgainstExpandedAabb(
    GreyboxCollisionResult& result,
    const GreyboxPrimitive& primitive,
    const GreyboxCollisionQuery& query) {
    if (!primitive.blocksMovement || primitive.kind == GreyboxPrimitiveKind::Floor) {
        return;
    }
    if (!verticalRangesOverlap(result.position.y, query.height, primitive)) {
        return;
    }

    const float minX = primitive.center.x - primitive.halfExtents.x - query.radius;
    const float maxX = primitive.center.x + primitive.halfExtents.x + query.radius;
    const float minZ = primitive.center.z - primitive.halfExtents.z - query.radius;
    const float maxZ = primitive.center.z + primitive.halfExtents.z + query.radius;

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
    result.grounded = true;
    recordCorrection(result, before, "floor_main");
}

} // namespace

GreyboxCollisionResult resolveGreyboxPlayerCollision(
    const GreyboxWorld& world,
    GreyboxCollisionQuery query) {
    query.radius = std::max(0.01F, query.radius);
    query.height = std::max(0.01F, query.height);

    GreyboxCollisionResult result{};
    result.position = query.position;

    resolveFloor(result);
    resolveBounds(result, world, query.radius);

    for (const auto& primitive : world.primitives) {
        resolveAgainstExpandedAabb(result, primitive, query);
    }

    return result;
}

} // namespace nemisis::dev

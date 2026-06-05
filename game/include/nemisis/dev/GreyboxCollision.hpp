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
};

struct GreyboxCollisionResult final {
    novacore::math::Vec3 position{};
    novacore::math::Vec3 correction{};
    std::size_t hitCount = 0;
    bool grounded = false;
    bool blocked = false;
    std::string lastPrimitiveId;
};

[[nodiscard]] GreyboxCollisionResult resolveGreyboxPlayerCollision(
    const GreyboxWorld& world,
    GreyboxCollisionQuery query);

} // namespace nemisis::dev

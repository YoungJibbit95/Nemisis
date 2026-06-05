#include "nemisis/weapons/WeaponShot.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace nemisis::weapons {

namespace {

constexpr float kPi = 3.14159265358979323846F;

[[nodiscard]] std::uint32_t hashString(const std::string& value) {
    std::uint32_t hash = 2166136261U;
    for (const char character : value) {
        hash ^= static_cast<std::uint8_t>(character);
        hash *= 16777619U;
    }
    return hash;
}

[[nodiscard]] std::uint32_t nextRandom(std::uint32_t& state) {
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return state;
}

[[nodiscard]] float signedUnitRandom(std::uint32_t& state) {
    constexpr float scale = 1.0F / static_cast<float>(0x7fffffffU);
    const auto value = nextRandom(state) & 0x7fffffffU;
    return (static_cast<float>(value) * scale * 2.0F) - 1.0F;
}

[[nodiscard]] float degreesToRadians(float degrees) {
    return degrees * (kPi / 180.0F);
}

[[nodiscard]] novacore::math::Vec3 normalized(novacore::math::Vec3 value) {
    const float lengthSquared = value.lengthSquared();
    if (lengthSquared <= 0.000001F) {
        return novacore::math::Vec3{0.0F, 0.0F, 1.0F};
    }

    const float inverseLength = 1.0F / std::sqrt(lengthSquared);
    return value * inverseLength;
}

} // namespace

ShotTraceResult buildShotTrace(
    const WeaponDefinition& definition,
    const ShotTraceRequest& request) {
    const float baseSpreadDegrees = request.ads ? definition.adsSpreadDegrees : definition.hipSpreadDegrees;
    const float movementSpreadDegrees = std::clamp(request.movementSpeed * 0.03F, 0.0F, 1.5F);
    const float spreadDegrees = std::max(0.0F, baseSpreadDegrees + movementSpreadDegrees);

    std::uint32_t randomState =
        request.seed ^
        (request.shotIndex * 0x9E3779B9U) ^
        hashString(definition.id);

    const float spreadYaw = signedUnitRandom(randomState) * spreadDegrees;
    const float spreadPitch = signedUnitRandom(randomState) * spreadDegrees;
    const float recoilYaw = definition.recoilYawPerShotDegrees * static_cast<float>(request.shotIndex);
    const float recoilPitch = definition.recoilPitchPerShotDegrees * static_cast<float>(request.shotIndex);

    const float yawRadians = degreesToRadians(spreadYaw + recoilYaw);
    const float pitchRadians = degreesToRadians(spreadPitch + recoilPitch);

    const auto forward = normalized(request.forward);
    auto direction = novacore::math::Vec3{
        forward.x + yawRadians,
        forward.y + pitchRadians,
        forward.z,
    };
    direction = normalized(direction);

    ShotTraceResult result{};
    result.origin = request.origin;
    result.direction = direction;
    result.rangeMeters = definition.maxRangeMeters;
    result.damage = definition.damage.closeDamage;
    result.spreadDegrees = spreadDegrees;
    result.seed = request.seed;
    result.shotIndex = request.shotIndex;
    return result;
}

} // namespace nemisis::weapons

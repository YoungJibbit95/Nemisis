#include "nemisis/render/RenderTuning.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace nemisis::render {

namespace {

float numberOr(const novacore::core::ConfigDocument& document, const char* key, float fallback) {
    return static_cast<float>(document.numberOr(key, fallback));
}

bool boolOr(const novacore::core::ConfigDocument& document, const char* key, bool fallback) {
    return document.boolOr(key, fallback);
}

[[nodiscard]] novacore::math::Vec3 vectorOr(
    const novacore::core::ConfigDocument& document,
    const char* prefix,
    novacore::math::Vec3 fallback) {
    const std::string base(prefix);
    fallback.x = numberOr(document, (base + ".x").c_str(), fallback.x);
    fallback.y = numberOr(document, (base + ".y").c_str(), fallback.y);
    fallback.z = numberOr(document, (base + ".z").c_str(), fallback.z);
    return fallback;
}

[[nodiscard]] novacore::math::Vec3 normalizedOr(
    novacore::math::Vec3 value,
    novacore::math::Vec3 fallback) {
    const float lengthSquared = value.lengthSquared();
    if (lengthSquared <= 0.000001F) {
        return fallback;
    }

    const float invLength = 1.0F / std::sqrt(lengthSquared);
    return {value.x * invLength, value.y * invLength, value.z * invLength};
}

} // namespace

DevRenderTuning devRenderTuningFromConfig(
    const novacore::core::ConfigDocument& document,
    DevRenderTuning fallback) {
    fallback.lighting.sunDirection = normalizedOr(
        vectorOr(document, "lighting.sun_direction", fallback.lighting.sunDirection),
        fallback.lighting.sunDirection);
    fallback.lighting.fillDirection = normalizedOr(
        vectorOr(document, "lighting.fill_direction", fallback.lighting.fillDirection),
        fallback.lighting.fillDirection);
    fallback.lighting.ambientIntensity = std::clamp(
        numberOr(document, "lighting.ambient_intensity", fallback.lighting.ambientIntensity),
        0.02F,
        0.95F);
    fallback.lighting.fillIntensity = std::clamp(
        numberOr(document, "lighting.fill_intensity", fallback.lighting.fillIntensity),
        0.0F,
        1.0F);
    fallback.lighting.rimIntensity = std::clamp(
        numberOr(document, "lighting.rim_intensity", fallback.lighting.rimIntensity),
        0.0F,
        1.0F);
    fallback.lighting.specularIntensity = std::clamp(
        numberOr(document, "lighting.specular_intensity", fallback.lighting.specularIntensity),
        0.0F,
        1.0F);
    fallback.lighting.contrast = std::clamp(
        numberOr(document, "lighting.contrast", fallback.lighting.contrast),
        0.5F,
        1.8F);
    fallback.lighting.saturation = std::clamp(
        numberOr(document, "lighting.saturation", fallback.lighting.saturation),
        0.0F,
        2.0F);

    fallback.verticalFovDegrees = std::clamp(
        numberOr(document, "camera.vertical_fov_degrees", fallback.verticalFovDegrees),
        55.0F,
        105.0F);
    fallback.nearPlane = std::clamp(
        numberOr(document, "camera.near_plane", fallback.nearPlane),
        0.01F,
        0.50F);
    fallback.farPlane = std::clamp(
        numberOr(document, "camera.far_plane", fallback.farPlane),
        fallback.nearPlane + 1.0F,
        1000.0F);
    fallback.showWorldDebugLines = boolOr(
        document,
        "debug.world_lines",
        fallback.showWorldDebugLines);
    return fallback;
}

} // namespace nemisis::render

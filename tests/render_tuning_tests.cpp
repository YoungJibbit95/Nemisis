#include "nemisis/render/RenderTuning.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

void testRenderTuningLoadsLightingAndCamera() {
    novacore::core::ConfigDocument document;
    document.set("lighting.sun_direction.x", "0");
    document.set("lighting.sun_direction.y", "10");
    document.set("lighting.sun_direction.z", "0");
    document.set("lighting.fill_direction.x", "-2");
    document.set("lighting.fill_direction.y", "1");
    document.set("lighting.fill_direction.z", "0");
    document.set("lighting.ambient_intensity", "0.52");
    document.set("lighting.fill_intensity", "0.31");
    document.set("lighting.rim_intensity", "0.24");
    document.set("lighting.specular_intensity", "0.19");
    document.set("lighting.contrast", "1.22");
    document.set("lighting.saturation", "1.15");
    document.set("sky.enabled", "true");
    document.set("sky.zenith_color.x", "0.12");
    document.set("sky.zenith_color.y", "0.22");
    document.set("sky.zenith_color.z", "0.44");
    document.set("sky.horizon_color.x", "0.55");
    document.set("sky.horizon_color.y", "0.66");
    document.set("sky.horizon_color.z", "0.77");
    document.set("sky.ground_color.x", "0.03");
    document.set("sky.ground_color.y", "0.04");
    document.set("sky.ground_color.z", "0.05");
    document.set("sky.horizon_height", "0.41");
    document.set("sky.gradient_power", "1.7");
    document.set("sky.exposure", "1.2");
    document.set("sky.sun_angular_radius_degrees", "2.2");
    document.set("sky.sun_intensity", "1.8");
    document.set("sky.haze_strength", "0.36");
    document.set("sky.cloud_strength", "0.28");
    document.set("lighting.contact_shadows_enabled", "true");
    document.set("lighting.contact_shadow_opacity", "0.52");
    document.set("lighting.contact_shadow_softness", "0.71");
    document.set("camera.vertical_fov_degrees", "82");
    document.set("camera.near_plane", "0.05");
    document.set("camera.far_plane", "250");
    document.set("debug.world_lines", "false");

    const auto tuning = nemisis::render::devRenderTuningFromConfig(document);
    expect(tuning.lighting.sunDirection.x == 0.0F, "render tuning keeps sun x");
    expect(tuning.lighting.sunDirection.y > 0.99F, "render tuning normalizes sun y");
    expect(tuning.lighting.sunDirection.z == 0.0F, "render tuning keeps sun z");
    expect(tuning.lighting.fillDirection.x < -0.89F, "render tuning normalizes fill direction x");
    expect(tuning.lighting.fillDirection.y > 0.44F, "render tuning normalizes fill direction y");
    expect(tuning.lighting.ambientIntensity > 0.51F && tuning.lighting.ambientIntensity < 0.53F, "render tuning loads ambient");
    expect(tuning.lighting.fillIntensity > 0.30F && tuning.lighting.fillIntensity < 0.32F, "render tuning loads fill intensity");
    expect(tuning.lighting.rimIntensity > 0.23F && tuning.lighting.rimIntensity < 0.25F, "render tuning loads rim intensity");
    expect(tuning.lighting.specularIntensity > 0.18F && tuning.lighting.specularIntensity < 0.20F, "render tuning loads specular intensity");
    expect(tuning.lighting.contrast > 1.21F && tuning.lighting.contrast < 1.23F, "render tuning loads contrast");
    expect(tuning.lighting.saturation > 1.14F && tuning.lighting.saturation < 1.16F, "render tuning loads saturation");
    expect(tuning.sky.enabled, "render tuning enables sky");
    expect(tuning.sky.zenithColor[2] > 0.43F && tuning.sky.zenithColor[2] < 0.45F, "render tuning loads sky zenith");
    expect(tuning.sky.horizonColor[1] > 0.65F && tuning.sky.horizonColor[1] < 0.67F, "render tuning loads sky horizon");
    expect(tuning.sky.groundColor[0] > 0.02F && tuning.sky.groundColor[0] < 0.04F, "render tuning loads sky ground");
    expect(tuning.sky.horizonHeight > 0.40F && tuning.sky.horizonHeight < 0.42F, "render tuning loads sky horizon height");
    expect(tuning.sky.gradientPower > 1.69F && tuning.sky.gradientPower < 1.71F, "render tuning loads sky power");
    expect(tuning.sky.exposure > 1.19F && tuning.sky.exposure < 1.21F, "render tuning loads sky exposure");
    expect(tuning.sky.sunAngularRadiusDegrees > 2.19F && tuning.sky.sunAngularRadiusDegrees < 2.21F, "render tuning loads sun radius");
    expect(tuning.sky.sunIntensity > 1.79F && tuning.sky.sunIntensity < 1.81F, "render tuning loads sun intensity");
    expect(tuning.sky.hazeStrength > 0.35F && tuning.sky.hazeStrength < 0.37F, "render tuning loads atmospheric haze");
    expect(tuning.sky.cloudStrength > 0.27F && tuning.sky.cloudStrength < 0.29F, "render tuning loads cloud coverage");
    expect(tuning.lighting.contactShadowsEnabled, "render tuning enables contact shadows");
    expect(tuning.lighting.contactShadowOpacity > 0.51F && tuning.lighting.contactShadowOpacity < 0.53F, "render tuning loads shadow opacity");
    expect(tuning.lighting.contactShadowSoftness > 0.70F && tuning.lighting.contactShadowSoftness < 0.72F, "render tuning loads shadow softness");
    expect(tuning.verticalFovDegrees == 82.0F, "render tuning loads FOV");
    expect(tuning.nearPlane == 0.05F, "render tuning loads near plane");
    expect(tuning.farPlane == 250.0F, "render tuning loads far plane");
    expect(!tuning.showWorldDebugLines, "render tuning loads debug line toggle");
}

void testRenderTuningClampsUnsafeValues() {
    novacore::core::ConfigDocument document;
    document.set("lighting.sun_direction.x", "0");
    document.set("lighting.sun_direction.y", "0");
    document.set("lighting.sun_direction.z", "0");
    document.set("lighting.ambient_intensity", "2.0");
    document.set("lighting.fill_intensity", "5.0");
    document.set("lighting.rim_intensity", "-1.0");
    document.set("lighting.specular_intensity", "2.0");
    document.set("lighting.contrast", "8.0");
    document.set("lighting.saturation", "-2.0");
    document.set("sky.horizon_height", "2.0");
    document.set("sky.gradient_power", "-1.0");
    document.set("sky.exposure", "9.0");
    document.set("sky.sun_angular_radius_degrees", "90.0");
    document.set("sky.sun_intensity", "20.0");
    document.set("sky.haze_strength", "2.0");
    document.set("sky.cloud_strength", "-2.0");
    document.set("lighting.contact_shadow_opacity", "4.0");
    document.set("lighting.contact_shadow_softness", "-1.0");
    document.set("camera.vertical_fov_degrees", "140");
    document.set("camera.near_plane", "-1");
    document.set("camera.far_plane", "0.1");

    nemisis::render::DevRenderTuning fallback{};
    fallback.lighting.sunDirection = {0.25F, 0.75F, 0.25F};
    fallback.lighting.ambientIntensity = 0.33F;
    fallback.verticalFovDegrees = 70.0F;
    fallback.nearPlane = 0.04F;
    fallback.farPlane = 100.0F;

    const auto tuning = nemisis::render::devRenderTuningFromConfig(document, fallback);
    expect(tuning.lighting.sunDirection.x == fallback.lighting.sunDirection.x, "zero sun direction falls back x");
    expect(tuning.lighting.sunDirection.y == fallback.lighting.sunDirection.y, "zero sun direction falls back y");
    expect(tuning.lighting.ambientIntensity == 0.95F, "ambient clamps high");
    expect(tuning.lighting.fillIntensity == 1.0F, "fill intensity clamps high");
    expect(tuning.lighting.rimIntensity == 0.0F, "rim intensity clamps low");
    expect(tuning.lighting.specularIntensity == 1.0F, "specular intensity clamps high");
    expect(tuning.lighting.contrast == 1.8F, "contrast clamps high");
    expect(tuning.lighting.saturation == 0.0F, "saturation clamps low");
    expect(tuning.sky.horizonHeight == 0.95F, "sky horizon clamps high");
    expect(tuning.sky.gradientPower == 0.20F, "sky power clamps low");
    expect(tuning.sky.exposure == 4.0F, "sky exposure clamps high");
    expect(tuning.sky.sunAngularRadiusDegrees == 12.0F, "sky sun radius clamps high");
    expect(tuning.sky.sunIntensity == 8.0F, "sky sun intensity clamps high");
    expect(tuning.sky.hazeStrength == 1.0F, "sky haze clamps high");
    expect(tuning.sky.cloudStrength == 0.0F, "sky clouds clamp low");
    expect(tuning.lighting.contactShadowOpacity == 1.0F, "shadow opacity clamps high");
    expect(tuning.lighting.contactShadowSoftness == 0.05F, "shadow softness clamps low");
    expect(tuning.verticalFovDegrees == 105.0F, "FOV clamps high");
    expect(tuning.nearPlane == 0.01F, "near plane clamps low");
    expect(tuning.farPlane > tuning.nearPlane, "far plane stays beyond near plane");
}

void testRenderTuningPreservesFallbacksWhenMissing() {
    novacore::core::ConfigDocument document;
    nemisis::render::DevRenderTuning fallback{};
    fallback.lighting.ambientIntensity = 0.44F;
    fallback.sky.enabled = false;
    fallback.sky.exposure = 0.80F;
    fallback.verticalFovDegrees = 78.0F;
    fallback.showWorldDebugLines = false;

    const auto tuning = nemisis::render::devRenderTuningFromConfig(document, fallback);
    expect(tuning.lighting.ambientIntensity == 0.44F, "missing ambient preserves fallback");
    expect(!tuning.sky.enabled, "missing sky toggle preserves fallback");
    expect(tuning.sky.exposure == 0.80F, "missing sky exposure preserves fallback");
    expect(tuning.verticalFovDegrees == 78.0F, "missing FOV preserves fallback");
    expect(!tuning.showWorldDebugLines, "missing debug toggle preserves fallback");
}

} // namespace

int main() {
    testRenderTuningLoadsLightingAndCamera();
    testRenderTuningClampsUnsafeValues();
    testRenderTuningPreservesFallbacksWhenMissing();

    if (failures > 0) {
        std::cerr << failures << " render tuning test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis render tuning tests passed\n";
    return EXIT_SUCCESS;
}

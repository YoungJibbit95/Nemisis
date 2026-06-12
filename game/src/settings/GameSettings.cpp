#include "nemisis/settings/GameSettings.hpp"

#include <algorithm>

namespace nemisis::settings {

namespace {

[[nodiscard]] float numberOr(const novacore::core::ConfigDocument& document, const char* key, float fallback) {
    return static_cast<float>(document.numberOr(key, fallback));
}

[[nodiscard]] bool boolOr(const novacore::core::ConfigDocument& document, const char* key, bool fallback) {
    return document.boolOr(key, fallback);
}

[[nodiscard]] float clamped(float value, float minValue, float maxValue) {
    return std::clamp(value, minValue, maxValue);
}

} // namespace

GameSettings gameSettingsFromConfig(
    const novacore::core::ConfigDocument& document,
    GameSettings fallback) {
    fallback.mouse.sensitivityX = numberOr(document, "mouse.sensitivity_x", fallback.mouse.sensitivityX);
    fallback.mouse.sensitivityY = numberOr(document, "mouse.sensitivity_y", fallback.mouse.sensitivityY);
    fallback.mouse.adsMultiplier = numberOr(document, "mouse.ads_multiplier", fallback.mouse.adsMultiplier);
    fallback.mouse.invertY = boolOr(document, "mouse.invert_y", fallback.mouse.invertY);

    fallback.controller.lookSensitivityX = numberOr(document, "controller.look_sensitivity_x", fallback.controller.lookSensitivityX);
    fallback.controller.lookSensitivityY = numberOr(document, "controller.look_sensitivity_y", fallback.controller.lookSensitivityY);
    fallback.controller.leftStickDeadzone = numberOr(document, "controller.left_stick_deadzone", fallback.controller.leftStickDeadzone);
    fallback.controller.rightStickDeadzone = numberOr(document, "controller.right_stick_deadzone", fallback.controller.rightStickDeadzone);
    fallback.controller.responseCurve = document.stringOr("controller.response_curve", fallback.controller.responseCurve);
    fallback.controller.aimAssistEnabled = boolOr(document, "controller.aim_assist.enabled", fallback.controller.aimAssistEnabled);
    fallback.controller.aimAssistSlowdown = numberOr(document, "controller.aim_assist.slowdown_strength", fallback.controller.aimAssistSlowdown);
    fallback.controller.aimAssistRotation = numberOr(document, "controller.aim_assist.rotation_strength", fallback.controller.aimAssistRotation);

    fallback.gameplay.holdToSprint = boolOr(document, "gameplay.hold_to_sprint", fallback.gameplay.holdToSprint);
    fallback.gameplay.toggleAds = boolOr(document, "gameplay.toggle_ads", fallback.gameplay.toggleAds);
    fallback.gameplay.showHitMarkers = boolOr(document, "gameplay.show_hit_markers", fallback.gameplay.showHitMarkers);
    fallback.gameplay.showDamageNumbers = boolOr(document, "gameplay.show_damage_numbers", fallback.gameplay.showDamageNumbers);
    fallback.gameplay.screenShakeScale = numberOr(document, "gameplay.screen_shake_scale", fallback.gameplay.screenShakeScale);

    fallback.video.hudScale = numberOr(document, "video.hud_scale", fallback.video.hudScale);
    fallback.video.menuSafeArea = numberOr(document, "video.menu_safe_area", fallback.video.menuSafeArea);
    fallback.video.showDebugWorldLines = boolOr(document, "video.show_debug_world_lines", fallback.video.showDebugWorldLines);

    fallback.mouse.sensitivityX = clamped(fallback.mouse.sensitivityX, 0.05F, 8.0F);
    fallback.mouse.sensitivityY = clamped(fallback.mouse.sensitivityY, 0.05F, 8.0F);
    fallback.mouse.adsMultiplier = clamped(fallback.mouse.adsMultiplier, 0.10F, 2.0F);
    fallback.controller.lookSensitivityX = clamped(fallback.controller.lookSensitivityX, 0.05F, 8.0F);
    fallback.controller.lookSensitivityY = clamped(fallback.controller.lookSensitivityY, 0.05F, 8.0F);
    fallback.controller.leftStickDeadzone = clamped(fallback.controller.leftStickDeadzone, 0.0F, 0.5F);
    fallback.controller.rightStickDeadzone = clamped(fallback.controller.rightStickDeadzone, 0.0F, 0.5F);
    fallback.controller.aimAssistSlowdown = clamped(fallback.controller.aimAssistSlowdown, 0.0F, 1.0F);
    fallback.controller.aimAssistRotation = clamped(fallback.controller.aimAssistRotation, 0.0F, 1.0F);
    fallback.gameplay.screenShakeScale = clamped(fallback.gameplay.screenShakeScale, 0.0F, 2.0F);
    fallback.video.hudScale = clamped(fallback.video.hudScale, 0.75F, 1.35F);
    fallback.video.menuSafeArea = clamped(fallback.video.menuSafeArea, 0.80F, 1.0F);
    return fallback;
}

void adjustMouseSensitivity(GameSettings& settings, float delta) {
    settings.mouse.sensitivityX = clamped(settings.mouse.sensitivityX + delta, 0.05F, 8.0F);
    settings.mouse.sensitivityY = clamped(settings.mouse.sensitivityY + delta, 0.05F, 8.0F);
}

void adjustControllerSensitivity(GameSettings& settings, float delta) {
    settings.controller.lookSensitivityX = clamped(settings.controller.lookSensitivityX + delta, 0.05F, 8.0F);
    settings.controller.lookSensitivityY = clamped(settings.controller.lookSensitivityY + delta, 0.05F, 8.0F);
}

void adjustHudScale(GameSettings& settings, float delta) {
    settings.video.hudScale = clamped(settings.video.hudScale + delta, 0.75F, 1.35F);
}

void toggleAimAssist(GameSettings& settings) {
    settings.controller.aimAssistEnabled = !settings.controller.aimAssistEnabled;
}

void toggleDamageNumbers(GameSettings& settings) {
    settings.gameplay.showDamageNumbers = !settings.gameplay.showDamageNumbers;
}

float effectiveLookScaleX(const GameSettings& settings, bool controller, bool adsHeld) {
    const float base = controller ? settings.controller.lookSensitivityX : settings.mouse.sensitivityX;
    return base * (adsHeld ? settings.mouse.adsMultiplier : 1.0F);
}

float effectiveLookScaleY(const GameSettings& settings, bool controller, bool adsHeld) {
    const float base = controller ? settings.controller.lookSensitivityY : settings.mouse.sensitivityY;
    const float invert = settings.mouse.invertY ? -1.0F : 1.0F;
    return base * invert * (adsHeld ? settings.mouse.adsMultiplier : 1.0F);
}

} // namespace nemisis::settings

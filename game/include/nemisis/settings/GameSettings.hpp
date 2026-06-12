#pragma once

#include "novacore/core/ConfigDocument.hpp"

#include <string>

namespace nemisis::settings {

struct MouseSettings final {
    float sensitivityX = 1.0F;
    float sensitivityY = 1.0F;
    float adsMultiplier = 0.85F;
    bool invertY = false;
};

struct ControllerSettings final {
    float lookSensitivityX = 1.0F;
    float lookSensitivityY = 1.0F;
    float leftStickDeadzone = 0.08F;
    float rightStickDeadzone = 0.06F;
    float aimAssistSlowdown = 0.22F;
    float aimAssistRotation = 0.08F;
    bool aimAssistEnabled = true;
    std::string responseCurve = "dynamic";
};

struct GameplaySettings final {
    bool holdToSprint = true;
    bool toggleAds = false;
    bool showHitMarkers = true;
    bool showDamageNumbers = true;
    float screenShakeScale = 0.65F;
};

struct VideoSettings final {
    float hudScale = 1.0F;
    float menuSafeArea = 0.96F;
    bool showDebugWorldLines = true;
};

struct GameSettings final {
    MouseSettings mouse{};
    ControllerSettings controller{};
    GameplaySettings gameplay{};
    VideoSettings video{};
};

[[nodiscard]] GameSettings gameSettingsFromConfig(
    const novacore::core::ConfigDocument& document,
    GameSettings fallback = {});

void adjustMouseSensitivity(GameSettings& settings, float delta);
void adjustControllerSensitivity(GameSettings& settings, float delta);
void adjustHudScale(GameSettings& settings, float delta);
void toggleAimAssist(GameSettings& settings);
void toggleDamageNumbers(GameSettings& settings);

[[nodiscard]] float effectiveLookScaleX(const GameSettings& settings, bool controller, bool adsHeld);
[[nodiscard]] float effectiveLookScaleY(const GameSettings& settings, bool controller, bool adsHeld);

} // namespace nemisis::settings

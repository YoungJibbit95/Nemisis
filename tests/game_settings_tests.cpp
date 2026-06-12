#include "nemisis/settings/GameSettings.hpp"

#include "novacore/core/ConfigDocument.hpp"

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

void testSettingsParseAndClamp() {
    constexpr std::string_view json = R"json(
{
  "mouse": {
    "sensitivity_x": 12.0,
    "sensitivity_y": 0.01,
    "ads_multiplier": 0.5,
    "invert_y": true
  },
  "controller": {
    "look_sensitivity_x": 2.4,
    "look_sensitivity_y": 2.2,
    "left_stick_deadzone": 0.9,
    "right_stick_deadzone": 0.2,
    "response_curve": "linear",
    "aim_assist": {
      "enabled": false,
      "slowdown_strength": 0.35,
      "rotation_strength": 0.12
    }
  },
  "gameplay": {
    "show_damage_numbers": false,
    "screen_shake_scale": 2.5
  },
  "video": {
    "hud_scale": 1.8,
    "menu_safe_area": 0.7,
    "show_debug_world_lines": false
  }
}
)json";

    novacore::core::ConfigDocument document;
    const auto parseResult = novacore::core::parseJsonConfig(json, document);
    expect(parseResult.ok(), "game settings fixture parses");

    const auto settings = nemisis::settings::gameSettingsFromConfig(document);
    expect(settings.mouse.sensitivityX == 8.0F, "mouse sensitivity x clamps high");
    expect(settings.mouse.sensitivityY == 0.05F, "mouse sensitivity y clamps low");
    expect(settings.mouse.invertY, "mouse invert y parses");
    expect(settings.controller.leftStickDeadzone == 0.5F, "controller deadzone clamps high");
    expect(settings.controller.responseCurve == "linear", "controller response curve parses");
    expect(!settings.controller.aimAssistEnabled, "aim assist enabled parses");
    expect(!settings.gameplay.showDamageNumbers, "damage number toggle parses");
    expect(settings.gameplay.screenShakeScale == 2.0F, "screen shake clamps high");
    expect(settings.video.hudScale == 1.35F, "hud scale clamps high");
    expect(settings.video.menuSafeArea == 0.80F, "menu safe area clamps low");
    expect(!settings.video.showDebugWorldLines, "debug world line toggle parses");
}

void testSettingsAdjustmentsAndLookScale() {
    nemisis::settings::GameSettings settings{};
    settings.mouse.sensitivityX = 1.0F;
    settings.mouse.sensitivityY = 2.0F;
    settings.mouse.adsMultiplier = 0.5F;
    settings.mouse.invertY = true;

    nemisis::settings::adjustMouseSensitivity(settings, 0.25F);
    nemisis::settings::adjustControllerSensitivity(settings, 0.75F);
    nemisis::settings::adjustHudScale(settings, 0.10F);
    nemisis::settings::toggleAimAssist(settings);
    nemisis::settings::toggleDamageNumbers(settings);

    expect(settings.mouse.sensitivityX > 1.24F && settings.mouse.sensitivityX < 1.26F, "mouse sensitivity adjusts");
    expect(settings.controller.lookSensitivityX > 1.74F && settings.controller.lookSensitivityX < 1.76F, "controller sensitivity adjusts");
    expect(settings.video.hudScale > 1.09F && settings.video.hudScale < 1.11F, "hud scale adjusts");
    expect(!settings.controller.aimAssistEnabled, "aim assist toggles off");
    expect(!settings.gameplay.showDamageNumbers, "damage numbers toggle off");
    expect(nemisis::settings::effectiveLookScaleX(settings, false, true) > 0.62F, "mouse ADS look scale includes multiplier");
    expect(nemisis::settings::effectiveLookScaleY(settings, false, true) < -0.99F, "mouse Y look scale includes inversion");
}

} // namespace

int main() {
    testSettingsParseAndClamp();
    testSettingsAdjustmentsAndLookScale();

    if (failures > 0) {
        std::cerr << failures << " game settings test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis game settings tests passed\n";
    return EXIT_SUCCESS;
}

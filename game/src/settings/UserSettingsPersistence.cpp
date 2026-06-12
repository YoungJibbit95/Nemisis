#include "nemisis/settings/UserSettingsPersistence.hpp"

#include "novacore/core/ConfigDocument.hpp"
#include "novacore/io/FileSystem.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace nemisis::settings {

namespace {

constexpr std::array<weapons::AttachmentSlot, weapons::kAttachmentSlotCount> kSlots{
    weapons::AttachmentSlot::Optic,
    weapons::AttachmentSlot::Barrel,
    weapons::AttachmentSlot::Muzzle,
    weapons::AttachmentSlot::Underbarrel,
    weapons::AttachmentSlot::Magazine,
    weapons::AttachmentSlot::Stock,
};

[[nodiscard]] std::string boolText(bool value) {
    return value ? "true" : "false";
}

[[nodiscard]] std::string quoted(std::string_view value) {
    std::string result;
    result.reserve(value.size() + 2U);
    result.push_back('"');
    for (const char character : value) {
        switch (character) {
        case '\\':
            result += "\\\\";
            break;
        case '"':
            result += "\\\"";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result.push_back(character);
            break;
        }
    }
    result.push_back('"');
    return result;
}

[[nodiscard]] std::string slotKey(weapons::AttachmentSlot slot) {
    switch (slot) {
    case weapons::AttachmentSlot::Optic:
        return "optic";
    case weapons::AttachmentSlot::Barrel:
        return "barrel";
    case weapons::AttachmentSlot::Muzzle:
        return "muzzle";
    case weapons::AttachmentSlot::Underbarrel:
        return "underbarrel";
    case weapons::AttachmentSlot::Magazine:
        return "magazine";
    case weapons::AttachmentSlot::Stock:
        return "stock";
    case weapons::AttachmentSlot::Count:
        break;
    }
    return "unknown";
}

[[nodiscard]] weapons::WeaponLoadout loadoutFromConfig(
    const novacore::core::ConfigDocument& document,
    weapons::WeaponLoadout fallback,
    const weapons::AttachmentRegistry& attachments) {
    fallback.weaponId = document.stringOr("loadout.weapon_id", fallback.weaponId);

    for (const auto slot : kSlots) {
        const auto key = "loadout.attachments." + slotKey(slot);
        const auto id = document.stringValue(key);
        if (!id.has_value()) {
            continue;
        }
        if (id->empty() || attachments.find(*id) != nullptr) {
            weapons::setAttachment(fallback, slot, *id);
        }
    }

    return fallback;
}

} // namespace

std::filesystem::path defaultUserSettingsPath() {
    return std::filesystem::path("configs") / "user" / "nemisis_user.json";
}

std::string serializeUserSettingsSnapshot(const UserSettingsSnapshot& snapshot) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(3);
    stream << "{\n";
    stream << "  \"mouse\": {\n";
    stream << "    \"sensitivity_x\": " << snapshot.settings.mouse.sensitivityX << ",\n";
    stream << "    \"sensitivity_y\": " << snapshot.settings.mouse.sensitivityY << ",\n";
    stream << "    \"ads_multiplier\": " << snapshot.settings.mouse.adsMultiplier << ",\n";
    stream << "    \"invert_y\": " << boolText(snapshot.settings.mouse.invertY) << "\n";
    stream << "  },\n";
    stream << "  \"controller\": {\n";
    stream << "    \"look_sensitivity_x\": " << snapshot.settings.controller.lookSensitivityX << ",\n";
    stream << "    \"look_sensitivity_y\": " << snapshot.settings.controller.lookSensitivityY << ",\n";
    stream << "    \"left_stick_deadzone\": " << snapshot.settings.controller.leftStickDeadzone << ",\n";
    stream << "    \"right_stick_deadzone\": " << snapshot.settings.controller.rightStickDeadzone << ",\n";
    stream << "    \"response_curve\": " << quoted(snapshot.settings.controller.responseCurve) << ",\n";
    stream << "    \"aim_assist\": {\n";
    stream << "      \"enabled\": " << boolText(snapshot.settings.controller.aimAssistEnabled) << ",\n";
    stream << "      \"slowdown_strength\": " << snapshot.settings.controller.aimAssistSlowdown << ",\n";
    stream << "      \"rotation_strength\": " << snapshot.settings.controller.aimAssistRotation << "\n";
    stream << "    }\n";
    stream << "  },\n";
    stream << "  \"gameplay\": {\n";
    stream << "    \"hold_to_sprint\": " << boolText(snapshot.settings.gameplay.holdToSprint) << ",\n";
    stream << "    \"toggle_ads\": " << boolText(snapshot.settings.gameplay.toggleAds) << ",\n";
    stream << "    \"show_hit_markers\": " << boolText(snapshot.settings.gameplay.showHitMarkers) << ",\n";
    stream << "    \"show_damage_numbers\": " << boolText(snapshot.settings.gameplay.showDamageNumbers) << ",\n";
    stream << "    \"screen_shake_scale\": " << snapshot.settings.gameplay.screenShakeScale << "\n";
    stream << "  },\n";
    stream << "  \"video\": {\n";
    stream << "    \"hud_scale\": " << snapshot.settings.video.hudScale << ",\n";
    stream << "    \"menu_safe_area\": " << snapshot.settings.video.menuSafeArea << ",\n";
    stream << "    \"show_debug_world_lines\": " << boolText(snapshot.settings.video.showDebugWorldLines) << "\n";
    stream << "  },\n";
    stream << "  \"loadout\": {\n";
    stream << "    \"weapon_id\": " << quoted(snapshot.loadout.weaponId) << ",\n";
    stream << "    \"attachments\": {\n";
    for (std::size_t index = 0; index < kSlots.size(); ++index) {
        const auto slot = kSlots[index];
        stream << "      \"" << slotKey(slot) << "\": "
               << quoted(weapons::selectedAttachmentId(snapshot.loadout, slot));
        if (index + 1U < kSlots.size()) {
            stream << ',';
        }
        stream << '\n';
    }
    stream << "    }\n";
    stream << "  }\n";
    stream << "}\n";
    return stream.str();
}

UserSettingsLoadResult loadUserSettingsSnapshot(
    const std::filesystem::path& path,
    const UserSettingsSnapshot& fallback,
    const weapons::AttachmentRegistry& attachments) {
    UserSettingsLoadResult result{};
    result.snapshot = fallback;

    if (!std::filesystem::exists(path)) {
        return result;
    }

    const auto file = novacore::io::readTextFile(path);
    if (!file.has_value()) {
        result.errors.push_back("Could not read user settings: " + path.string());
        return result;
    }

    novacore::core::ConfigDocument document;
    const auto parseResult = novacore::core::parseJsonConfig(file->text, document);
    if (!parseResult.ok()) {
        for (const auto& error : parseResult.errors) {
            result.errors.push_back("User settings parse error at " + std::to_string(error.offset) + ": " + error.message);
        }
        return result;
    }

    result.snapshot.settings = gameSettingsFromConfig(document, fallback.settings);
    result.snapshot.loadout = loadoutFromConfig(document, fallback.loadout, attachments);
    result.loaded = true;
    return result;
}

UserSettingsSaveResult saveUserSettingsSnapshot(
    const std::filesystem::path& path,
    const UserSettingsSnapshot& snapshot) {
    UserSettingsSaveResult result{};
    std::error_code error;
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, error);
        if (error) {
            result.errors.push_back("Could not create user settings directory: " + parent.string());
            return result;
        }
    }

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        result.errors.push_back("Could not open user settings for write: " + path.string());
        return result;
    }

    const auto text = serializeUserSettingsSnapshot(snapshot);
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!file.good()) {
        result.errors.push_back("Could not write user settings: " + path.string());
        return result;
    }

    result.saved = true;
    return result;
}

} // namespace nemisis::settings

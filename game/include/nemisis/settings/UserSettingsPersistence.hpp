#pragma once

#include "nemisis/settings/GameSettings.hpp"
#include "nemisis/weapons/WeaponAttachments.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace nemisis::settings {

struct UserSettingsSnapshot final {
    GameSettings settings{};
    weapons::WeaponLoadout loadout{};
};

struct UserSettingsLoadResult final {
    UserSettingsSnapshot snapshot{};
    bool loaded = false;
    std::vector<std::string> errors;

    [[nodiscard]] bool ok() const {
        return errors.empty();
    }
};

struct UserSettingsSaveResult final {
    bool saved = false;
    std::vector<std::string> errors;

    [[nodiscard]] bool ok() const {
        return errors.empty();
    }
};

[[nodiscard]] std::filesystem::path defaultUserSettingsPath();

[[nodiscard]] std::string serializeUserSettingsSnapshot(const UserSettingsSnapshot& snapshot);

[[nodiscard]] UserSettingsLoadResult loadUserSettingsSnapshot(
    const std::filesystem::path& path,
    const UserSettingsSnapshot& fallback,
    const weapons::AttachmentRegistry& attachments);

[[nodiscard]] UserSettingsSaveResult saveUserSettingsSnapshot(
    const std::filesystem::path& path,
    const UserSettingsSnapshot& snapshot);

} // namespace nemisis::settings

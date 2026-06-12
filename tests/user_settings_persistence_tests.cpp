#include "nemisis/settings/UserSettingsPersistence.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
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

nemisis::settings::UserSettingsSnapshot makeSnapshot() {
    nemisis::settings::UserSettingsSnapshot snapshot{};
    snapshot.settings.mouse.sensitivityX = 1.75F;
    snapshot.settings.mouse.sensitivityY = 1.25F;
    snapshot.settings.mouse.adsMultiplier = 0.72F;
    snapshot.settings.controller.lookSensitivityX = 2.0F;
    snapshot.settings.controller.aimAssistEnabled = false;
    snapshot.settings.gameplay.showDamageNumbers = false;
    snapshot.settings.video.hudScale = 1.15F;
    snapshot.loadout = nemisis::weapons::defaultPrototypeLoadout("smg_01");
    nemisis::weapons::setAttachment(snapshot.loadout, nemisis::weapons::AttachmentSlot::Magazine, "mag_ext_45_01");
    return snapshot;
}

void testSerializeSaveAndLoadUserSettings() {
    nemisis::weapons::AttachmentRegistry attachments;
    attachments.registerPrototypeAttachments();
    const auto path = std::filesystem::temp_directory_path() / "nemisis_user_settings_test" / "profile.json";
    std::filesystem::remove_all(path.parent_path());

    const auto snapshot = makeSnapshot();
    const auto text = nemisis::settings::serializeUserSettingsSnapshot(snapshot);
    expect(text.find("\"weapon_id\": \"smg_01\"") != std::string::npos, "serialized user settings include weapon id");
    expect(text.find("\"magazine\": \"mag_ext_45_01\"") != std::string::npos, "serialized user settings include attachment id");

    const auto save = nemisis::settings::saveUserSettingsSnapshot(path, snapshot);
    expect(save.ok() && save.saved, "user settings save succeeds");
    expect(std::filesystem::exists(path), "user settings file exists after save");

    const auto loaded = nemisis::settings::loadUserSettingsSnapshot(path, {}, attachments);
    expect(loaded.ok() && loaded.loaded, "user settings load succeeds");
    expect(loaded.snapshot.settings.mouse.sensitivityX > 1.74F, "loaded mouse sensitivity survives round trip");
    expect(!loaded.snapshot.settings.controller.aimAssistEnabled, "loaded aim assist toggle survives round trip");
    expect(!loaded.snapshot.settings.gameplay.showDamageNumbers, "loaded damage numbers toggle survives round trip");
    expect(loaded.snapshot.loadout.weaponId == "smg_01", "loaded weapon id survives round trip");
    expect(nemisis::weapons::selectedAttachmentId(loaded.snapshot.loadout, nemisis::weapons::AttachmentSlot::Magazine) == "mag_ext_45_01", "loaded magazine attachment survives round trip");

    std::filesystem::remove_all(path.parent_path());
}

void testMissingAndInvalidFilesUseFallback() {
    nemisis::weapons::AttachmentRegistry attachments;
    attachments.registerPrototypeAttachments();
    const auto missingPath = std::filesystem::temp_directory_path() / "nemisis_user_settings_missing" / "profile.json";
    std::filesystem::remove_all(missingPath.parent_path());

    const auto fallback = makeSnapshot();
    const auto missing = nemisis::settings::loadUserSettingsSnapshot(missingPath, fallback, attachments);
    expect(missing.ok() && !missing.loaded, "missing user settings is not an error");
    expect(missing.snapshot.loadout.weaponId == fallback.loadout.weaponId, "missing user settings returns fallback loadout");

    std::filesystem::create_directories(missingPath.parent_path());
    {
        std::ofstream file(missingPath);
        file << "{ not json";
    }
    const auto invalid = nemisis::settings::loadUserSettingsSnapshot(missingPath, fallback, attachments);
    expect(!invalid.ok(), "invalid user settings reports parse errors");

    std::filesystem::remove_all(missingPath.parent_path());
}

} // namespace

int main() {
    testSerializeSaveAndLoadUserSettings();
    testMissingAndInvalidFilesUseFallback();

    if (failures > 0) {
        std::cerr << failures << " user settings persistence test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis user settings persistence tests passed\n";
    return EXIT_SUCCESS;
}

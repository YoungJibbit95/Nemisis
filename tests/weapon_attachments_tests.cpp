#include "nemisis/weapons/WeaponAttachments.hpp"

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

nemisis::weapons::WeaponDefinition makeBaseWeapon() {
    nemisis::weapons::WeaponDefinition weapon{};
    weapon.id = "ar_01";
    weapon.displayName = "Prototype AR";
    weapon.weaponClass = nemisis::weapons::WeaponClass::AssaultRifle;
    weapon.magazineSize = 30;
    weapon.fireRateRpm = 720;
    weapon.adsTimeSeconds = 0.20F;
    weapon.reloadTimeSeconds = 2.0F;
    weapon.maxRangeMeters = 80.0F;
    weapon.hipSpreadDegrees = 1.2F;
    weapon.adsSpreadDegrees = 0.24F;
    weapon.recoilPitchPerShotDegrees = 0.08F;
    weapon.recoilYawPerShotDegrees = 0.03F;
    weapon.recoilRecoveryDegreesPerSecond = 9.0F;
    weapon.viewKickDegrees = 0.30F;
    return weapon;
}

void testPrototypeRegistryBuildsEffectiveWeapon() {
    nemisis::weapons::AttachmentRegistry registry;
    registry.registerPrototypeAttachments();
    auto loadout = nemisis::weapons::defaultPrototypeLoadout("ar_01");
    const auto base = makeBaseWeapon();

    const auto summary = nemisis::weapons::buildWeaponWithAttachments(base, loadout, registry);

    expect(registry.attachmentCount() >= 8U, "prototype attachment registry has multiple options");
    expect(summary.effectiveWeapon.id == base.id, "effective weapon keeps base id");
    expect(summary.appliedAttachmentIds.size() == nemisis::weapons::kAttachmentSlotCount, "default loadout applies each slot");
    expect(summary.effectiveWeapon.adsTimeSeconds < base.adsTimeSeconds, "default loadout improves ADS handling");
    expect(summary.effectiveWeapon.recoilPitchPerShotDegrees < base.recoilPitchPerShotDegrees, "default loadout improves recoil");
    expect(summary.effectiveMagazineSize == base.magazineSize, "fast mag keeps default capacity");
    expect(summary.mobilityModifier > 0.0F, "default loadout has mobility bias");
}

void testMagazineAndSlotCycling() {
    nemisis::weapons::AttachmentRegistry registry;
    registry.registerPrototypeAttachments();
    auto loadout = nemisis::weapons::defaultPrototypeLoadout("ar_01");
    nemisis::weapons::setAttachment(loadout, nemisis::weapons::AttachmentSlot::Magazine, "mag_ext_45_01");

    const auto summary = nemisis::weapons::buildWeaponWithAttachments(makeBaseWeapon(), loadout, registry);

    expect(summary.effectiveMagazineSize == 42U, "extended magazine adds capacity");
    expect(nemisis::weapons::nextAttachmentSlot(nemisis::weapons::AttachmentSlot::Stock) == nemisis::weapons::AttachmentSlot::Optic, "next slot wraps");
    expect(nemisis::weapons::previousAttachmentSlot(nemisis::weapons::AttachmentSlot::Optic) == nemisis::weapons::AttachmentSlot::Stock, "previous slot wraps");
    expect(!registry.attachmentsForSlot(nemisis::weapons::AttachmentSlot::Barrel).empty(), "barrel slot has options");
}

void testInvalidAttachmentIsIgnored() {
    nemisis::weapons::AttachmentRegistry registry;
    registry.registerPrototypeAttachments();
    auto loadout = nemisis::weapons::defaultPrototypeLoadout("ar_01");
    nemisis::weapons::setAttachment(loadout, nemisis::weapons::AttachmentSlot::Optic, "missing_optic");

    const auto summary = nemisis::weapons::buildWeaponWithAttachments(makeBaseWeapon(), loadout, registry);

    expect(summary.appliedAttachmentIds.size() == nemisis::weapons::kAttachmentSlotCount - 1U, "missing attachment is ignored");
    expect(summary.effectiveWeapon.adsTimeSeconds > 0.0F, "invalid attachment cannot break effective weapon");
}

} // namespace

int main() {
    testPrototypeRegistryBuildsEffectiveWeapon();
    testMagazineAndSlotCycling();
    testInvalidAttachmentIsIgnored();

    if (failures > 0) {
        std::cerr << failures << " weapon attachment test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis weapon attachment tests passed\n";
    return EXIT_SUCCESS;
}

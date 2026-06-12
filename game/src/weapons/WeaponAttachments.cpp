#include "nemisis/weapons/WeaponAttachments.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace nemisis::weapons {

namespace {

[[nodiscard]] float clampedScale(float value) {
    return std::clamp(value, 0.15F, 4.0F);
}

[[nodiscard]] std::uint16_t adjustedMagazineSize(std::uint16_t base, int delta) {
    const int adjusted = std::clamp(static_cast<int>(base) + delta, 1, 255);
    return static_cast<std::uint16_t>(adjusted);
}

AttachmentDefinition attachment(
    std::string id,
    std::string displayName,
    AttachmentSlot slot,
    std::string description,
    std::string icon,
    AttachmentModifier modifier) {
    AttachmentDefinition definition{};
    definition.id = std::move(id);
    definition.displayName = std::move(displayName);
    definition.slot = slot;
    definition.shortDescription = std::move(description);
    definition.iconAssetId = std::move(icon);
    definition.modifier = modifier;
    return definition;
}

void applyModifier(WeaponDefinition& weapon, const AttachmentModifier& modifier) {
    weapon.adsTimeSeconds *= clampedScale(modifier.adsTimeScale);
    weapon.reloadTimeSeconds *= clampedScale(modifier.reloadTimeScale);
    weapon.hipSpreadDegrees *= clampedScale(modifier.hipSpreadScale);
    weapon.adsSpreadDegrees *= clampedScale(modifier.adsSpreadScale);
    weapon.recoilPitchPerShotDegrees *= clampedScale(modifier.recoilPitchScale);
    weapon.recoilYawPerShotDegrees *= clampedScale(modifier.recoilYawScale);
    weapon.recoilRecoveryDegreesPerSecond *= clampedScale(modifier.recoilRecoveryScale);
    weapon.hipMovementSpreadPerMeter *= clampedScale(modifier.movementSpreadScale);
    weapon.adsMovementSpreadPerMeter *= clampedScale(modifier.movementSpreadScale);
    weapon.maxRangeMeters *= clampedScale(modifier.maxRangeScale);
    weapon.viewKickDegrees *= clampedScale(modifier.viewKickScale);
    weapon.magazineSize = adjustedMagazineSize(weapon.magazineSize, modifier.magazineSizeDelta);
}

} // namespace

void AttachmentRegistry::clear() {
    attachments_.clear();
}

void AttachmentRegistry::registerAttachment(AttachmentDefinition definition) {
    attachments_.insert_or_assign(definition.id, std::move(definition));
}

void AttachmentRegistry::registerPrototypeAttachments() {
    clear();

    registerAttachment(attachment(
        "optic_reflex_01",
        "Reflex Micro",
        AttachmentSlot::Optic,
        "clean sight picture, faster target pickup",
        "ui_icon_optic_reflex",
        AttachmentModifier{
            0.97F,
            1.0F,
            1.0F,
            0.88F,
            1.0F,
            1.0F,
            1.0F,
            0.95F,
            1.0F,
            0.96F,
            0,
            0.02F,
        }));

    registerAttachment(attachment(
        "optic_holo_01",
        "Holo Combat",
        AttachmentSlot::Optic,
        "stable mid-range frame with slower ADS",
        "ui_icon_optic_holo",
        AttachmentModifier{
            1.05F,
            1.0F,
            1.0F,
            0.80F,
            0.98F,
            0.98F,
            1.02F,
            0.96F,
            1.0F,
            0.92F,
            0,
            -0.01F,
        }));

    registerAttachment(attachment(
        "barrel_blackout_short_01",
        "8.5 Short Barrel",
        AttachmentSlot::Barrel,
        "compact blackout-style barrel for mobility",
        "ui_icon_barrel_short",
        AttachmentModifier{
            0.90F,
            1.0F,
            1.10F,
            1.05F,
            1.06F,
            1.03F,
            0.96F,
            1.08F,
            0.86F,
            1.06F,
            0,
            0.08F,
        }));

    registerAttachment(attachment(
        "barrel_hammer_16_01",
        "16 Precision Barrel",
        AttachmentSlot::Barrel,
        "rifle-length stability and damage reach",
        "ui_icon_barrel_long",
        AttachmentModifier{
            1.08F,
            1.0F,
            0.94F,
            0.88F,
            0.92F,
            0.94F,
            1.08F,
            0.88F,
            1.16F,
            0.90F,
            0,
            -0.05F,
        }));

    registerAttachment(attachment(
        "muzzle_compact_comp_01",
        "Compact Comp",
        AttachmentSlot::Muzzle,
        "vertical climb control with mild visual kick",
        "ui_icon_muzzle_comp",
        AttachmentModifier{
            1.0F,
            1.0F,
            1.03F,
            0.98F,
            0.82F,
            0.94F,
            1.12F,
            0.96F,
            1.0F,
            0.84F,
            0,
            -0.01F,
        }));

    registerAttachment(attachment(
        "underbarrel_angled_grip_01",
        "Angled Grip",
        AttachmentSlot::Underbarrel,
        "ADS handling and lateral recoil control",
        "ui_icon_underbarrel_angled",
        AttachmentModifier{
            0.92F,
            1.0F,
            0.98F,
            0.95F,
            0.97F,
            0.82F,
            1.05F,
            0.92F,
            1.0F,
            0.96F,
            0,
            0.03F,
        }));

    registerAttachment(attachment(
        "mag_fast_30_01",
        "Fast Mag",
        AttachmentSlot::Magazine,
        "standard capacity with quicker reloads",
        "ui_icon_mag_fast",
        AttachmentModifier{
            1.0F,
            0.84F,
            1.0F,
            1.0F,
            1.0F,
            1.0F,
            1.0F,
            1.0F,
            1.0F,
            1.0F,
            0,
            0.01F,
        }));

    registerAttachment(attachment(
        "mag_ext_45_01",
        "Extended Mag",
        AttachmentSlot::Magazine,
        "larger magazine, slower reload, heavier handling",
        "ui_icon_mag_ext",
        AttachmentModifier{
            1.04F,
            1.16F,
            1.02F,
            1.0F,
            1.0F,
            1.0F,
            1.0F,
            1.04F,
            1.0F,
            1.0F,
            12,
            -0.05F,
        }));

    registerAttachment(attachment(
        "stock_pilot_wire_01",
        "Pilot Wire Stock",
        AttachmentSlot::Stock,
        "fast ready-up and high movement readability",
        "ui_icon_stock_wire",
        AttachmentModifier{
            0.90F,
            1.0F,
            1.06F,
            1.02F,
            1.04F,
            1.02F,
            0.98F,
            1.05F,
            0.98F,
            1.04F,
            0,
            0.07F,
        }));
}

const AttachmentDefinition* AttachmentRegistry::find(std::string_view id) const {
    auto it = attachments_.find(std::string(id));
    if (it == attachments_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::vector<const AttachmentDefinition*> AttachmentRegistry::attachmentsForSlot(AttachmentSlot slot) const {
    std::vector<const AttachmentDefinition*> result;
    for (const auto& [_, attachment] : attachments_) {
        if (attachment.slot == slot) {
            result.push_back(&attachment);
        }
    }
    std::sort(result.begin(), result.end(), [](const AttachmentDefinition* lhs, const AttachmentDefinition* rhs) {
        return lhs->displayName < rhs->displayName;
    });
    return result;
}

std::size_t AttachmentRegistry::attachmentCount() const {
    return attachments_.size();
}

std::string_view attachmentSlotName(AttachmentSlot slot) {
    switch (slot) {
    case AttachmentSlot::Optic:
        return "Optic";
    case AttachmentSlot::Barrel:
        return "Barrel";
    case AttachmentSlot::Muzzle:
        return "Muzzle";
    case AttachmentSlot::Underbarrel:
        return "Underbarrel";
    case AttachmentSlot::Magazine:
        return "Magazine";
    case AttachmentSlot::Stock:
        return "Stock";
    case AttachmentSlot::Count:
        break;
    }
    return "Unknown";
}

AttachmentSlot nextAttachmentSlot(AttachmentSlot slot) {
    const auto index = (attachmentSlotIndex(slot) + 1U) % kAttachmentSlotCount;
    return static_cast<AttachmentSlot>(index);
}

AttachmentSlot previousAttachmentSlot(AttachmentSlot slot) {
    const auto current = attachmentSlotIndex(slot);
    const auto index = current == 0U ? kAttachmentSlotCount - 1U : current - 1U;
    return static_cast<AttachmentSlot>(index);
}

std::size_t attachmentSlotIndex(AttachmentSlot slot) {
    const auto index = static_cast<std::size_t>(slot);
    return index >= kAttachmentSlotCount ? 0U : index;
}

void setAttachment(WeaponLoadout& loadout, AttachmentSlot slot, std::string attachmentId) {
    loadout.attachmentIds[attachmentSlotIndex(slot)] = std::move(attachmentId);
}

std::string_view selectedAttachmentId(const WeaponLoadout& loadout, AttachmentSlot slot) {
    return loadout.attachmentIds[attachmentSlotIndex(slot)];
}

AttachmentBuildSummary buildWeaponWithAttachments(
    const WeaponDefinition& baseWeapon,
    const WeaponLoadout& loadout,
    const AttachmentRegistry& registry) {
    AttachmentBuildSummary summary{};
    summary.effectiveWeapon = baseWeapon;

    for (const auto& id : loadout.attachmentIds) {
        if (id.empty()) {
            continue;
        }

        const auto* attachment = registry.find(id);
        if (attachment == nullptr) {
            continue;
        }

        applyModifier(summary.effectiveWeapon, attachment->modifier);
        summary.mobilityModifier += attachment->modifier.mobilityDelta;
        summary.appliedAttachmentIds.push_back(attachment->id);
    }

    summary.effectiveWeapon.adsTimeSeconds = std::max(0.05F, summary.effectiveWeapon.adsTimeSeconds);
    summary.effectiveWeapon.reloadTimeSeconds = std::max(0.25F, summary.effectiveWeapon.reloadTimeSeconds);
    summary.effectiveWeapon.hipSpreadDegrees = std::max(0.02F, summary.effectiveWeapon.hipSpreadDegrees);
    summary.effectiveWeapon.adsSpreadDegrees = std::max(0.01F, summary.effectiveWeapon.adsSpreadDegrees);
    summary.effectiveWeapon.recoilPitchPerShotDegrees = std::max(0.005F, summary.effectiveWeapon.recoilPitchPerShotDegrees);
    summary.effectiveWeapon.recoilYawPerShotDegrees = std::max(0.002F, summary.effectiveWeapon.recoilYawPerShotDegrees);
    summary.mobilityModifier = std::clamp(summary.mobilityModifier, -0.25F, 0.25F);
    summary.effectiveMagazineSize = summary.effectiveWeapon.magazineSize;
    return summary;
}

WeaponLoadout defaultPrototypeLoadout(std::string weaponId) {
    WeaponLoadout loadout{};
    loadout.weaponId = std::move(weaponId);
    setAttachment(loadout, AttachmentSlot::Optic, "optic_reflex_01");
    setAttachment(loadout, AttachmentSlot::Barrel, "barrel_blackout_short_01");
    setAttachment(loadout, AttachmentSlot::Muzzle, "muzzle_compact_comp_01");
    setAttachment(loadout, AttachmentSlot::Underbarrel, "underbarrel_angled_grip_01");
    setAttachment(loadout, AttachmentSlot::Magazine, "mag_fast_30_01");
    setAttachment(loadout, AttachmentSlot::Stock, "stock_pilot_wire_01");
    return loadout;
}

} // namespace nemisis::weapons

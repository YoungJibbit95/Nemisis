#pragma once

#include "nemisis/weapons/WeaponTypes.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nemisis::weapons {

enum class AttachmentSlot : std::size_t {
    Optic = 0,
    Barrel,
    Muzzle,
    Underbarrel,
    Magazine,
    Stock,
    Count
};

inline constexpr std::size_t kAttachmentSlotCount = static_cast<std::size_t>(AttachmentSlot::Count);

struct AttachmentModifier final {
    float adsTimeScale = 1.0F;
    float reloadTimeScale = 1.0F;
    float hipSpreadScale = 1.0F;
    float adsSpreadScale = 1.0F;
    float recoilPitchScale = 1.0F;
    float recoilYawScale = 1.0F;
    float recoilRecoveryScale = 1.0F;
    float movementSpreadScale = 1.0F;
    float maxRangeScale = 1.0F;
    float viewKickScale = 1.0F;
    int magazineSizeDelta = 0;
    float mobilityDelta = 0.0F;
};

struct AttachmentDefinition final {
    std::string id;
    std::string displayName;
    AttachmentSlot slot = AttachmentSlot::Optic;
    std::string shortDescription;
    std::string iconAssetId;
    AttachmentModifier modifier{};
};

struct WeaponLoadout final {
    std::string weaponId = "ar_01";
    std::array<std::string, kAttachmentSlotCount> attachmentIds{};
};

struct AttachmentBuildSummary final {
    WeaponDefinition effectiveWeapon{};
    std::vector<std::string> appliedAttachmentIds;
    float mobilityModifier = 0.0F;
    std::uint16_t effectiveMagazineSize = 0;
};

class AttachmentRegistry final {
public:
    void clear();
    void registerAttachment(AttachmentDefinition definition);
    void registerPrototypeAttachments();

    [[nodiscard]] const AttachmentDefinition* find(std::string_view id) const;
    [[nodiscard]] std::vector<const AttachmentDefinition*> attachmentsForSlot(AttachmentSlot slot) const;
    [[nodiscard]] std::size_t attachmentCount() const;

private:
    std::unordered_map<std::string, AttachmentDefinition> attachments_;
};

[[nodiscard]] std::string_view attachmentSlotName(AttachmentSlot slot);
[[nodiscard]] AttachmentSlot nextAttachmentSlot(AttachmentSlot slot);
[[nodiscard]] AttachmentSlot previousAttachmentSlot(AttachmentSlot slot);
[[nodiscard]] std::size_t attachmentSlotIndex(AttachmentSlot slot);

void setAttachment(WeaponLoadout& loadout, AttachmentSlot slot, std::string attachmentId);
[[nodiscard]] std::string_view selectedAttachmentId(const WeaponLoadout& loadout, AttachmentSlot slot);

[[nodiscard]] AttachmentBuildSummary buildWeaponWithAttachments(
    const WeaponDefinition& baseWeapon,
    const WeaponLoadout& loadout,
    const AttachmentRegistry& registry);

[[nodiscard]] WeaponLoadout defaultPrototypeLoadout(std::string weaponId = "ar_01");

} // namespace nemisis::weapons

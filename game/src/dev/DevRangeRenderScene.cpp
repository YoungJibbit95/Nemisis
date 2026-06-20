#include "nemisis/dev/DevRangeRenderScene.hpp"

#include "nemisis/player/PlayerFirstPersonRig.hpp"
#include "nemisis/player/PlayerView.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

namespace nemisis::dev {

namespace {

constexpr std::array<float, 4> kArenaTint{0.46F, 0.54F, 0.55F, 1.0F};
constexpr std::array<float, 4> kActiveTargetTint{0.78F, 0.26F, 0.20F, 1.0F};
constexpr std::array<float, 4> kEliminatedTargetTint{0.20F, 0.20F, 0.20F, 1.0F};
constexpr std::array<float, 4> kDummyTargetTint{0.90F, 0.42F, 0.18F, 1.0F};
constexpr std::array<float, 4> kHumanoidTint{0.28F, 0.74F, 0.90F, 1.0F};
constexpr std::array<float, 4> kFloorTint{0.32F, 0.38F, 0.40F, 1.0F};
constexpr std::array<float, 4> kWallTint{0.36F, 0.44F, 0.47F, 1.0F};
constexpr std::array<float, 4> kCoverTint{0.42F, 0.48F, 0.45F, 1.0F};
constexpr std::array<float, 4> kRampTint{0.30F, 0.47F, 0.43F, 1.0F};
constexpr std::array<float, 4> kWeaponMeshTint{0.56F, 0.58F, 0.54F, 1.0F};
constexpr std::array<float, 4> kSmgMeshTint{0.42F, 0.54F, 0.58F, 1.0F};
constexpr std::array<float, 4> kSidearmMeshTint{0.58F, 0.54F, 0.48F, 1.0F};
constexpr std::array<float, 4> kArmsTint{0.48F, 0.52F, 0.48F, 1.0F};
constexpr std::array<float, 4> kAssetStageBackboardTint{0.05F, 0.28F, 0.34F, 0.78F};
constexpr std::array<float, 4> kAssetStagePlinthTint{0.18F, 0.22F, 0.23F, 1.0F};

struct StaticMeshPlacement final {
    std::string_view assetId;
    novacore::math::Vec3 position;
    novacore::math::Vec3 scale;
    float yawDegrees = 0.0F;
    std::array<float, 4> color;
    float pitchDegrees = 0.0F;
    float rollDegrees = 0.0F;
};

struct WeaponPickupPlacement final {
    novacore::math::Vec3 position;
    std::array<float, 4> color;
};

struct FirstPersonWeaponMount final {
    std::string_view assetId;
    std::string_view fallbackAssetId;
    novacore::math::Vec3 hipOffset;
    novacore::math::Vec3 adsOffset;
    novacore::math::Vec3 scale;
    float yawCorrectionDegrees = 0.0F;
    float pitchCorrectionDegrees = 0.0F;
    float rollCorrectionDegrees = 0.0F;
    float hipPitchFollow = 0.42F;
    float adsPitchFollow = 0.82F;
    float recoilYawScale = 0.50F;
    float recoilPitchScale = 0.38F;
    float reloadPitchDegrees = 8.0F;
    float reloadRollDegrees = 8.0F;
    std::array<float, 4> color;
};

struct FirstPersonArmMount final {
    std::string_view assetId;
    novacore::math::Vec3 hipOffset;
    novacore::math::Vec3 adsOffset;
    novacore::math::Vec3 scale;
    float yawCorrectionDegrees = 0.0F;
    float pitchCorrectionDegrees = 0.0F;
    float rollCorrectionDegrees = 0.0F;
    std::array<float, 4> color;
};

struct FirstPersonBodyMount final {
    std::string_view assetId;
    std::string_view fallbackAssetId;
    novacore::math::Vec3 hipOffset;
    novacore::math::Vec3 adsOffset;
    novacore::math::Vec3 scale;
    std::array<float, 4> color;
};

[[nodiscard]] float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

[[nodiscard]] float degreesToRadians(float degrees) {
    constexpr float kPi = 3.14159265358979323846F;
    return degrees * (kPi / 180.0F);
}

[[nodiscard]] novacore::math::Vec3 rotateZ(novacore::math::Vec3 value, float rollDegrees) {
    const float r = degreesToRadians(rollDegrees);
    const float s = std::sin(r);
    const float c = std::cos(r);
    return {
        (value.x * c) - (value.y * s),
        (value.x * s) + (value.y * c),
        value.z,
    };
}

[[nodiscard]] novacore::math::Vec3 rotateX(novacore::math::Vec3 value, float pitchDegrees) {
    const float p = degreesToRadians(pitchDegrees);
    const float s = std::sin(p);
    const float c = std::cos(p);
    return {
        value.x,
        (value.y * c) - (value.z * s),
        (value.y * s) + (value.z * c),
    };
}

[[nodiscard]] novacore::math::Vec3 rotateY(novacore::math::Vec3 value, float yawDegrees) {
    const float y = degreesToRadians(yawDegrees);
    const float s = std::sin(y);
    const float c = std::cos(y);
    return {
        (value.x * c) + (value.z * s),
        value.y,
        (-value.x * s) + (value.z * c),
    };
}

[[nodiscard]] novacore::math::Vec3 mulVec3(
    novacore::math::Vec3 a,
    novacore::math::Vec3 b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

[[nodiscard]] novacore::math::Vec3 transformMeshLocalPoint(
    novacore::math::Vec3 local,
    novacore::math::Vec3 scale,
    float yawDegrees,
    float pitchDegrees,
    float rollDegrees) {
    return rotateY(rotateX(rotateZ(mulVec3(local, scale), rollDegrees), pitchDegrees), yawDegrees);
}

[[nodiscard]] std::optional<novacore::math::Vec3> cookedSocketLocalPosition(
    const DevRangeRenderSceneDesc& desc,
    std::string_view assetId,
    std::string_view socketName) {
    if (desc.meshCatalog == nullptr) {
        return std::nullopt;
    }
    const auto* source = desc.meshCatalog->findByAssetId(assetId);
    if (source == nullptr || !source->meshData.has_value()) {
        return std::nullopt;
    }
    for (const auto& marker : source->meshData->nodeMarkers) {
        if (marker.name == socketName) {
            return marker.worldPosition;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::size_t cookedSocketCount(
    const DevRangeRenderSceneDesc& desc,
    std::string_view assetId) {
    if (desc.meshCatalog == nullptr) {
        return 0U;
    }
    const auto* source = desc.meshCatalog->findByAssetId(assetId);
    if (source == nullptr || !source->meshData.has_value()) {
        return 0U;
    }
    return static_cast<std::size_t>(std::count_if(
        source->meshData->nodeMarkers.begin(),
        source->meshData->nodeMarkers.end(),
        [](const novacore::assets::GltfNodeMarker& marker) {
            return marker.name.size() >= 7U && marker.name.compare(0U, 7U, "socket_") == 0;
        }));
}

[[nodiscard]] novacore::math::Vec3 alignMeshPositionToSocket(
    novacore::math::Vec3 targetSocketPosition,
    novacore::math::Vec3 localSocketPosition,
    novacore::math::Vec3 scale,
    float yawDegrees,
    float pitchDegrees,
    float rollDegrees) {
    return targetSocketPosition - transformMeshLocalPoint(
        localSocketPosition,
        scale,
        yawDegrees,
        pitchDegrees,
        rollDegrees);
}

[[nodiscard]] float easeOut01(float value) {
    value = clamp01(value);
    return 1.0F - ((1.0F - value) * (1.0F - value));
}

[[nodiscard]] std::array<float, 4> contactColor(GreyboxContactRole role) {
    switch (role) {
    case GreyboxContactRole::Ground:
        return {0.58F, 0.95F, 1.0F, 1.0F};
    case GreyboxContactRole::Step:
        return {0.98F, 0.86F, 0.26F, 1.0F};
    case GreyboxContactRole::Wall:
        return {0.22F, 0.80F, 1.0F, 1.0F};
    case GreyboxContactRole::Bounds:
        return {1.0F, 0.24F, 0.20F, 1.0F};
    case GreyboxContactRole::Sweep:
        return {1.0F, 0.64F, 0.18F, 1.0F};
    }
    return {0.80F, 0.80F, 0.80F, 1.0F};
}

[[nodiscard]] bool startsWith(std::string_view value, std::string_view prefix) {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] novacore::render::RenderMaterialFallback materialFallbackForAsset(std::string_view assetId) {
    if (startsWith(assetId, "wpn_project_")) {
        return {1.22F, 1.45F, 1.10F, 0.86F};
    }
    if (startsWith(assetId, "wpn_a2_")) {
        return {1.12F, 1.28F, 1.06F, 0.92F};
    }
    if (startsWith(assetId, "wpn_a1_") || startsWith(assetId, "wpn_")) {
        return {1.06F, 1.16F, 1.04F, 0.94F};
    }
    if (startsWith(assetId, "chr_project_")) {
        return {0.86F, 0.52F, 0.95F, 0.82F};
    }
    if (startsWith(assetId, "chr_a2_") || startsWith(assetId, "chr_a1_") || startsWith(assetId, "chr_")) {
        return {0.92F, 0.62F, 0.98F, 0.88F};
    }
    if (startsWith(assetId, "env_") || startsWith(assetId, "map_") || startsWith(assetId, "prop_")) {
        return {0.78F, 0.42F, 0.92F, 0.74F};
    }
    return {};
}

[[nodiscard]] bool usesMaterialFallbackProfile(const novacore::render::RenderMaterialFallback& material) {
    return material.rimScale != 1.0F ||
        material.specularScale != 1.0F ||
        material.contrastScale != 1.0F ||
        material.saturationScale != 1.0F;
}

[[nodiscard]] bool targetEliminated(const DevRangeRenderSceneDesc& desc) {
    const auto* lane = desc.targetRange == nullptr ? nullptr : activeTargetLane(*desc.targetRange);
    return lane != nullptr && lane->target.eliminated;
}

[[nodiscard]] bool hasTargetRange(const DevRangeRenderSceneDesc& desc) {
    return desc.targetRange != nullptr && !desc.targetRange->lanes.empty();
}

[[nodiscard]] novacore::math::Vec3 playerEyePosition(const DevRangeRenderSceneDesc& desc) {
    if (desc.player.hasCameraRig) {
        return desc.player.cameraPosition;
    }
    return desc.player.position + novacore::math::Vec3{0.0F, 1.65F, 0.0F};
}

[[nodiscard]] player::PlayerViewComponent renderView(const DevRangeRenderSceneDesc& desc) {
    return desc.player.hasCameraRig ? desc.player.cameraView : desc.player.view;
}

[[nodiscard]] const FirstPersonWeaponMount& firstPersonWeaponMount(std::string_view weaponId) {
    static constexpr FirstPersonWeaponMount kPrimary{
        "wpn_project_rifle_m4a1",
        "wpn_a2_modular_rifle_01",
        {0.26F, -0.30F, 0.90F},
        {0.035F, -0.165F, 0.72F},
        {1.0F, 1.0F, 1.0F},
        0.0F,
        0.0F,
        0.0F,
        0.38F,
        0.78F,
        0.46F,
        0.34F,
        9.5F,
        9.0F,
        kWeaponMeshTint,
    };
    static constexpr FirstPersonWeaponMount kSmg{
        "wpn_project_smg_fr17",
        "wpn_a2_blackout_carbine_01",
        {0.22F, -0.285F, 0.82F},
        {0.028F, -0.155F, 0.66F},
        {1.15F, 1.15F, 1.15F},
        0.0F,
        0.0F,
        0.0F,
        0.38F,
        0.78F,
        0.50F,
        0.36F,
        8.0F,
        8.0F,
        kSmgMeshTint,
    };
    static constexpr FirstPersonWeaponMount kShotgun{
        "wpn_project_rifle_ncar",
        "wpn_a2_modular_rifle_01",
        {0.27F, -0.315F, 0.94F},
        {0.04F, -0.18F, 0.75F},
        {0.92F, 0.92F, 0.92F},
        0.0F,
        0.0F,
        0.0F,
        0.36F,
        0.78F,
        0.44F,
        0.32F,
        10.0F,
        10.0F,
        kWeaponMeshTint,
    };
    static constexpr FirstPersonWeaponMount kSidearm{
        "wpn_project_sidearm_glock19",
        "wpn_a2_striker_sidearm_01",
        {0.18F, -0.245F, 0.68F},
        {0.020F, -0.125F, 0.54F},
        {1.35F, 1.35F, 1.35F},
        0.0F,
        0.0F,
        0.0F,
        0.48F,
        0.88F,
        0.58F,
        0.42F,
        6.5F,
        5.5F,
        kSidearmMeshTint,
    };

    if (weaponId == "sidearm_01") {
        return kSidearm;
    }
    if (weaponId == "smg_01") {
        return kSmg;
    }
    if (weaponId == "shotgun_01") {
        return kShotgun;
    }
    return kPrimary;
}

[[nodiscard]] const FirstPersonArmMount& firstPersonArmMount(bool adsAligned) {
    static constexpr FirstPersonArmMount kA1Arms{
        "chr_a1_fp_arms_01",
        {0.08F, -0.50F, 0.78F},
        {0.018F, -0.43F, 0.62F},
        {0.84F, 0.84F, 0.84F},
        0.0F,
        0.0F,
        0.0F,
        kArmsTint,
    };
    static constexpr FirstPersonArmMount kCharacterProxyArms{
        "chr_a1_fp_arms_01",
        {0.12F, -0.52F, 0.82F},
        {0.035F, -0.45F, 0.66F},
        {0.90F, 0.90F, 0.90F},
        0.0F,
        0.0F,
        0.0F,
        {0.52F, 0.58F, 0.54F, 0.88F},
    };
    return adsAligned ? kA1Arms : kCharacterProxyArms;
}

[[nodiscard]] const FirstPersonBodyMount& firstPersonBodyMount() {
    static constexpr FirstPersonBodyMount kBody{
        "chr_project_male1",
        "chr_a2_pilot_operator_01",
        {0.0F, -2.34F, 0.30F},
        {0.0F, -2.38F, 0.22F},
        {0.46F, 0.46F, 0.46F},
        {0.46F, 0.54F, 0.50F, 0.42F},
    };
    return kBody;
}

[[nodiscard]] std::array<float, 4> withAlpha(
    std::array<float, 4> color,
    float alphaScale) {
    color[3] *= clamp01(alphaScale);
    return color;
}

[[nodiscard]] player::FirstPersonRigMountDesc toRigMount(const FirstPersonWeaponMount& mount) {
    return player::FirstPersonRigMountDesc{
        mount.hipOffset,
        mount.adsOffset,
        mount.scale,
        mount.yawCorrectionDegrees,
        mount.pitchCorrectionDegrees,
        mount.rollCorrectionDegrees,
        mount.hipPitchFollow,
        mount.adsPitchFollow,
        mount.recoilYawScale,
        mount.recoilPitchScale,
        mount.reloadPitchDegrees,
        mount.reloadRollDegrees,
    };
}

[[nodiscard]] player::FirstPersonRigMountDesc toRigMount(const FirstPersonArmMount& mount) {
    player::FirstPersonRigMountDesc result{};
    result.hipOffset = mount.hipOffset;
    result.adsOffset = mount.adsOffset;
    result.scale = mount.scale;
    result.yawCorrectionDegrees = mount.yawCorrectionDegrees;
    result.pitchCorrectionDegrees = mount.pitchCorrectionDegrees;
    result.rollCorrectionDegrees = mount.rollCorrectionDegrees;
    result.hipPitchFollow = 0.46F;
    result.adsPitchFollow = 0.58F;
    return result;
}

[[nodiscard]] player::FirstPersonRigMountDesc toRigMount(const FirstPersonBodyMount& mount) {
    player::FirstPersonRigMountDesc result{};
    result.hipOffset = mount.hipOffset;
    result.adsOffset = mount.adsOffset;
    result.scale = mount.scale;
    result.hipPitchFollow = 0.12F;
    result.adsPitchFollow = 0.10F;
    return result;
}

[[nodiscard]] std::array<StaticMeshPlacement, 24> staticShowcaseMeshes() {
    return {
        StaticMeshPlacement{
            "env_test_arena_kit_01",
            {0.0F, 0.0F, 0.0F},
            {1.0F, 1.0F, 1.0F},
            0.0F,
            kArenaTint,
        },
        StaticMeshPlacement{
            "chr_a1_stylized_operator_01",
            {-8.0F, 0.0F, 9.5F},
            {1.0F, 1.0F, 1.0F},
            135.0F,
            {0.30F, 0.55F, 0.62F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_a1_modern_rifle_01",
            {8.0F, 0.85F, 9.5F},
            {0.82F, 0.82F, 0.82F},
            -35.0F,
            {0.16F, 0.20F, 0.19F, 1.0F},
        },
        StaticMeshPlacement{
            "chr_proto_humanoid_01",
            {-3.2F, 0.0F, 14.0F},
            {1.0F, 1.0F, 1.0F},
            180.0F,
            kHumanoidTint,
        },
        StaticMeshPlacement{
            "map_floor_tile_01",
            {-7.0F, 0.0F, -4.0F},
            {1.0F, 1.0F, 1.0F},
            0.0F,
            kFloorTint,
        },
        StaticMeshPlacement{
            "map_wall_panel_01",
            {-10.5F, 0.0F, 10.0F},
            {1.0F, 1.0F, 1.0F},
            90.0F,
            kWallTint,
        },
        StaticMeshPlacement{
            "map_cover_crate_01",
            {-4.2F, 0.0F, 2.0F},
            {1.0F, 1.0F, 1.0F},
            0.0F,
            kCoverTint,
        },
        StaticMeshPlacement{
            "map_ramp_01",
            {4.0F, 0.0F, -2.5F},
            {1.0F, 1.0F, 1.0F},
            0.0F,
            kRampTint,
        },
        StaticMeshPlacement{
            "map_target_stand_01",
            {3.2F, 0.0F, 14.0F},
            {1.0F, 1.0F, 1.0F},
            180.0F,
            kDummyTargetTint,
        },
        StaticMeshPlacement{
            "chr_a2_pilot_operator_01",
            {-2.75F, 0.0F, -5.15F},
            {1.12F, 1.12F, 1.12F},
            180.0F,
            {0.46F, 0.78F, 0.84F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_a2_blackout_carbine_01",
            {-1.25F, 1.18F, -6.65F},
            {1.22F, 1.22F, 1.22F},
            90.0F,
            {0.68F, 0.70F, 0.64F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_a2_modular_rifle_01",
            {0.95F, 1.20F, -6.65F},
            {1.12F, 1.12F, 1.12F},
            90.0F,
            {0.56F, 0.66F, 0.70F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_a2_striker_sidearm_01",
            {2.55F, 1.08F, -6.72F},
            {1.46F, 1.46F, 1.46F},
            90.0F,
            {0.70F, 0.64F, 0.54F, 1.0F},
        },
        StaticMeshPlacement{
            "map_a2_wallrun_panel_01",
            {5.35F, 0.0F, -5.20F},
            {1.0F, 1.0F, 1.0F},
            90.0F,
            {0.20F, 0.78F, 0.88F, 1.0F},
        },
        StaticMeshPlacement{
            "map_a2_slide_ramp_01",
            {-5.65F, 0.0F, -5.80F},
            {1.0F, 1.0F, 1.0F},
            0.0F,
            {0.32F, 0.74F, 0.62F, 1.0F},
        },
        StaticMeshPlacement{
            "map_a2_cover_crate_01",
            {3.60F, 0.0F, -3.95F},
            {0.88F, 0.88F, 0.88F},
            -12.0F,
            {0.56F, 0.62F, 0.56F, 1.0F},
        },
        StaticMeshPlacement{
            "prop_a2_range_hero_01",
            {0.0F, 0.0F, -3.65F},
            {1.08F, 1.08F, 1.08F},
            180.0F,
            {0.95F, 0.50F, 0.20F, 1.0F},
        },
        StaticMeshPlacement{
            "chr_project_male1",
            {-7.55F, 0.0F, -6.75F},
            {0.92F, 0.92F, 0.92F},
            180.0F,
            {0.72F, 0.76F, 0.72F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_rifle_m4a1",
            {-4.95F, 1.02F, -7.78F},
            {1.00F, 1.00F, 1.00F},
            90.0F,
            {0.62F, 0.64F, 0.58F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_rifle_afr120",
            {-2.75F, 1.05F, -7.88F},
            {0.70F, 0.70F, 0.70F},
            90.0F,
            {0.54F, 0.64F, 0.72F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_rifle_ncar",
            {-0.55F, 1.04F, -7.86F},
            {0.88F, 0.88F, 0.88F},
            90.0F,
            {0.56F, 0.60F, 0.66F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_smg_fr17",
            {1.45F, 1.00F, -7.82F},
            {1.15F, 1.15F, 1.15F},
            90.0F,
            {0.46F, 0.62F, 0.68F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_sidearm_glock19",
            {3.15F, 0.92F, -7.86F},
            {1.35F, 1.35F, 1.35F},
            90.0F,
            {0.66F, 0.61F, 0.54F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_sidearm_p320",
            {4.55F, 0.92F, -7.86F},
            {1.30F, 1.30F, 1.30F},
            90.0F,
            {0.68F, 0.60F, 0.52F, 1.0F},
        },
    };
}

void appendBox(
    novacore::render::RenderFrameInfo& frame,
    novacore::math::Vec3 center,
    novacore::math::Vec3 halfExtents,
    std::array<float, 4> color,
    DevRangeRenderSceneStats& stats) {
    frame.worldBoxes.push_back(novacore::render::RenderBox3D{
        center,
        halfExtents,
        color,
    });
    ++stats.worldBoxCount;
}

[[nodiscard]] novacore::math::Vec3 midpoint(
    novacore::math::Vec3 a,
    novacore::math::Vec3 b) {
    return (a + b) * 0.5F;
}

[[nodiscard]] novacore::math::Vec3 segmentHalfExtents(
    novacore::math::Vec3 a,
    novacore::math::Vec3 b,
    float thickness) {
    const auto delta = b - a;
    return {
        std::max(thickness, std::abs(delta.x) * 0.5F),
        std::max(thickness, std::abs(delta.y) * 0.5F),
        std::max(thickness, std::abs(delta.z) * 0.5F),
    };
}

void appendRigSegmentBox(
    novacore::render::RenderFrameInfo& frame,
    const player::FirstPersonRigFrame& rig,
    player::FirstPersonRigJoint a,
    player::FirstPersonRigJoint b,
    float thickness,
    std::array<float, 4> color,
    DevRangeRenderSceneStats& stats) {
    const auto& jointA = player::firstPersonRigJoint(rig, a);
    const auto& jointB = player::firstPersonRigJoint(rig, b);
    appendBox(
        frame,
        midpoint(jointA.worldPosition, jointB.worldPosition),
        segmentHalfExtents(jointA.worldPosition, jointB.worldPosition, thickness),
        color,
        stats);
    ++stats.firstPersonBodyPrimitiveCount;
}

void appendFirstPersonRigPrimitives(
    novacore::render::RenderFrameInfo& frame,
    const player::FirstPersonRigFrame& rig,
    DevRangeRenderSceneStats& stats) {
    for (const auto& socket : rig.sockets) {
        if (socket.valid) {
            ++stats.firstPersonSocketCount;
        }
    }

    const std::array<float, 4> gloveColor{0.34F, 0.40F, 0.36F, 0.92F};
    const std::array<float, 4> sleeveColor{0.28F, 0.36F, 0.34F, 0.78F};
    const std::array<float, 4> torsoColor{0.24F, 0.32F, 0.30F, 0.18F + (rig.lookDownBodyAlpha * 0.44F)};
    const std::array<float, 4> armorColor{0.18F, 0.25F, 0.24F, 0.14F + (rig.lookDownBodyAlpha * 0.34F)};

    appendRigSegmentBox(
        frame,
        rig,
        player::FirstPersonRigJoint::RightForearm,
        player::FirstPersonRigJoint::RightHand,
        0.040F,
        sleeveColor,
        stats);
    appendRigSegmentBox(
        frame,
        rig,
        player::FirstPersonRigJoint::LeftForearm,
        player::FirstPersonRigJoint::LeftHand,
        0.042F,
        sleeveColor,
        stats);
    appendBox(frame, rig.rightHand.position, {0.052F, 0.042F, 0.070F}, gloveColor, stats);
    ++stats.firstPersonBodyPrimitiveCount;
    appendBox(frame, rig.leftHand.position, {0.055F, 0.044F, 0.075F}, gloveColor, stats);
    ++stats.firstPersonBodyPrimitiveCount;

    if (rig.lookDownBodyAlpha <= 0.03F) {
        return;
    }

    appendRigSegmentBox(
        frame,
        rig,
        player::FirstPersonRigJoint::Pelvis,
        player::FirstPersonRigJoint::Spine,
        0.160F,
        torsoColor,
        stats);
    appendRigSegmentBox(
        frame,
        rig,
        player::FirstPersonRigJoint::Spine,
        player::FirstPersonRigJoint::Chest,
        0.185F,
        torsoColor,
        stats);
    appendBox(
        frame,
        player::firstPersonRigJoint(rig, player::FirstPersonRigJoint::Pelvis).worldPosition,
        {0.245F, 0.105F, 0.145F},
        armorColor,
        stats);
    ++stats.firstPersonBodyPrimitiveCount;
    appendBox(
        frame,
        player::firstPersonRigJoint(rig, player::FirstPersonRigJoint::Chest).worldPosition,
        {0.235F, 0.145F, 0.120F},
        armorColor,
        stats);
    ++stats.firstPersonBodyPrimitiveCount;
}

void appendWeaponFeedbackPrimitives(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    const player::FirstPersonRigFrame& rig,
    DevRangeRenderSceneStats& stats) {
    const bool recentFire = desc.player.firedThisFrame ||
        desc.player.animation.fireAlpha > 0.08F ||
        (desc.player.weapon.shotIndex > 0U && desc.player.weapon.timeSinceLastShotSeconds < 0.075F);
    if (!recentFire && !desc.player.hitThisFrame) {
        return;
    }

    const auto& muzzle = player::firstPersonRigSocket(rig, player::FirstPersonRigSocket::Muzzle);
    const auto& ejection = player::firstPersonRigSocket(rig, player::FirstPersonRigSocket::EjectionPort);
    const auto muzzlePosition = muzzle.valid ? muzzle.worldPosition : rig.muzzle.position;
    const auto flashAlpha = std::max(0.34F, desc.player.animation.fireAlpha);
    if (recentFire) {
        appendBox(
            frame,
            muzzlePosition,
            {0.070F + (0.025F * flashAlpha), 0.050F, 0.110F + (0.040F * flashAlpha)},
            {1.0F, 0.72F, 0.18F, 0.88F},
            stats);
        ++stats.firstPersonFeedbackPrimitiveCount;
        appendBox(
            frame,
            muzzlePosition + novacore::math::Vec3{0.0F, 0.018F, 0.075F},
            {0.028F, 0.024F, 0.150F},
            {1.0F, 0.94F, 0.52F, 0.74F},
            stats);
        ++stats.firstPersonFeedbackPrimitiveCount;
        if (ejection.valid) {
            appendBox(
                frame,
                ejection.worldPosition + novacore::math::Vec3{0.045F, 0.018F, -0.010F},
                {0.026F, 0.012F, 0.010F},
                {0.98F, 0.78F, 0.32F, 0.92F},
                stats);
            ++stats.firstPersonFeedbackPrimitiveCount;
        }
    }

    if (!desc.player.hasLatestShotTrace) {
        return;
    }

    const auto lineColor = desc.player.hitThisFrame
        ? std::array<float, 4>{1.0F, 0.28F, 0.18F, 0.92F}
        : std::array<float, 4>{1.0F, 0.78F, 0.22F, 0.58F};
    frame.worldLines.push_back(novacore::render::RenderLine3D{
        muzzlePosition,
        desc.player.latestShotEnd,
        lineColor,
    });
    ++stats.worldLineCount;
    ++stats.hitFeedbackLineCount;

    if (desc.player.hitThisFrame) {
        appendBox(
            frame,
            desc.player.latestShotEnd,
            desc.player.eliminatedThisFrame
                ? novacore::math::Vec3{0.22F, 0.22F, 0.22F}
                : novacore::math::Vec3{0.14F, 0.14F, 0.14F},
            desc.player.eliminatedThisFrame
                ? std::array<float, 4>{1.0F, 0.36F, 0.18F, 0.96F}
                : std::array<float, 4>{1.0F, 0.84F, 0.24F, 0.90F},
            stats);
        ++stats.firstPersonFeedbackPrimitiveCount;
    }
}

void appendLanePressureVisuals(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    const DevTargetLane& lane,
    DevRangeRenderSceneStats& stats) {
    if (!lane.pressureActive || lane.pressure01 <= 0.02F) {
        return;
    }

    const float pressure = clamp01(lane.pressure01);
    const auto target = lane.target.position;
    const auto pressureColor = std::array<float, 4>{
        0.94F,
        0.18F + (0.30F * (1.0F - pressure)),
        0.10F,
        0.28F + (pressure * 0.42F),
    };
    appendBox(
        frame,
        {target.x, 0.035F, target.z},
        {lane.target.radiusMeters + (pressure * 0.55F), 0.028F, lane.target.radiusMeters + (pressure * 0.55F)},
        pressureColor,
        stats);
    ++stats.activeLanePressurePrimitiveCount;
    appendBox(
        frame,
        target + novacore::math::Vec3{0.0F, 0.18F + (pressure * 0.40F), 0.0F},
        {0.052F + (pressure * 0.038F), 0.24F + (pressure * 0.34F), 0.052F + (pressure * 0.038F)},
        {pressureColor[0], pressureColor[1] * 1.20F, pressureColor[2], pressureColor[3] * 0.72F},
        stats);
    ++stats.activeLanePressurePrimitiveCount;

    frame.worldLines.push_back(novacore::render::RenderLine3D{
        target,
        playerEyePosition(desc),
        {1.0F, 0.20F, 0.14F, 0.30F + (pressure * 0.40F)},
    });
    ++stats.worldLineCount;
    ++stats.activeLanePressurePrimitiveCount;
}

void appendAssetStageGuides(
    novacore::render::RenderFrameInfo& frame,
    DevRangeRenderSceneStats& stats) {
    appendBox(frame, {0.0F, 1.28F, -5.45F}, {4.95F, 1.24F, 0.08F}, kAssetStageBackboardTint, stats);
    appendBox(frame, {-2.75F, 0.08F, -5.15F}, {0.62F, 0.08F, 0.44F}, kAssetStagePlinthTint, stats);
    appendBox(frame, {-1.25F, 0.88F, -6.65F}, {0.70F, 0.08F, 0.38F}, kAssetStagePlinthTint, stats);
    appendBox(frame, {0.95F, 0.90F, -6.65F}, {0.74F, 0.08F, 0.38F}, kAssetStagePlinthTint, stats);
    appendBox(frame, {2.55F, 0.80F, -6.72F}, {0.42F, 0.07F, 0.30F}, kAssetStagePlinthTint, stats);
}

void appendWeaponPickupPads(
    novacore::render::RenderFrameInfo& frame,
    DevRangeRenderSceneStats& stats) {
    static constexpr std::array<WeaponPickupPlacement, 4> kPickups{
        WeaponPickupPlacement{{-4.95F, 0.0F, -7.78F}, {0.34F, 0.62F, 0.70F, 0.74F}},
        WeaponPickupPlacement{{-0.55F, 0.0F, -7.86F}, {0.62F, 0.58F, 0.36F, 0.74F}},
        WeaponPickupPlacement{{1.45F, 0.0F, -7.82F}, {0.26F, 0.72F, 0.84F, 0.74F}},
        WeaponPickupPlacement{{3.15F, 0.0F, -7.86F}, {0.78F, 0.64F, 0.42F, 0.74F}},
    };

    for (const auto& pickup : kPickups) {
        appendBox(frame, pickup.position + novacore::math::Vec3{0.0F, 0.035F, 0.0F}, {0.46F, 0.035F, 0.46F}, pickup.color, stats);
        appendBox(
            frame,
            pickup.position + novacore::math::Vec3{0.0F, 0.24F, 0.0F},
            {0.07F, 0.18F, 0.07F},
            {pickup.color[0] * 1.18F, pickup.color[1] * 1.18F, pickup.color[2] * 1.18F, 0.82F},
            stats);
    }
}

} // namespace

DevRangeRenderSceneStats DevRangeRenderSceneBuilder::append(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc) const {
    DevRangeRenderSceneStats stats{};
    if (desc.greyboxWorld == nullptr || desc.meshResources == nullptr) {
        return stats;
    }

    frame.sky = desc.sky;
    stats.skyPassEnabled = desc.sky.enabled;
    frame.lighting = desc.lighting;
    frame.camera3D.enabled = true;
    frame.camera3D.position = playerEyePosition(desc);
    const auto view = renderView(desc);
    frame.camera3D.yawDegrees = view.yawDegrees;
    frame.camera3D.pitchDegrees = view.pitchDegrees;
    frame.camera3D.verticalFovDegrees = desc.player.hasCameraRig
        ? desc.player.verticalFovDegrees
        : desc.verticalFovDegrees;
    frame.camera3D.nearPlane = desc.nearPlane;
    frame.camera3D.farPlane = desc.farPlane;

    const auto targetLaneCount = desc.targetRange == nullptr ? 0U : desc.targetRange->lanes.size();
    frame.worldBoxes.reserve(frame.worldBoxes.size() + desc.greyboxWorld->primitives.size() + (targetLaneCount * 3U) + 29U);
    frame.worldMeshes.reserve(frame.worldMeshes.size() + 13U + (targetLaneCount * 2U));

    appendSkyboxMesh(frame, desc, stats);
    appendWorldGeometry(frame, desc, stats);
    appendStaticShowcaseMeshes(frame, desc, stats);
    appendTargetLaneMeshes(frame, desc, stats);
    appendLocalPlayerBodyMesh(frame, desc, stats);
    appendFirstPersonMeshes(frame, desc, stats);
    if (desc.showWorldDebugLines) {
        appendMovementTechVisuals(frame, desc, stats);
        appendWorldDebugLines(frame, desc, stats);
    }
    return stats;
}

novacore::render::MeshResourceHandle DevRangeRenderSceneBuilder::findMesh(
    const DevRangeRenderSceneDesc& desc,
    std::string_view assetId) const {
    if (desc.meshResources == nullptr) {
        return {};
    }

    const auto it = desc.meshResources->find(std::string(assetId));
    if (it == desc.meshResources->end()) {
        return {};
    }
    return it->second;
}

bool DevRangeRenderSceneBuilder::appendMesh(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    std::string_view assetId,
    novacore::math::Vec3 position,
    novacore::math::Vec3 scale,
    float yawDegrees,
    std::array<float, 4> color,
    DevRangeRenderSceneStats& stats,
    float pitchDegrees,
    float rollDegrees) const {
    const auto handle = findMesh(desc, assetId);
    if (!handle.isValid()) {
        ++stats.skippedMeshInstanceCount;
        return false;
    }

    const auto material = materialFallbackForAsset(assetId);
    frame.worldMeshes.push_back(novacore::render::RenderMesh3D{
        handle,
        std::string(assetId),
        position,
        scale,
        yawDegrees,
        pitchDegrees,
        rollDegrees,
        color,
        material,
    });
    ++stats.meshInstanceCount;
    if (usesMaterialFallbackProfile(material)) {
        ++stats.materialFallbackProfileCount;
    }
    return true;
}

void DevRangeRenderSceneBuilder::appendWorldGeometry(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    if (desc.greyboxWorld == nullptr) {
        return;
    }

    for (const auto& primitive : desc.greyboxWorld->primitives) {
        if (primitive.kind == GreyboxPrimitiveKind::Target && hasTargetRange(desc)) {
            continue;
        }
        auto color = primitive.color;
        if (primitive.kind == GreyboxPrimitiveKind::Target && targetEliminated(desc)) {
            color = {0.10F, 0.10F, 0.10F, 1.0F};
        }
        appendBox(frame, primitive.center, primitive.halfExtents, color, stats);
    }
}

void DevRangeRenderSceneBuilder::appendSkyboxMesh(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    if (desc.sky.enabled) {
        return;
    }

    const auto eye = playerEyePosition(desc);
    constexpr float kSkyboxScale = 0.16F;
    if (appendMesh(
        frame,
        desc,
        "env_project_skybox1",
        eye + novacore::math::Vec3{0.0F, -8.0F, 0.0F},
        {kSkyboxScale, kSkyboxScale, kSkyboxScale},
        0.0F,
        {0.82F, 0.90F, 1.0F, 1.0F},
        stats)) {
        ++stats.skyFallbackMeshCount;
    }
}

void DevRangeRenderSceneBuilder::appendStaticShowcaseMeshes(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    appendAssetStageGuides(frame, stats);
    appendWeaponPickupPads(frame, stats);
    for (const auto& placement : staticShowcaseMeshes()) {
        (void)appendMesh(
            frame,
            desc,
            placement.assetId,
            placement.position,
            placement.scale,
            placement.yawDegrees,
            placement.color,
            stats,
            placement.pitchDegrees,
            placement.rollDegrees);
    }
}

void DevRangeRenderSceneBuilder::appendTargetLaneMeshes(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    if (desc.targetRange == nullptr) {
        return;
    }

    for (std::size_t index = 0; index < desc.targetRange->lanes.size(); ++index) {
        const auto& lane = desc.targetRange->lanes[index];
        const bool active = index == desc.targetRange->activeLaneIndex;
        const bool eliminated = lane.target.eliminated;
        const auto color = eliminated
            ? kEliminatedTargetTint
            : active
                ? kActiveTargetTint
                : kDummyTargetTint;

        appendBox(
            frame,
            lane.target.position,
            {lane.target.radiusMeters, 0.95F, lane.target.radiusMeters},
            {color[0] * 0.55F, color[1] * 0.55F, color[2] * 0.55F, 0.75F},
            stats);
        appendLanePressureVisuals(frame, desc, lane, stats);

        const auto actorAsset = eliminated ? std::string_view("prop_target_dummy_01") : std::string_view("chr_project_male1");
        if (appendMesh(
                frame,
                desc,
                actorAsset,
                {lane.target.position.x, 0.0F, lane.target.position.z},
                eliminated ? novacore::math::Vec3{0.82F, 0.82F, 0.82F} : novacore::math::Vec3{0.92F, 0.92F, 0.92F},
                180.0F,
                color,
                stats)) {
            ++stats.targetMeshCount;
        } else if (!eliminated && appendMesh(
                frame,
                desc,
                "chr_dev_soldier_a",
                {lane.target.position.x, 0.0F, lane.target.position.z},
                {0.92F, 0.92F, 0.92F},
                180.0F,
                color,
                stats)) {
            ++stats.targetMeshCount;
        }

        (void)appendMesh(
            frame,
            desc,
            "map_target_stand_01",
            {lane.target.position.x, 0.0F, lane.target.position.z - 0.18F},
            {0.72F, 0.72F, 0.72F},
            180.0F,
            {0.24F, 0.28F, 0.28F, 1.0F},
            stats);
    }
}

void DevRangeRenderSceneBuilder::appendLocalPlayerBodyMesh(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    if (!desc.player.hasMovementState || desc.player.hasCameraRig) {
        return;
    }

    const auto view = renderView(desc);
    const auto vectors = player::viewVectors(view);
    const auto animationOffset = desc.player.hasAnimationFrame
        ? desc.player.animation.thirdPersonBodyOffset
        : novacore::math::Vec3{};
    const float animationPitch = desc.player.hasAnimationFrame
        ? desc.player.animation.thirdPersonBodyPitchDegrees
        : 0.0F;
    const float animationRoll = desc.player.hasAnimationFrame
        ? desc.player.animation.thirdPersonBodyRollDegrees
        : 0.0F;
    const auto bodyPosition =
        desc.player.position -
        (vectors.forward * 0.12F) +
        novacore::math::Vec3{0.0F, 0.02F, 0.0F} +
        animationOffset;
    const auto bodyColor = desc.player.movementMode == movement::MovementMode::WallRunning
        ? std::array<float, 4>{0.34F, 0.86F, 1.0F, 0.62F}
        : desc.player.movementMode == movement::MovementMode::Mantling
            ? std::array<float, 4>{0.86F, 0.78F, 0.42F, 0.62F}
            : std::array<float, 4>{0.54F, 0.62F, 0.58F, 0.52F};

    if (!appendMesh(
            frame,
            desc,
            "chr_project_male1",
            bodyPosition,
            {0.82F, 0.82F, 0.82F},
            view.yawDegrees,
            bodyColor,
            stats,
            animationPitch,
            (desc.player.cameraRollDegrees * 0.18F) + animationRoll)) {
        (void)appendMesh(
            frame,
            desc,
            "chr_a2_pilot_operator_01",
            bodyPosition,
            {0.86F, 0.86F, 0.86F},
            view.yawDegrees,
            bodyColor,
            stats,
            animationPitch,
            (desc.player.cameraRollDegrees * 0.18F) + animationRoll);
    }
}

void DevRangeRenderSceneBuilder::appendFirstPersonMeshes(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    const auto view = renderView(desc);
    const auto eye = playerEyePosition(desc);
    const auto& mount = firstPersonWeaponMount(desc.player.activeWeaponId);
    const auto& bodyMount = firstPersonBodyMount();
    const auto& armsMount = firstPersonArmMount(desc.player.adsAlpha > 0.35F);

    player::CharacterAnimationFrame animation = desc.player.animation;
    if (!desc.player.hasAnimationFrame) {
        player::CharacterAnimationState fallbackState{};
        animation = player::evaluateCharacterAnimation(
            fallbackState,
            player::CharacterAnimationInput{
                {},
                desc.player.movementMode,
                desc.player.movementTech,
                desc.player.weapon,
                desc.player.speed01,
                desc.player.cameraRollDegrees,
                desc.player.mantleProgress01,
                1.0F / 60.0F,
                desc.player.hasWallRunContact,
                desc.player.adsAlpha > 0.35F,
                false,
                false,
                false,
            });
    }

    player::FirstPersonRigInput rigInput{};
    rigInput.cameraPosition = eye;
    rigInput.view = view;
    rigInput.animation = animation;
    rigInput.weapon = desc.player.weapon;
    rigInput.weaponClass = desc.player.activeWeaponClass;
    rigInput.movementMode = desc.player.movementMode;
    rigInput.headBobOffset = desc.player.headBobOffset;
    rigInput.weaponSwayOffset = desc.player.weaponSwayOffset;
    rigInput.weaponMount = toRigMount(mount);
    rigInput.armsMount = toRigMount(armsMount);
    rigInput.bodyMount = toRigMount(bodyMount);
    rigInput.cameraRollDegrees = desc.player.cameraRollDegrees;
    rigInput.adsAlpha = desc.player.adsAlpha;
    rigInput.speed01 = desc.player.speed01;
    rigInput.mantleProgress01 = desc.player.mantleProgress01;
    rigInput.hasWallRunContact = desc.player.hasWallRunContact;
    rigInput.hasAnimationFrame = desc.player.hasAnimationFrame;
    const auto rig = player::evaluateFirstPersonRig(rigInput);

    appendFirstPersonRigPrimitives(frame, rig, stats);
    appendWeaponFeedbackPrimitives(frame, desc, rig, stats);
    const auto& weaponSocket = player::firstPersonRigSocket(rig, player::FirstPersonRigSocket::WeaponRoot);
    auto weaponPosition = weaponSocket.valid ? weaponSocket.worldPosition : rig.weapon.position;
    const auto weaponYaw = weaponSocket.valid ? weaponSocket.yawDegrees : rig.weapon.yawDegrees;
    const auto weaponPitch = weaponSocket.valid ? weaponSocket.pitchDegrees : rig.weapon.pitchDegrees;
    const auto weaponRoll = weaponSocket.valid ? weaponSocket.rollDegrees : rig.weapon.rollDegrees;
    const std::string_view selectedWeaponAssetId = findMesh(desc, mount.assetId).isValid()
        ? mount.assetId
        : mount.fallbackAssetId;
    const auto cookedSockets = cookedSocketCount(desc, selectedWeaponAssetId);
    stats.firstPersonCookedSocketCount += cookedSockets;
    if (const auto muzzleLocal = cookedSocketLocalPosition(desc, selectedWeaponAssetId, "socket_muzzle");
        muzzleLocal.has_value()) {
        const auto& muzzleSocket = player::firstPersonRigSocket(rig, player::FirstPersonRigSocket::Muzzle);
        if (muzzleSocket.valid) {
            weaponPosition = alignMeshPositionToSocket(
                muzzleSocket.worldPosition,
                *muzzleLocal,
                rig.weapon.scale,
                weaponYaw,
                weaponPitch,
                weaponRoll);
            ++stats.firstPersonSocketAlignedMeshCount;
        }
    }

    bool weaponMeshAppended = appendMesh(
        frame,
        desc,
        mount.assetId,
        weaponPosition,
        rig.weapon.scale,
        weaponYaw,
        withAlpha(mount.color, rig.weapon.alpha),
        stats,
        weaponPitch,
        weaponRoll);
    if (!weaponMeshAppended) {
        weaponMeshAppended = appendMesh(
            frame,
            desc,
            mount.fallbackAssetId,
            weaponPosition,
            rig.weapon.scale,
            weaponYaw,
            withAlpha(mount.color, rig.weapon.alpha),
            stats,
            weaponPitch,
            weaponRoll);
    }
    if (weaponMeshAppended) {
        ++stats.firstPersonMeshCount;
    }

    bool armsMeshAppended = appendMesh(
        frame,
        desc,
        armsMount.assetId,
        rig.arms.position,
        rig.arms.scale,
        rig.arms.yawDegrees,
        withAlpha(armsMount.color, rig.arms.alpha),
        stats,
        rig.arms.pitchDegrees,
        rig.arms.rollDegrees);
    if (!armsMeshAppended) {
        armsMeshAppended = appendMesh(
            frame,
            desc,
            "chr_a1_fp_arms_01",
            rig.arms.position,
            rig.arms.scale * 0.74F,
            rig.arms.yawDegrees,
            kArmsTint,
            stats,
            rig.arms.pitchDegrees,
            rig.arms.rollDegrees);
    }
    if (armsMeshAppended) {
        ++stats.firstPersonMeshCount;
    }
}

void DevRangeRenderSceneBuilder::appendMovementTechVisuals(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    const auto& tech = desc.player.movementTech;
    const auto vectors = player::viewVectors(renderView(desc));
    const auto eye = playerEyePosition(desc);
    const auto bootBase = desc.player.position + novacore::math::Vec3{0.0F, 0.16F, 0.0F};

    if (tech.gravityInvertersActive || tech.gravityInverterGlowSeconds > 0.0F) {
        const auto right = vectors.horizontalRight;
        const auto glowColor = tech.wallRunArmTriggerPressed
            ? std::array<float, 4>{0.96F, 0.58F, 0.18F, 1.0F}
            : std::array<float, 4>{0.08F, 0.86F, 1.0F, 1.0F};
        appendBox(frame, bootBase + (right * 0.18F), {0.09F, 0.035F, 0.14F}, glowColor, stats);
        appendBox(frame, bootBase - (right * 0.18F), {0.09F, 0.035F, 0.14F}, glowColor, stats);

        if (tech.wallRunArmTriggerPressed || tech.wallRunArmTriggerSeconds > 0.0F) {
            const auto armCenter =
                eye +
                (vectors.forward * 0.52F) -
                (right * 0.24F) +
                novacore::math::Vec3{0.0F, -0.34F, 0.0F};
            const bool armMeshAppended = appendMesh(
                frame,
                desc,
                "chr_a1_fp_arms_01",
                armCenter,
                {0.54F, 0.54F, 0.54F},
                renderView(desc).yawDegrees - 10.0F,
                {0.58F, 0.78F, 0.72F, 0.92F},
                stats,
                (renderView(desc).pitchDegrees * 0.28F) - 8.0F,
                desc.player.cameraRollDegrees * 0.42F);
            if (!armMeshAppended) {
                appendBox(
                    frame,
                    armCenter,
                    {0.045F, 0.028F, 0.035F},
                    {0.98F, 0.42F, 0.14F, 1.0F},
                    stats);
            }
        }
    }

    if (tech.doubleJumpPlatformThrown || tech.energyPlatformSeconds > 0.0F) {
        const auto platformCenter = tech.energyPlatformCenter.lengthSquared() > 0.0001F
            ? tech.energyPlatformCenter
            : desc.player.position + novacore::math::Vec3{0.0F, -0.18F, 0.0F};
        appendBox(
            frame,
            platformCenter,
            {0.52F, 0.018F, 0.32F},
            {0.18F, 0.82F, 1.0F, 1.0F},
            stats);
        appendBox(
            frame,
            platformCenter + novacore::math::Vec3{0.0F, 0.028F, 0.0F},
            {0.32F, 0.012F, 0.18F},
            {0.88F, 0.98F, 1.0F, 1.0F},
            stats);

        const auto throwArmCenter =
            eye +
            (vectors.forward * 0.56F) -
            (vectors.horizontalRight * 0.28F) +
            novacore::math::Vec3{0.0F, -0.40F, 0.0F};
        (void)appendMesh(
            frame,
            desc,
            "chr_a1_fp_arms_01",
            throwArmCenter,
            {0.50F, 0.50F, 0.50F},
            renderView(desc).yawDegrees - 16.0F,
            {0.58F, 0.84F, 0.92F, 0.90F},
            stats,
            (renderView(desc).pitchDegrees * 0.30F) - 12.0F,
            desc.player.cameraRollDegrees * 0.25F);
    }

    if (tech.mantleReachTriggered || tech.mantleReachSeconds > 0.0F) {
        const float progress = easeOut01(desc.player.mantleProgress01);
        const auto handCenter =
            eye +
            (vectors.forward * (0.62F + (progress * 0.34F))) -
            (vectors.horizontalRight * (0.24F - (progress * 0.10F))) +
            novacore::math::Vec3{0.0F, -0.30F + (progress * 0.22F), 0.0F};
        const bool mantleArmMeshAppended = appendMesh(
            frame,
            desc,
            "chr_a1_fp_arms_01",
            handCenter,
            {0.50F, 0.50F, 0.50F},
            renderView(desc).yawDegrees - 8.0F,
            {0.68F, 0.88F, 0.92F, 0.94F},
            stats,
            (renderView(desc).pitchDegrees * 0.24F) - (progress * 10.0F),
            desc.player.cameraRollDegrees * 0.20F);
        if (!mantleArmMeshAppended) {
            appendBox(
                frame,
                handCenter,
                {0.035F, 0.035F, 0.16F},
                {0.72F, 0.94F, 1.0F, 1.0F},
                stats);
            appendBox(
                frame,
                handCenter - (vectors.forward * 0.18F) + novacore::math::Vec3{0.0F, -0.04F, 0.0F},
                {0.045F, 0.032F, 0.14F},
                {0.48F, 0.64F, 0.70F, 1.0F},
                stats);
        }
    }

    if (tech.mantleClimbTriggered || tech.mantleClimbSeconds > 0.0F) {
        const auto target = tech.mantleTargetPosition.lengthSquared() > 0.0001F
            ? tech.mantleTargetPosition
            : desc.player.position;
        appendBox(
            frame,
            target + novacore::math::Vec3{0.0F, 0.08F, 0.0F},
            {0.34F, 0.035F, 0.34F},
            {0.98F, 0.86F, 0.24F, 1.0F},
            stats);
    }
}

void DevRangeRenderSceneBuilder::appendWorldDebugLines(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    const auto vectors = player::viewVectors(renderView(desc));
    const auto eye = playerEyePosition(desc);
    frame.worldLines.push_back(novacore::render::RenderLine3D{
        eye,
        eye + (vectors.forward * 24.0F),
        {0.40F, 0.92F, 1.0F, 1.0F},
    });
    ++stats.worldLineCount;

    if (desc.collision != nullptr && desc.collision->grounded) {
        const auto normalBase = desc.player.position + novacore::math::Vec3{0.0F, 0.08F, 0.0F};
        frame.worldLines.push_back(novacore::render::RenderLine3D{
            normalBase,
            normalBase + (desc.collision->groundNormal * 1.2F),
            desc.collision->onRamp
                ? std::array<float, 4>{0.30F, 1.0F, 0.55F, 1.0F}
                : desc.collision->stepped
                    ? std::array<float, 4>{0.95F, 0.80F, 0.24F, 1.0F}
                    : std::array<float, 4>{0.72F, 0.95F, 1.0F, 1.0F},
        });
        ++stats.worldLineCount;
    }

    if (desc.collision != nullptr && desc.collision->nearWallRunSurface) {
        const auto wallBase = desc.player.position + novacore::math::Vec3{0.0F, 1.18F, 0.0F};
        frame.worldLines.push_back(novacore::render::RenderLine3D{
            wallBase,
            wallBase + (desc.collision->wallNormal * 1.05F),
            {0.15F, 0.90F, 1.0F, 1.0F},
        });
        ++stats.worldLineCount;
        frame.worldLines.push_back(novacore::render::RenderLine3D{
            wallBase,
            wallBase + (desc.collision->wallTangent * 1.85F),
            {1.0F, 0.58F, 0.18F, 1.0F},
        });
        ++stats.worldLineCount;
    }

    if (desc.collision != nullptr && desc.collision->swept) {
        const auto sweepBase = desc.collision->sweepStartPosition + novacore::math::Vec3{0.0F, 0.18F, 0.0F};
        frame.worldLines.push_back(novacore::render::RenderLine3D{
            sweepBase,
            sweepBase + desc.collision->requestedDisplacement,
            {0.38F, 0.42F, 0.46F, 0.65F},
        });
        ++stats.worldLineCount;
        frame.worldLines.push_back(novacore::render::RenderLine3D{
            sweepBase,
            sweepBase + desc.collision->appliedDisplacement,
            desc.collision->sweepHit
                ? std::array<float, 4>{1.0F, 0.28F, 0.16F, 1.0F}
                : std::array<float, 4>{0.35F, 1.0F, 0.62F, 1.0F},
        });
        ++stats.worldLineCount;

        if (desc.collision->sweepHit) {
            const auto hitBase = sweepBase + desc.collision->appliedDisplacement;
            frame.worldLines.push_back(novacore::render::RenderLine3D{
                hitBase,
                hitBase + (desc.collision->sweepNormal * 1.25F),
                {1.0F, 0.70F, 0.16F, 1.0F},
            });
            ++stats.worldLineCount;
        }
    }

    if (desc.collision != nullptr && !desc.collision->contacts.empty()) {
        std::size_t emittedContacts = 0;
        for (const auto& contact : desc.collision->contacts) {
            if (contact.normal.lengthSquared() <= 0.0001F) {
                continue;
            }
            const auto point = contact.point.lengthSquared() > 0.0001F
                ? contact.point
                : desc.player.position + novacore::math::Vec3{0.0F, 0.16F, 0.0F};
            frame.worldLines.push_back(novacore::render::RenderLine3D{
                point + novacore::math::Vec3{0.0F, 0.06F, 0.0F},
                point + novacore::math::Vec3{0.0F, 0.06F, 0.0F} + (contact.normal * (contact.role == GreyboxContactRole::Sweep ? 1.15F : 0.72F)),
                contactColor(contact.role),
            });
            ++stats.worldLineCount;
            ++emittedContacts;
            if (emittedContacts >= 8U) {
                break;
            }
        }
    }

    if (desc.collision != nullptr && desc.collision->mantleCandidate) {
        const auto start = eye - novacore::math::Vec3{0.0F, 0.25F, 0.0F};
        frame.worldLines.push_back(novacore::render::RenderLine3D{
            start,
            desc.collision->mantleObstaclePoint,
            {0.95F, 0.78F, 0.18F, 1.0F},
        });
        ++stats.worldLineCount;
        frame.worldLines.push_back(novacore::render::RenderLine3D{
            desc.collision->mantleObstaclePoint,
            desc.collision->mantleTargetPosition + novacore::math::Vec3{0.0F, 0.08F, 0.0F},
            {0.98F, 0.92F, 0.38F, 1.0F},
        });
        ++stats.worldLineCount;
    }
}

} // namespace nemisis::dev

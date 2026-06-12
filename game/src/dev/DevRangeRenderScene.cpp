#include "nemisis/dev/DevRangeRenderScene.hpp"

#include "nemisis/player/PlayerView.hpp"

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
constexpr std::array<float, 4> kWeaponBoxTint{0.08F, 0.11F, 0.12F, 1.0F};
constexpr std::array<float, 4> kWeaponMeshTint{0.16F, 0.18F, 0.19F, 1.0F};
constexpr std::array<float, 4> kSmgMeshTint{0.10F, 0.16F, 0.18F, 1.0F};
constexpr std::array<float, 4> kSidearmMeshTint{0.14F, 0.13F, 0.12F, 1.0F};
constexpr std::array<float, 4> kArmsTint{0.38F, 0.42F, 0.40F, 1.0F};
constexpr std::array<float, 4> kMuzzleTint{0.95F, 0.64F, 0.20F, 1.0F};
constexpr std::array<float, 4> kAimCoreTint{0.88F, 0.98F, 1.0F, 1.0F};
constexpr std::array<float, 4> kAimLineTint{0.58F, 0.92F, 1.0F, 1.0F};

struct StaticMeshPlacement final {
    std::string_view assetId;
    novacore::math::Vec3 position;
    novacore::math::Vec3 scale;
    float yawDegrees = 0.0F;
    std::array<float, 4> color;
};

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

[[nodiscard]] bool isSidearm(weapons::WeaponClass weaponClass) {
    return weaponClass == weapons::WeaponClass::Sidearm;
}

[[nodiscard]] std::string_view firstPersonWeaponAssetId(std::string_view weaponId) {
    if (weaponId == "sidearm_01") {
        return "wpn_a2_striker_sidearm_01";
    }
    if (weaponId == "smg_01") {
        return "wpn_a2_blackout_carbine_01";
    }
    if (weaponId == "shotgun_01") {
        return "wpn_a2_modular_rifle_01";
    }
    return "wpn_a2_blackout_carbine_01";
}

[[nodiscard]] std::string_view fallbackWeaponAssetId(std::string_view weaponId) {
    if (weaponId == "sidearm_01") {
        return "wpn_sidearm_01";
    }
    if (weaponId == "smg_01") {
        return "wpn_proto_smg_01";
    }
    return "wpn_ar_01";
}

[[nodiscard]] std::array<float, 4> firstPersonWeaponTint(weapons::WeaponClass weaponClass) {
    if (weaponClass == weapons::WeaponClass::Sidearm) {
        return kSidearmMeshTint;
    }
    if (weaponClass == weapons::WeaponClass::Smg) {
        return kSmgMeshTint;
    }
    return kWeaponMeshTint;
}

[[nodiscard]] std::array<StaticMeshPlacement, 17> staticShowcaseMeshes() {
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
            {-13.0F, 0.0F, 11.0F},
            {1.0F, 1.0F, 1.0F},
            140.0F,
            {0.22F, 0.64F, 0.72F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_a2_blackout_carbine_01",
            {-8.2F, 1.05F, -9.2F},
            {0.88F, 0.88F, 0.88F},
            25.0F,
            {0.14F, 0.18F, 0.17F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_a2_modular_rifle_01",
            {-5.8F, 1.08F, -9.2F},
            {0.82F, 0.82F, 0.82F},
            25.0F,
            {0.13F, 0.18F, 0.20F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_a2_striker_sidearm_01",
            {-3.6F, 1.00F, -9.2F},
            {0.92F, 0.92F, 0.92F},
            25.0F,
            {0.16F, 0.15F, 0.14F, 1.0F},
        },
        StaticMeshPlacement{
            "map_a2_wallrun_panel_01",
            {-18.4F, 0.0F, -5.5F},
            {1.0F, 1.0F, 1.0F},
            90.0F,
            {0.08F, 0.58F, 0.70F, 1.0F},
        },
        StaticMeshPlacement{
            "map_a2_slide_ramp_01",
            {-13.5F, 0.0F, -6.2F},
            {1.0F, 1.0F, 1.0F},
            0.0F,
            {0.15F, 0.60F, 0.52F, 1.0F},
        },
        StaticMeshPlacement{
            "map_a2_cover_crate_01",
            {12.5F, 0.0F, 3.8F},
            {1.0F, 1.0F, 1.0F},
            -18.0F,
            {0.40F, 0.48F, 0.44F, 1.0F},
        },
        StaticMeshPlacement{
            "prop_a2_range_hero_01",
            {0.0F, 0.0F, 11.6F},
            {1.0F, 1.0F, 1.0F},
            180.0F,
            {0.95F, 0.50F, 0.20F, 1.0F},
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

} // namespace

DevRangeRenderSceneStats DevRangeRenderSceneBuilder::append(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc) const {
    DevRangeRenderSceneStats stats{};
    if (desc.greyboxWorld == nullptr || desc.meshResources == nullptr) {
        return stats;
    }

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
    frame.worldBoxes.reserve(frame.worldBoxes.size() + desc.greyboxWorld->primitives.size() + targetLaneCount + 8U);
    frame.worldMeshes.reserve(frame.worldMeshes.size() + 12U + (targetLaneCount * 2U));

    appendWorldGeometry(frame, desc, stats);
    appendStaticShowcaseMeshes(frame, desc, stats);
    appendTargetLaneMeshes(frame, desc, stats);
    appendFirstPersonMeshes(frame, desc, stats);
    appendAimMarker(frame, desc, stats);
    if (desc.showWorldDebugLines) {
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
    DevRangeRenderSceneStats& stats) const {
    const auto handle = findMesh(desc, assetId);
    if (!handle.isValid()) {
        ++stats.skippedMeshInstanceCount;
        return false;
    }

    frame.worldMeshes.push_back(novacore::render::RenderMesh3D{
        handle,
        std::string(assetId),
        position,
        scale,
        yawDegrees,
        color,
    });
    ++stats.meshInstanceCount;
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

void DevRangeRenderSceneBuilder::appendStaticShowcaseMeshes(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    for (const auto& placement : staticShowcaseMeshes()) {
        (void)appendMesh(
            frame,
            desc,
            placement.assetId,
            placement.position,
            placement.scale,
            placement.yawDegrees,
            placement.color,
            stats);
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

        const auto actorAsset = eliminated ? std::string_view("prop_target_dummy_01") : std::string_view("chr_dev_soldier_a");
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

void DevRangeRenderSceneBuilder::appendFirstPersonMeshes(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    const auto view = renderView(desc);
    const auto vectors = player::viewVectors(view);
    const auto eye = playerEyePosition(desc);
    const bool sidearm = isSidearm(desc.player.activeWeaponClass);
    const float ads = desc.player.adsAlpha;
    const float recoilLift = desc.player.weapon.recoilPitchOffsetDegrees * 0.010F;
    const float recoilSide = desc.player.weapon.recoilYawOffsetDegrees * 0.014F;
    const float forwardDistance = sidearm ? 0.76F : 0.98F;
    const float rightOffset = sidearm ? 0.22F : 0.30F;
    const float lowReady = sidearm ? -0.29F : -0.34F;
    const auto weaponCenter =
        eye +
        (vectors.forward * (forwardDistance - (ads * 0.18F))) +
        (vectors.horizontalRight * ((rightOffset * (1.0F - (ads * 0.74F))) + recoilSide)) +
        novacore::math::Vec3{0.0F, lowReady + (ads * 0.16F) + recoilLift, 0.0F} +
        (desc.player.weaponSwayOffset * (sidearm ? 0.68F : 1.0F));

    appendBox(
        frame,
        weaponCenter,
        {
            sidearm ? 0.11F : 0.20F,
            sidearm ? 0.07F : 0.08F,
            sidearm ? 0.21F : 0.46F,
        },
        kWeaponBoxTint,
        stats);

    const auto activeAsset = firstPersonWeaponAssetId(desc.player.activeWeaponId);
    const auto weaponScale = sidearm
        ? novacore::math::Vec3{0.46F, 0.46F, 0.46F}
        : novacore::math::Vec3{0.58F - (ads * 0.08F), 0.58F - (ads * 0.08F), 0.58F - (ads * 0.08F)};
    bool weaponMeshAppended = appendMesh(
        frame,
        desc,
        activeAsset,
        weaponCenter,
        weaponScale,
        view.yawDegrees + (sidearm ? -4.0F : 0.0F) + (desc.player.weapon.recoilYawOffsetDegrees * 0.55F),
        firstPersonWeaponTint(desc.player.activeWeaponClass),
        stats);
    if (!weaponMeshAppended) {
        weaponMeshAppended = appendMesh(
            frame,
            desc,
            fallbackWeaponAssetId(desc.player.activeWeaponId),
            weaponCenter,
            weaponScale,
            view.yawDegrees,
            firstPersonWeaponTint(desc.player.activeWeaponClass),
            stats);
    }
    if (weaponMeshAppended) {
        ++stats.firstPersonMeshCount;
    }

    const auto muzzleCenter =
        weaponCenter +
        (vectors.forward * (sidearm ? 0.35F : 0.62F)) +
        (vectors.horizontalRight * (sidearm ? -0.02F : -0.05F)) +
        novacore::math::Vec3{0.0F, sidearm ? 0.02F : 0.03F, 0.0F};
    appendBox(frame, muzzleCenter, sidearm ? novacore::math::Vec3{0.035F, 0.035F, 0.035F} : novacore::math::Vec3{0.045F, 0.045F, 0.045F}, kAimCoreTint, stats);
    if (desc.player.weapon.timeSinceLastShotSeconds < 0.075F && desc.player.weapon.shotIndex > 0U) {
        appendBox(frame, muzzleCenter + (vectors.forward * 0.16F), {0.06F, 0.04F, 0.06F}, kMuzzleTint, stats);
    }

    appendBox(
        frame,
        weaponCenter - (vectors.forward * (sidearm ? 0.12F : 0.18F)) - (vectors.horizontalRight * 0.13F) + novacore::math::Vec3{0.0F, -0.09F, 0.0F},
        {0.07F, 0.055F, 0.09F},
        kArmsTint,
        stats);
    appendBox(
        frame,
        weaponCenter - (vectors.forward * (sidearm ? 0.03F : 0.05F)) + (vectors.horizontalRight * 0.17F) + novacore::math::Vec3{0.0F, -0.10F, 0.0F},
        {0.075F, 0.055F, 0.10F},
        kArmsTint,
        stats);

    bool armsMeshAppended = appendMesh(
        frame,
        desc,
        "chr_a1_fp_arms_01",
        eye +
            (vectors.forward * (sidearm ? 0.58F : 0.70F)) +
            (vectors.horizontalRight * (sidearm ? 0.02F : 0.05F)) +
            novacore::math::Vec3{0.0F, sidearm ? -0.46F : -0.50F, 0.0F} +
            (desc.player.weaponSwayOffset * 0.55F),
        {0.42F, 0.42F, 0.42F},
        view.yawDegrees,
        kArmsTint,
        stats);
    if (!armsMeshAppended) {
        armsMeshAppended = appendMesh(
            frame,
            desc,
            "chr_dev_arms_a",
            eye +
                (vectors.forward * 0.68F) +
                (vectors.horizontalRight * 0.05F) +
                novacore::math::Vec3{0.0F, -0.50F, 0.0F} +
                (desc.player.weaponSwayOffset * 0.55F),
            {0.32F, 0.32F, 0.32F},
            view.yawDegrees,
            kArmsTint,
            stats);
    }
    if (armsMeshAppended) {
        ++stats.firstPersonMeshCount;
    }
}

void DevRangeRenderSceneBuilder::appendAimMarker(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    const auto vectors = player::viewVectors(renderView(desc));
    const auto aimCenter = playerEyePosition(desc) + (vectors.forward * 18.0F);

    appendBox(frame, aimCenter, {0.055F, 0.055F, 0.055F}, kAimCoreTint, stats);
    ++stats.aimMarkerBoxCount;

    appendBox(
        frame,
        aimCenter + novacore::math::Vec3{0.18F, 0.0F, 0.0F},
        {0.11F, 0.018F, 0.018F},
        kAimLineTint,
        stats);
    ++stats.aimMarkerBoxCount;

    appendBox(
        frame,
        aimCenter + novacore::math::Vec3{-0.18F, 0.0F, 0.0F},
        {0.11F, 0.018F, 0.018F},
        kAimLineTint,
        stats);
    ++stats.aimMarkerBoxCount;

    appendBox(
        frame,
        aimCenter + novacore::math::Vec3{0.0F, 0.18F, 0.0F},
        {0.018F, 0.11F, 0.018F},
        kAimLineTint,
        stats);
    ++stats.aimMarkerBoxCount;

    appendBox(
        frame,
        aimCenter + novacore::math::Vec3{0.0F, -0.18F, 0.0F},
        {0.018F, 0.11F, 0.018F},
        kAimLineTint,
        stats);
    ++stats.aimMarkerBoxCount;
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
}

} // namespace nemisis::dev

#include "nemisis/dev/DevRangeRenderScene.hpp"

#include "nemisis/player/PlayerView.hpp"

#include <algorithm>
#include <cmath>
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
constexpr std::array<float, 4> kMuzzleTint{0.95F, 0.64F, 0.20F, 1.0F};
constexpr std::array<float, 4> kAimCoreTint{0.88F, 0.98F, 1.0F, 1.0F};
constexpr std::array<float, 4> kAimLineTint{0.58F, 0.92F, 1.0F, 1.0F};
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

[[nodiscard]] float clamp01(float value) {
    return std::clamp(value, 0.0F, 1.0F);
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
        return "wpn_project_sidearm_glock19";
    }
    if (weaponId == "smg_01") {
        return "wpn_project_smg_fr17";
    }
    if (weaponId == "shotgun_01") {
        return "wpn_project_rifle_ncar";
    }
    return "wpn_project_rifle_m4a1";
}

[[nodiscard]] std::string_view fallbackWeaponAssetId(std::string_view weaponId) {
    if (weaponId == "sidearm_01") {
        return "wpn_a2_striker_sidearm_01";
    }
    if (weaponId == "smg_01") {
        return "wpn_a2_blackout_carbine_01";
    }
    return "wpn_a2_modular_rifle_01";
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
            {2.05F, 2.05F, 2.05F},
            92.0F,
            {0.62F, 0.64F, 0.58F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_rifle_afr120",
            {-2.75F, 1.05F, -7.88F},
            {0.78F, 0.78F, 0.78F},
            88.0F,
            {0.54F, 0.64F, 0.72F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_rifle_ncar",
            {-0.55F, 1.04F, -7.86F},
            {1.10F, 1.10F, 1.10F},
            88.0F,
            {0.56F, 0.60F, 0.66F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_smg_fr17",
            {1.45F, 1.00F, -7.82F},
            {1.55F, 1.55F, 1.55F},
            90.0F,
            {0.46F, 0.62F, 0.68F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_sidearm_glock19",
            {3.15F, 0.92F, -7.86F},
            {2.00F, 2.00F, 2.00F},
            92.0F,
            {0.66F, 0.61F, 0.54F, 1.0F},
        },
        StaticMeshPlacement{
            "wpn_project_sidearm_p320",
            {4.55F, 0.92F, -7.86F},
            {2.05F, 2.05F, 2.05F},
            92.0F,
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
    frame.worldBoxes.reserve(frame.worldBoxes.size() + desc.greyboxWorld->primitives.size() + targetLaneCount + 21U);
    frame.worldMeshes.reserve(frame.worldMeshes.size() + 14U + (targetLaneCount * 2U));

    appendSkyboxMesh(frame, desc, stats);
    appendWorldGeometry(frame, desc, stats);
    appendStaticShowcaseMeshes(frame, desc, stats);
    appendTargetLaneMeshes(frame, desc, stats);
    appendLocalPlayerBodyMesh(frame, desc, stats);
    appendFirstPersonMeshes(frame, desc, stats);
    appendMovementTechVisuals(frame, desc, stats);
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
    DevRangeRenderSceneStats& stats,
    float pitchDegrees,
    float rollDegrees) const {
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
        pitchDegrees,
        rollDegrees,
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

void DevRangeRenderSceneBuilder::appendSkyboxMesh(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    const auto eye = playerEyePosition(desc);
    (void)appendMesh(
        frame,
        desc,
        "env_project_skybox1",
        eye,
        {42.0F, 42.0F, 42.0F},
        0.0F,
        {0.74F, 0.82F, 0.92F, 1.0F},
        stats);
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
    if (!desc.player.hasMovementState) {
        return;
    }

    const auto view = renderView(desc);
    const auto vectors = player::viewVectors(view);
    const auto bodyPosition =
        desc.player.position -
        (vectors.forward * 0.16F) +
        novacore::math::Vec3{0.0F, -0.62F, 0.0F};
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
            0.0F,
            desc.player.cameraRollDegrees * 0.18F)) {
        (void)appendMesh(
            frame,
            desc,
            "chr_a2_pilot_operator_01",
            bodyPosition,
            {0.86F, 0.86F, 0.86F},
            view.yawDegrees,
            bodyColor,
            stats,
            0.0F,
            desc.player.cameraRollDegrees * 0.18F);
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
    const float reloadProgress = desc.player.weapon.reloading ? clamp01(desc.player.weapon.reloadProgress) : 0.0F;
    const float reloadArc = std::sin(reloadProgress * 3.1415926535F);
    const float mantleLift = easeOut01(desc.player.mantleProgress01) * 0.13F;
    const float wallRunRoll = desc.player.hasWallRunContact ? desc.player.cameraRollDegrees * 0.32F : 0.0F;
    const float forwardDistance = sidearm ? 0.78F : 0.92F;
    const float rightOffset = sidearm ? 0.20F : 0.24F;
    const float lowReady = sidearm ? -0.25F : -0.29F;
    const auto weaponCenter =
        eye +
        (vectors.forward * (forwardDistance - (ads * 0.18F))) +
        (vectors.horizontalRight * ((rightOffset * (1.0F - (ads * 0.74F))) + recoilSide)) +
        novacore::math::Vec3{0.0F, lowReady + (ads * 0.16F) + recoilLift - (reloadArc * 0.12F) + mantleLift, 0.0F} +
        (desc.player.weaponSwayOffset * (sidearm ? 0.68F : 1.0F));

    const auto activeAsset = firstPersonWeaponAssetId(desc.player.activeWeaponId);
    const auto weaponScale = sidearm
        ? novacore::math::Vec3{1.44F - (ads * 0.12F), 1.44F - (ads * 0.12F), 1.44F - (ads * 0.12F)}
        : novacore::math::Vec3{1.78F - (ads * 0.18F), 1.78F - (ads * 0.18F), 1.78F - (ads * 0.18F)};
    bool weaponMeshAppended = appendMesh(
        frame,
        desc,
        activeAsset,
        weaponCenter,
        weaponScale,
        view.yawDegrees + (sidearm ? -4.0F : 0.0F) + (desc.player.weapon.recoilYawOffsetDegrees * 0.55F),
        firstPersonWeaponTint(desc.player.activeWeaponClass),
        stats,
        view.pitchDegrees + (desc.player.weapon.recoilPitchOffsetDegrees * 0.42F) + (reloadArc * (sidearm ? 7.0F : 10.0F)),
        wallRunRoll - (reloadArc * (sidearm ? 6.0F : 10.0F)));
    if (!weaponMeshAppended) {
        weaponMeshAppended = appendMesh(
            frame,
            desc,
            fallbackWeaponAssetId(desc.player.activeWeaponId),
            weaponCenter,
            weaponScale,
            view.yawDegrees,
            firstPersonWeaponTint(desc.player.activeWeaponClass),
            stats,
            view.pitchDegrees,
            wallRunRoll);
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

    bool armsMeshAppended = appendMesh(
        frame,
        desc,
        "chr_a1_fp_arms_01",
        eye +
            (vectors.forward * (sidearm ? 0.64F : 0.78F)) +
            (vectors.horizontalRight * ((sidearm ? 0.04F : 0.08F) - (reloadArc * 0.05F))) +
            novacore::math::Vec3{0.0F, (sidearm ? -0.44F : -0.48F) - (reloadArc * 0.11F) + mantleLift, 0.0F} +
            (desc.player.weaponSwayOffset * 0.55F),
        {0.62F, 0.62F, 0.62F},
        view.yawDegrees,
        kArmsTint,
        stats,
        view.pitchDegrees + (reloadArc * 8.0F),
        wallRunRoll - (reloadArc * 12.0F));
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
            {0.44F, 0.44F, 0.44F},
            view.yawDegrees,
            kArmsTint,
            stats,
            view.pitchDegrees,
            wallRunRoll);
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
            appendBox(
                frame,
                eye + (vectors.forward * 0.48F) - (right * 0.26F) + novacore::math::Vec3{0.0F, -0.36F, 0.0F},
                {0.045F, 0.028F, 0.035F},
                {0.98F, 0.42F, 0.14F, 1.0F},
                stats);
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
    }

    if (tech.mantleReachTriggered || tech.mantleReachSeconds > 0.0F) {
        const float progress = easeOut01(desc.player.mantleProgress01);
        const auto handCenter =
            eye +
            (vectors.forward * (0.62F + (progress * 0.34F))) -
            (vectors.horizontalRight * (0.24F - (progress * 0.10F))) +
            novacore::math::Vec3{0.0F, -0.30F + (progress * 0.22F), 0.0F};
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

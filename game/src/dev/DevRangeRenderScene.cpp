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
constexpr std::array<float, 4> kArmsTint{0.38F, 0.42F, 0.40F, 1.0F};
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
    return desc.debugTarget != nullptr && desc.debugTarget->eliminated;
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

[[nodiscard]] std::array<StaticMeshPlacement, 10> staticShowcaseMeshes(bool eliminated) {
    return {
        StaticMeshPlacement{
            "env_test_arena_kit_01",
            {0.0F, 0.0F, 0.0F},
            {1.0F, 1.0F, 1.0F},
            0.0F,
            kArenaTint,
        },
        StaticMeshPlacement{
            eliminated ? std::string_view("prop_target_dummy_01") : std::string_view("chr_dev_soldier_a"),
            {0.0F, 0.0F, 15.0F},
            {1.0F, 1.0F, 1.0F},
            180.0F,
            eliminated ? kEliminatedTargetTint : kActiveTargetTint,
        },
        StaticMeshPlacement{
            "prop_target_dummy_01",
            {-5.5F, 0.0F, 18.0F},
            {0.85F, 0.85F, 0.85F},
            180.0F,
            kDummyTargetTint,
        },
        StaticMeshPlacement{
            "prop_target_dummy_01",
            {5.5F, 0.0F, 18.0F},
            {0.85F, 0.85F, 0.85F},
            180.0F,
            kDummyTargetTint,
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

    frame.worldBoxes.reserve(frame.worldBoxes.size() + desc.greyboxWorld->primitives.size() + 8U);
    frame.worldMeshes.reserve(frame.worldMeshes.size() + 13U);

    appendWorldGeometry(frame, desc, stats);
    appendStaticShowcaseMeshes(frame, desc, stats);
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
    for (const auto& placement : staticShowcaseMeshes(targetEliminated(desc))) {
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

void DevRangeRenderSceneBuilder::appendFirstPersonMeshes(
    novacore::render::RenderFrameInfo& frame,
    const DevRangeRenderSceneDesc& desc,
    DevRangeRenderSceneStats& stats) const {
    const auto vectors = player::viewVectors(renderView(desc));
    const auto eye = playerEyePosition(desc);
    const auto weaponCenter =
        eye +
        (vectors.forward * 0.92F) +
        (vectors.horizontalRight * 0.28F) +
        novacore::math::Vec3{0.0F, -0.24F, 0.0F} +
        desc.player.weaponSwayOffset;

    appendBox(
        frame,
        weaponCenter,
        {
            0.18F + (desc.player.adsAlpha * 0.035F),
            0.08F,
            0.42F - (desc.player.adsAlpha * 0.055F),
        },
        kWeaponBoxTint,
        stats);

    if (appendMesh(
            frame,
            desc,
            "wpn_ar_01",
            weaponCenter,
            {
                0.35F - (desc.player.adsAlpha * 0.04F),
                0.35F - (desc.player.adsAlpha * 0.04F),
                0.35F - (desc.player.adsAlpha * 0.04F),
            },
            renderView(desc).yawDegrees,
            kWeaponMeshTint,
            stats)) {
        ++stats.firstPersonMeshCount;
    }

    if (appendMesh(
            frame,
            desc,
            "wpn_proto_smg_01",
            eye +
                (vectors.forward * 1.10F) +
                (vectors.horizontalRight * -0.18F) +
                novacore::math::Vec3{0.0F, -0.30F, 0.0F} +
                (desc.player.weaponSwayOffset * 0.8F),
            {0.42F, 0.42F, 0.42F},
            renderView(desc).yawDegrees + 90.0F,
            kSmgMeshTint,
            stats)) {
        ++stats.firstPersonMeshCount;
    }

    if (appendMesh(
            frame,
            desc,
            "chr_dev_arms_a",
            eye +
                (vectors.forward * 0.76F) +
                (vectors.horizontalRight * 0.06F) +
                novacore::math::Vec3{0.0F, -0.48F, 0.0F} +
                (desc.player.weaponSwayOffset * 0.55F),
            {0.32F, 0.32F, 0.32F},
            renderView(desc).yawDegrees,
            kArmsTint,
            stats)) {
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
}

} // namespace nemisis::dev

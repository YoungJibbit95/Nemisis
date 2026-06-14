#pragma once

#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/dev/DevTargetRange.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/player/PlayerAnimation.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/math/Types.hpp"
#include "novacore/render/Renderer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nemisis::dev {

using MeshResourceLookup = std::unordered_map<std::string, novacore::render::MeshResourceHandle>;

struct DevRangePlayerRenderState final {
    novacore::math::Vec3 position{};
    player::PlayerViewComponent view{};
    novacore::math::Vec3 cameraPosition{};
    player::PlayerViewComponent cameraView{};
    novacore::math::Vec3 headBobOffset{};
    novacore::math::Vec3 weaponSwayOffset{};
    movement::MovementTechState movementTech{};
    movement::MovementMode movementMode = movement::MovementMode::Grounded;
    float cameraRollDegrees = 0.0F;
    float verticalFovDegrees = 74.0F;
    float speed01 = 0.0F;
    float adsAlpha = 0.0F;
    float mantleProgress01 = 0.0F;
    float wallRunTimeRemaining = 0.0F;
    std::string activeWeaponId = "ar_01";
    weapons::WeaponClass activeWeaponClass = weapons::WeaponClass::AssaultRifle;
    weapons::WeaponRuntimeState weapon{};
    std::uint16_t effectiveMagazineSize = 0;
    player::CharacterAnimationFrame animation{};
    bool hasMovementState = false;
    bool hasCameraRig = false;
    bool hasWallRunContact = false;
    bool hasAnimationFrame = false;
};

struct DevRangeRenderSceneStats final {
    std::size_t worldBoxCount = 0;
    std::size_t meshInstanceCount = 0;
    std::size_t worldLineCount = 0;
    std::size_t skippedMeshInstanceCount = 0;
    std::size_t firstPersonMeshCount = 0;
    std::size_t aimMarkerBoxCount = 0;
    std::size_t targetMeshCount = 0;
};

struct DevRangeRenderSceneDesc final {
    const GreyboxWorld* greyboxWorld = nullptr;
    const DevTargetRangeState* targetRange = nullptr;
    const GreyboxCollisionResult* collision = nullptr;
    const MeshResourceLookup* meshResources = nullptr;
    DevRangePlayerRenderState player{};
    novacore::render::RenderWorldLighting lighting{
        {0.30F, 0.88F, 0.34F},
        0.34F,
        {-0.52F, 0.36F, -0.68F},
        0.22F,
        0.22F,
        0.16F,
        1.12F,
        1.08F,
    };
    bool showWorldDebugLines = true;
    float verticalFovDegrees = 74.0F;
    float nearPlane = 0.03F;
    float farPlane = 120.0F;
};

class DevRangeRenderSceneBuilder final {
public:
    [[nodiscard]] DevRangeRenderSceneStats append(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc) const;

private:
    [[nodiscard]] novacore::render::MeshResourceHandle findMesh(
        const DevRangeRenderSceneDesc& desc,
        std::string_view assetId) const;

    bool appendMesh(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        std::string_view assetId,
        novacore::math::Vec3 position,
        novacore::math::Vec3 scale,
        float yawDegrees,
        std::array<float, 4> color,
        DevRangeRenderSceneStats& stats,
        float pitchDegrees = 0.0F,
        float rollDegrees = 0.0F) const;

    void appendWorldGeometry(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;

    void appendSkyboxMesh(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;

    void appendStaticShowcaseMeshes(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;

    void appendTargetLaneMeshes(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;

    void appendLocalPlayerBodyMesh(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;

    void appendFirstPersonMeshes(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;

    void appendMovementTechVisuals(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;

    void appendAimMarker(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;

    void appendWorldDebugLines(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;
};

} // namespace nemisis::dev

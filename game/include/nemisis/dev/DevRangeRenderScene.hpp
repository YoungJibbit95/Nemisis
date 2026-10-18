#pragma once

#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/dev/DevTargetRange.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/player/PlayerAnimation.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerFirstPersonRig.hpp"
#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/math/Types.hpp"
#include "novacore/render/Mesh.hpp"
#include "novacore/render/Renderer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nemisis::dev {

using MeshResourceLookup = std::unordered_map<std::string, novacore::render::MeshResourceHandle>;

inline constexpr std::string_view kLocalPlayerBodyAssetId = "chr_project_male1";
// Local animation must not mutate the shared showcase/target asset resource.
inline constexpr std::string_view kLocalPlayerBodyResourceId = "player/local/body";

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
    novacore::math::Vec3 latestShotOrigin{};
    novacore::math::Vec3 latestShotEnd{};
    float hitDistanceMeters = 0.0F;
    bool hasMovementState = false;
    bool hasCameraRig = false;
    bool hasWallRunContact = false;
    bool hasAnimationFrame = false;
    bool firedThisFrame = false;
    bool hitThisFrame = false;
    bool eliminatedThisFrame = false;
    bool hasLatestShotTrace = false;
};

struct DevRangeRenderSceneStats final {
    bool skyPassEnabled = false;
    bool firstPersonViewmodelActive = false;
    bool localWorldModelActive = false;
    std::size_t skyFallbackMeshCount = 0;
    std::size_t worldBoxCount = 0;
    std::size_t meshInstanceCount = 0;
    std::size_t worldLineCount = 0;
    std::size_t skippedMeshInstanceCount = 0;
    std::size_t firstPersonMeshCount = 0;
    std::size_t firstPersonBodyPrimitiveCount = 0;
    std::size_t firstPersonSocketCount = 0;
    std::size_t firstPersonCookedSocketCount = 0;
    std::size_t firstPersonSocketAlignedMeshCount = 0;
    std::size_t firstPersonGripSocketBoundCount = 0;
    std::size_t firstPersonArmsSocketBoundCount = 0;
    std::size_t firstPersonFeedbackPrimitiveCount = 0;
    std::size_t hitFeedbackLineCount = 0;
    std::size_t activeLanePressurePrimitiveCount = 0;
    std::size_t targetMeshCount = 0;
    std::size_t materialFallbackProfileCount = 0;
    std::size_t contactShadowCount = 0;
};

// One evaluated fixed-tick snapshot, shared by rendering and hand constraints.
// Marker matrices are asset-scene relative before the single weapon instance.
struct FirstPersonPresentation final {
    player::FirstPersonRigFrame rig{};
    std::string_view weaponAssetId;
    std::string_view armsAssetId;
    std::array<float, 4> weaponColor{};
    std::array<float, 4> armsColor{};
    novacore::animation::Mat4 weaponWorld{};
    std::optional<novacore::animation::Mat4> muzzleWorld;
    std::optional<novacore::animation::Mat4> rightGripWorld;
    std::optional<novacore::animation::Mat4> leftGripWorld;
    std::size_t cookedSocketCount = 0;
    std::size_t alignedMeshCount = 0;
    std::size_t gripBoundCount = 0;
    std::size_t armsBoundCount = 0;
    std::size_t skippedWeaponMeshCount = 0;
};

struct DevRangeRenderSceneDesc final {
    const GreyboxWorld* greyboxWorld = nullptr;
    const DevTargetRangeState* targetRange = nullptr;
    const GreyboxCollisionResult* collision = nullptr;
    const MeshResourceLookup* meshResources = nullptr;
    DevRangePlayerRenderState player{};
    novacore::render::RenderSky sky{
        true,
        {0.07F, 0.18F, 0.36F, 1.0F},
        {0.45F, 0.61F, 0.76F, 1.0F},
        {0.025F, 0.035F, 0.045F, 1.0F},
        0.46F,
        1.45F,
        1.08F,
        1.55F,
        1.65F,
        0.30F,
        0.22F,
    };
    novacore::render::RenderWorldLighting lighting{
        {0.30F, 0.88F, 0.34F},
        0.34F,
        {-0.52F, 0.36F, -0.68F},
        0.22F,
        0.22F,
        0.16F,
        1.12F,
        1.08F,
        true,
        0.46F,
        0.64F,
    };
    bool showWorldDebugLines = true;
    float verticalFovDegrees = 74.0F;
    float nearPlane = 0.03F;
    float farPlane = 120.0F;
    const novacore::render::MeshCatalog* meshCatalog = nullptr;
    bool showFirstPersonRigDiagnostics = false;
    const FirstPersonPresentation* firstPersonPresentation = nullptr;
};

[[nodiscard]] FirstPersonPresentation evaluateFirstPersonPresentation(const DevRangeRenderSceneDesc& desc);

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
        float rollDegrees = 0.0F,
        novacore::render::RenderMeshLayer layer = novacore::render::RenderMeshLayer::World,
        bool castsContactShadow = true,
        novacore::render::MeshResourceHandle resourceOverride = {}) const;

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

    void appendWorldDebugLines(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;
};

} // namespace nemisis::dev

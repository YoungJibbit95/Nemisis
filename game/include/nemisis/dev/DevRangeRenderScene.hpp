#pragma once

#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/player/PlayerComponents.hpp"

#include "novacore/math/Types.hpp"
#include "novacore/render/Renderer.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

namespace nemisis::dev {

using MeshResourceLookup = std::unordered_map<std::string, novacore::render::MeshResourceHandle>;

struct DevRangePlayerRenderState final {
    novacore::math::Vec3 position{};
    player::PlayerViewComponent view{};
    bool hasMovementState = false;
};

struct DevRangeRenderSceneStats final {
    std::size_t worldBoxCount = 0;
    std::size_t meshInstanceCount = 0;
    std::size_t worldLineCount = 0;
    std::size_t skippedMeshInstanceCount = 0;
    std::size_t firstPersonMeshCount = 0;
    std::size_t aimMarkerBoxCount = 0;
};

struct DevRangeRenderSceneDesc final {
    const GreyboxWorld* greyboxWorld = nullptr;
    const DebugTargetState* debugTarget = nullptr;
    const GreyboxCollisionResult* collision = nullptr;
    const MeshResourceLookup* meshResources = nullptr;
    DevRangePlayerRenderState player{};
    novacore::render::RenderWorldLighting lighting{{0.30F, 0.88F, 0.34F}, 0.38F};
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
        DevRangeRenderSceneStats& stats) const;

    void appendWorldGeometry(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;

    void appendStaticShowcaseMeshes(
        novacore::render::RenderFrameInfo& frame,
        const DevRangeRenderSceneDesc& desc,
        DevRangeRenderSceneStats& stats) const;

    void appendFirstPersonMeshes(
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

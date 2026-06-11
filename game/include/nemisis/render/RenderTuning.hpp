#pragma once

#include "novacore/core/ConfigDocument.hpp"
#include "novacore/render/Renderer.hpp"

namespace nemisis::render {

struct DevRenderTuning final {
    novacore::render::RenderWorldLighting lighting{{0.30F, 0.88F, 0.34F}, 0.38F};
    float verticalFovDegrees = 74.0F;
    float nearPlane = 0.03F;
    float farPlane = 120.0F;
    bool showWorldDebugLines = true;
};

[[nodiscard]] DevRenderTuning devRenderTuningFromConfig(
    const novacore::core::ConfigDocument& document,
    DevRenderTuning fallback = {});

} // namespace nemisis::render

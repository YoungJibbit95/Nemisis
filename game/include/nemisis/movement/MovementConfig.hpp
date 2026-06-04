#pragma once

#include "nemisis/movement/MovementTuning.hpp"

#include "novacore/core/ConfigDocument.hpp"

namespace nemisis::movement {

[[nodiscard]] MovementTuning movementTuningFromConfig(
    const novacore::core::ConfigDocument& document,
    MovementTuning fallback = {});

} // namespace nemisis::movement

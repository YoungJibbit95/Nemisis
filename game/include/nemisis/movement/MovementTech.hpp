#pragma once

#include "novacore/math/Types.hpp"

#include <algorithm>
#include <string_view>

namespace nemisis::movement {

enum class MovementTechCue {
    None,
    WallRunGravityArmTrigger,
    GravityBootsActive,
    DoubleJumpEnergyPlatform,
    WallJumpDetach,
    MantleReach,
    MantleClimb
};

struct MovementTechState final {
    bool wallRunArmTriggerPressed = false;
    bool gravityInvertersActive = false;
    bool doubleJumpPlatformThrown = false;
    bool wallJumpDetachTriggered = false;
    bool mantleReachTriggered = false;
    bool mantleClimbTriggered = false;

    float wallRunArmTriggerSeconds = 0.0F;
    float gravityInverterGlowSeconds = 0.0F;
    float energyPlatformSeconds = 0.0F;
    float wallJumpDetachSeconds = 0.0F;
    float mantleReachSeconds = 0.0F;
    float mantleClimbSeconds = 0.0F;

    novacore::math::Vec3 energyPlatformCenter{};
    novacore::math::Vec3 energyPlatformNormal{0.0F, 1.0F, 0.0F};
    novacore::math::Vec3 wallRunNormal{};
    novacore::math::Vec3 mantleTargetPosition{};
    novacore::math::Vec3 mantleNormal{};
};

inline constexpr float kWallRunArmTriggerCueSeconds = 0.24F;
inline constexpr float kGravityInverterGlowCueSeconds = 0.30F;
inline constexpr float kEnergyPlatformCueSeconds = 0.32F;
inline constexpr float kWallJumpDetachCueSeconds = 0.18F;
inline constexpr float kMantleReachCueSeconds = 0.28F;
inline constexpr float kMantleClimbCueSeconds = 0.34F;

[[nodiscard]] inline float consumeTechTimer(float value, float fixedDeltaSeconds) {
    return std::max(0.0F, value - std::max(0.0F, fixedDeltaSeconds));
}

inline void beginMovementTechFrame(MovementTechState& tech, float fixedDeltaSeconds) {
    tech.wallRunArmTriggerPressed = false;
    tech.doubleJumpPlatformThrown = false;
    tech.wallJumpDetachTriggered = false;
    tech.mantleReachTriggered = false;
    tech.mantleClimbTriggered = false;
    tech.wallRunArmTriggerSeconds = consumeTechTimer(tech.wallRunArmTriggerSeconds, fixedDeltaSeconds);
    tech.gravityInverterGlowSeconds = consumeTechTimer(tech.gravityInverterGlowSeconds, fixedDeltaSeconds);
    tech.energyPlatformSeconds = consumeTechTimer(tech.energyPlatformSeconds, fixedDeltaSeconds);
    tech.wallJumpDetachSeconds = consumeTechTimer(tech.wallJumpDetachSeconds, fixedDeltaSeconds);
    tech.mantleReachSeconds = consumeTechTimer(tech.mantleReachSeconds, fixedDeltaSeconds);
    tech.mantleClimbSeconds = consumeTechTimer(tech.mantleClimbSeconds, fixedDeltaSeconds);
}

inline void triggerWallRunGravityTech(MovementTechState& tech, novacore::math::Vec3 wallNormal) {
    tech.wallRunArmTriggerPressed = true;
    tech.gravityInvertersActive = true;
    tech.wallRunArmTriggerSeconds = kWallRunArmTriggerCueSeconds;
    tech.gravityInverterGlowSeconds = kGravityInverterGlowCueSeconds;
    tech.wallRunNormal = wallNormal;
}

inline void keepGravityBootsActive(MovementTechState& tech, novacore::math::Vec3 wallNormal) {
    tech.gravityInvertersActive = true;
    tech.wallRunNormal = wallNormal;
    tech.gravityInverterGlowSeconds = std::max(tech.gravityInverterGlowSeconds, 0.08F);
}

inline void stopGravityBoots(MovementTechState& tech) {
    tech.gravityInvertersActive = false;
}

inline void triggerDoubleJumpPlatform(MovementTechState& tech, novacore::math::Vec3 playerPosition) {
    tech.doubleJumpPlatformThrown = true;
    tech.energyPlatformSeconds = kEnergyPlatformCueSeconds;
    tech.energyPlatformCenter = playerPosition + novacore::math::Vec3{0.0F, -0.18F, 0.0F};
    tech.energyPlatformNormal = {0.0F, 1.0F, 0.0F};
}

inline void triggerWallJumpDetach(MovementTechState& tech) {
    tech.wallJumpDetachTriggered = true;
    tech.wallJumpDetachSeconds = kWallJumpDetachCueSeconds;
    tech.gravityInvertersActive = false;
}

inline void triggerMantleReach(MovementTechState& tech) {
    tech.mantleReachTriggered = true;
    tech.mantleReachSeconds = kMantleReachCueSeconds;
}

inline void triggerMantleClimb(
    MovementTechState& tech,
    novacore::math::Vec3 targetPosition,
    novacore::math::Vec3 normal) {
    tech.mantleClimbTriggered = true;
    tech.mantleReachTriggered = true;
    tech.mantleClimbSeconds = kMantleClimbCueSeconds;
    tech.mantleReachSeconds = std::max(tech.mantleReachSeconds, kMantleReachCueSeconds);
    tech.mantleTargetPosition = targetPosition;
    tech.mantleNormal = normal;
}

[[nodiscard]] inline MovementTechCue dominantMovementTechCue(const MovementTechState& tech) {
    if (tech.doubleJumpPlatformThrown || tech.energyPlatformSeconds > 0.0F) {
        return MovementTechCue::DoubleJumpEnergyPlatform;
    }
    if (tech.mantleClimbTriggered || tech.mantleClimbSeconds > 0.0F) {
        return MovementTechCue::MantleClimb;
    }
    if (tech.wallRunArmTriggerPressed || tech.wallRunArmTriggerSeconds > 0.0F) {
        return MovementTechCue::WallRunGravityArmTrigger;
    }
    if (tech.wallJumpDetachTriggered || tech.wallJumpDetachSeconds > 0.0F) {
        return MovementTechCue::WallJumpDetach;
    }
    if (tech.mantleReachTriggered || tech.mantleReachSeconds > 0.0F) {
        return MovementTechCue::MantleReach;
    }
    if (tech.gravityInvertersActive || tech.gravityInverterGlowSeconds > 0.0F) {
        return MovementTechCue::GravityBootsActive;
    }
    return MovementTechCue::None;
}

[[nodiscard]] inline std::string_view movementTechCueName(MovementTechCue cue) {
    switch (cue) {
    case MovementTechCue::None:
        return "none";
    case MovementTechCue::WallRunGravityArmTrigger:
        return "arm-grav";
    case MovementTechCue::GravityBootsActive:
        return "grav-boots";
    case MovementTechCue::DoubleJumpEnergyPlatform:
        return "energy-step";
    case MovementTechCue::WallJumpDetach:
        return "wall-detach";
    case MovementTechCue::MantleReach:
        return "mantle-reach";
    case MovementTechCue::MantleClimb:
        return "mantle-climb";
    }
    return "unknown";
}

} // namespace nemisis::movement

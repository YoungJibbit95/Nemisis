#pragma once

#include "novacore/animation/Skeleton.hpp"

#include <array>
#include <cstddef>
#include <limits>

namespace nemisis::player {

struct HandIKArm final {
    novacore::animation::JointIndex shoulder = novacore::animation::invalidJointIndex;
    novacore::animation::JointIndex elbow = novacore::animation::invalidJointIndex;
    novacore::animation::JointIndex wrist = novacore::animation::invalidJointIndex;
    std::size_t socketIndex = std::numeric_limits<std::size_t>::max();
    novacore::math::Vec3 poleParent{};
    novacore::math::Vec3 bendAxisMid{};
};

struct PlayerHandConstraintRig final {
    // Right, then left. Calibrated against the selected authored hierarchy.
    std::array<HandIKArm, 2> arms{};
    bool valid = false;
};

struct PlayerHandTargets final {
    novacore::animation::Mat4 armsWorld{};
    novacore::animation::Mat4 rightGripWorld{};
    novacore::animation::Mat4 leftGripWorld{};
};

enum class HandConstraintStatus { Disabled, Applied, Unreachable, Invalid };

struct PlayerHandConstraintResult final {
    HandConstraintStatus status = HandConstraintStatus::Disabled;
    std::array<bool, 2> reached{};
    std::array<float, 2> residualMeters{};
    std::array<float, 2> orientationResidualDegrees{
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()};
    std::array<unsigned, 2> iterations{};

    [[nodiscard]] bool applied() const {
        return status == HandConstraintStatus::Applied || status == HandConstraintStatus::Unreachable;
    }
};

[[nodiscard]] bool buildPlayerHandConstraintRig(
    const novacore::animation::Skeleton& skeleton,
    PlayerHandConstraintRig& outRig);

// NovaCore supplies one sampled base pose and performs final global evaluation.
// Only rotations change. Invalid input leaves both outputs at the base pose;
// unreachable targets retain a bounded pose and report the actual residual.
[[nodiscard]] PlayerHandConstraintResult applyPlayerHandConstraints(
    const PlayerHandConstraintRig& rig,
    const novacore::animation::Skeleton& skeleton,
    const novacore::animation::LocalPose& baseLocal,
    const novacore::animation::GlobalPose& baseGlobal,
    const PlayerHandTargets& targets,
    novacore::animation::LocalPose& outLocal,
    novacore::animation::GlobalPose& outGlobal);

} // namespace nemisis::player

#pragma once

#include "nemisis/player/PlayerHandConstraints.hpp"

namespace nemisis::player::detail {

[[nodiscard]] bool buildProjectHandConstraintRig(const novacore::animation::Skeleton& skeleton,
                                                 PlayerHandConstraintRig& rig);

[[nodiscard]] PlayerHandConstraintResult
applyProjectHandConstraints(const PlayerHandConstraintRig& rig, const novacore::animation::Skeleton& skeleton,
                            const novacore::animation::LocalPose& local, const novacore::animation::GlobalPose& global,
                            const PlayerHandTargets& targets, novacore::animation::LocalPose& outLocal,
                            novacore::animation::GlobalPose& outGlobal);

} // namespace nemisis::player::detail

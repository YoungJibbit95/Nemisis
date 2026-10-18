#include "nemisis/player/PlayerHandConstraints.hpp"
#include "ProjectHandConstraints.hpp"

#include "ozz/animation/runtime/ik_two_bone_job.h"
#include "ozz/base/maths/simd_quaternion.h"

#include <algorithm>
#include <cmath>
#include <string_view>

namespace nemisis::player {
namespace {
using namespace novacore;

constexpr float kLengthEpsilon = 0.000001F;
constexpr float kFrameTolerance = 0.0001F;

float dot(math::Vec3 a, math::Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
math::Vec3 cross(math::Vec3 a, math::Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
bool unit(math::Vec3 value, math::Vec3& out) {
    const float squared = value.lengthSquared();
    if (!animation::isFinite(value) || !std::isfinite(squared) || squared <= kLengthEpsilon * kLengthEpsilon) {
        return false;
    }
    out = value * (1.0F / std::sqrt(squared));
    return true;
}
bool uniformPositive(math::Vec3 scale) {
    return animation::isFinite(scale) && scale.x > kLengthEpsilon &&
        std::abs(scale.x - scale.y) <= kFrameTolerance * scale.x &&
        std::abs(scale.x - scale.z) <= kFrameTolerance * scale.x;
}

struct UniformFrame final {
    math::Vec3 position{};
    math::Quat rotation{};
    float scale = 1.0F;
};

// Accept an affine rotation with positive uniform scale, never shear/reflection.
bool frameFromMatrix(const animation::Mat4& matrix, UniformFrame& out) {
    if (!std::ranges::all_of(matrix.values, [](float value) { return std::isfinite(value); }) ||
        std::abs(matrix.at(3, 0)) > kFrameTolerance || std::abs(matrix.at(3, 1)) > kFrameTolerance ||
        std::abs(matrix.at(3, 2)) > kFrameTolerance || std::abs(matrix.at(3, 3) - 1.0F) > kFrameTolerance) {
        return false;
    }
    const math::Vec3 x{matrix.at(0, 0), matrix.at(1, 0), matrix.at(2, 0)};
    const math::Vec3 y{matrix.at(0, 1), matrix.at(1, 1), matrix.at(2, 1)};
    const math::Vec3 z{matrix.at(0, 2), matrix.at(1, 2), matrix.at(2, 2)};
    const math::Vec3 scale{std::sqrt(x.lengthSquared()), std::sqrt(y.lengthSquared()), std::sqrt(z.lengthSquared())};
    if (!uniformPositive(scale)) {
        return false;
    }
    const auto a = x * (1.0F / scale.x);
    const auto b = y * (1.0F / scale.y);
    const auto c = z * (1.0F / scale.z);
    if (std::abs(dot(a, b)) > kFrameTolerance || std::abs(dot(a, c)) > kFrameTolerance ||
        std::abs(dot(b, c)) > kFrameTolerance || dot(cross(a, b), c) < 1.0F - kFrameTolerance) {
        return false;
    }
    math::Quat q;
    const float trace = a.x + b.y + c.z;
    if (trace > 0.0F) {
        const float s = std::sqrt(trace + 1.0F) * 2.0F;
        q = {(b.z - c.y) / s, (c.x - a.z) / s, (a.y - b.x) / s, s * 0.25F};
    } else if (a.x > b.y && a.x > c.z) {
        const float s = std::sqrt(1.0F + a.x - b.y - c.z) * 2.0F;
        q = {s * 0.25F, (b.x + a.y) / s, (c.x + a.z) / s, (b.z - c.y) / s};
    } else if (b.y > c.z) {
        const float s = std::sqrt(1.0F + b.y - a.x - c.z) * 2.0F;
        q = {(b.x + a.y) / s, s * 0.25F, (c.y + b.z) / s, (c.x - a.z) / s};
    } else {
        const float s = std::sqrt(1.0F + c.z - a.x - b.y) * 2.0F;
        q = {(c.x + a.z) / s, (c.y + b.z) / s, s * 0.25F, (a.y - b.x) / s};
    }
    if (!animation::isFinite(q) || animation::quaternionLengthSquared(q) <= kLengthEpsilon) {
        return false;
    }
    out = {animation::matrixTranslation(matrix), animation::normalize(q), scale.x};
    return true;
}

bool validArm(const HandIKArm& arm, const animation::Skeleton& skeleton) {
    const auto count = skeleton.joints.size();
    return arm.shoulder < count && arm.elbow < count && arm.wrist < count &&
        arm.shoulder != arm.elbow && arm.elbow != arm.wrist && arm.shoulder != arm.wrist &&
        arm.socketIndex < skeleton.sockets.size() && skeleton.sockets[arm.socketIndex].joint == arm.wrist &&
        skeleton.joints[arm.elbow].parent == static_cast<std::int32_t>(arm.shoulder) &&
        skeleton.joints[arm.wrist].parent == static_cast<std::int32_t>(arm.elbow);
}

bool independentArms(const PlayerHandConstraintRig& rig, const animation::Skeleton& skeleton) {
    for (std::size_t side = 0; side < rig.arms.size(); ++side) {
        const auto& other = rig.arms[1U - side];
        int joint = static_cast<int>(rig.arms[side].shoulder);
        while (joint >= 0) {
            const auto index = static_cast<animation::JointIndex>(joint);
            if (index == other.shoulder || index == other.elbow || index == other.wrist) {
                return false;
            }
            joint = skeleton.joints[index].parent;
        }
    }
    return true;
}

ozz::math::Float4x4 toOzz(const animation::Mat4& matrix) {
    ozz::math::Float4x4 result;
    for (std::size_t column = 0; column < 4; ++column) {
        result.cols[column] = ozz::math::simd_float4::Load(
            matrix.at(0, column), matrix.at(1, column), matrix.at(2, column), matrix.at(3, column));
    }
    return result;
}
math::Quat fromOzz(const ozz::math::SimdQuaternion& value) {
    float elements[4];
    ozz::math::StorePtrU(value.xyzw, elements);
    return {elements[0], elements[1], elements[2], elements[3]};
}

} // namespace

bool buildPlayerHandConstraintRig(const animation::Skeleton& skeleton, PlayerHandConstraintRig& outRig) {
    outRig = {};
    if (!animation::validateSkeleton(skeleton).valid()) {
        return false;
    }
    if (animation::findJoint(skeleton, "DEF-upper_arm.R")) {
        return detail::buildProjectHandConstraintRig(skeleton, outRig);
    }
    PlayerHandConstraintRig candidate;
    constexpr std::array names{
        std::array<std::string_view, 4>{"upper_arm.R", "lower_arm.R", "hand.R", "socket_hand_r"},
        std::array<std::string_view, 4>{"upper_arm.L", "lower_arm.L", "hand.L", "socket_hand_l"}};
    for (std::size_t side = 0; side < names.size(); ++side) {
        auto& arm = candidate.arms[side];
        for (std::size_t part = 0; part < 3U; ++part) {
            if (std::ranges::count_if(skeleton.joints, [&](const auto& joint) {
                    return joint.name == names[side][part];
                }) != 1) {
                return false;
            }
        }
        if (std::ranges::count_if(skeleton.sockets, [&](const auto& socket) {
                return socket.name == names[side][3];
            }) != 1) {
            return false;
        }
        const auto shoulder = animation::findJoint(skeleton, names[side][0]);
        const auto elbow = animation::findJoint(skeleton, names[side][1]);
        const auto wrist = animation::findJoint(skeleton, names[side][2]);
        const auto socket = animation::findSocket(skeleton, names[side][3]);
        if (!shoulder || !elbow || !wrist || !socket) {
            return false;
        }
        arm.shoulder = *shoulder;
        arm.elbow = *elbow;
        arm.wrist = *wrist;
        arm.socketIndex = *socket;
        if (!validArm(arm, skeleton)) {
            return false;
        }
    }
    if (!independentArms(candidate, skeleton)) {
        return false;
    }
    animation::GlobalPose bind;
    if (!animation::evaluateGlobalPose(skeleton, animation::makeBindPose(skeleton), bind)) {
        return false;
    }
    for (auto& arm : candidate.arms) {
        UniformFrame shoulder, elbow, wrist, parent;
        const auto parentIndex = skeleton.joints[arm.shoulder].parent;
        if (!frameFromMatrix(bind.joints[arm.shoulder], shoulder) || !frameFromMatrix(bind.joints[arm.elbow], elbow) ||
            !frameFromMatrix(bind.joints[arm.wrist], wrist) ||
            (parentIndex >= 0 && !frameFromMatrix(bind.joints[static_cast<std::size_t>(parentIndex)], parent))) {
            return false;
        }
        const auto upper = elbow.position - shoulder.position;
        const auto lower = wrist.position - elbow.position;
        math::Vec3 direction, plane, pole;
        if (!unit(wrist.position - shoulder.position, direction) || !unit(cross(upper, lower) * -1.0F, plane) ||
            !unit(upper - direction * dot(upper, direction), pole)) {
            return false;
        }
        arm.poleParent = animation::rotate(animation::conjugate(parent.rotation), pole);
        arm.bendAxisMid = animation::rotate(animation::conjugate(elbow.rotation), plane);
    }
    candidate.valid = true;
    outRig = candidate;
    return true;
}

PlayerHandConstraintResult applyPlayerHandConstraints(
    const PlayerHandConstraintRig& rig, const animation::Skeleton& skeleton,
    const animation::LocalPose& baseLocal, const animation::GlobalPose& baseGlobal,
    const PlayerHandTargets& targets, animation::LocalPose& outLocal, animation::GlobalPose& outGlobal) {
    if (animation::findJoint(skeleton, "DEF-upper_arm.R")) {
        return detail::applyProjectHandConstraints(rig, skeleton, baseLocal, baseGlobal, targets, outLocal, outGlobal);
    }
    // The normal caller uses distinct reusable outputs. Preserve transactional
    // behavior for an explicitly in-place call as well.
    animation::LocalPose aliasLocal;
    animation::GlobalPose aliasGlobal;
    const auto* local = &baseLocal;
    const auto* global = &baseGlobal;
    if (&baseLocal == &outLocal) { aliasLocal = baseLocal; local = &aliasLocal; }
    if (&baseGlobal == &outGlobal) { aliasGlobal = baseGlobal; global = &aliasGlobal; }
    outLocal = *local;
    outGlobal = *global;
    const auto invalid = [&]() {
        outLocal = *local;
        outGlobal = *global;
        PlayerHandConstraintResult result;
        result.status = HandConstraintStatus::Invalid;
        return result;
    };
    if (!rig.valid || !animation::validateSkeleton(skeleton).valid() || !animation::validatePose(skeleton, *local).valid() ||
        global->joints.size() != skeleton.joints.size() || !validArm(rig.arms[0], skeleton) ||
        !validArm(rig.arms[1], skeleton) || !independentArms(rig, skeleton)) {
        return invalid();
    }
    for (std::size_t joint = 0; joint < local->joints.size(); ++joint) {
        UniformFrame checked;
        if (!uniformPositive(local->joints[joint].scale) || !frameFromMatrix(global->joints[joint], checked)) {
            return invalid();
        }
    }
    UniformFrame armsWorld;
    if (!frameFromMatrix(targets.armsWorld, armsWorld)) {
        return invalid();
    }
    PlayerHandConstraintResult result;
    result.status = HandConstraintStatus::Applied;
    const std::array grips{targets.rightGripWorld, targets.leftGripWorld};
    for (std::size_t side = 0; side < rig.arms.size(); ++side) {
        const auto& arm = rig.arms[side];
        const auto& socket = skeleton.sockets[arm.socketIndex];
        UniformFrame grip, shoulder, elbow, wrist, parent;
        const auto parentIndex = skeleton.joints[arm.shoulder].parent;
        math::Vec3 pole, referenceAxis;
        if (!animation::isFinite(socket.local) || !uniformPositive(socket.local.scale) ||
            animation::quaternionLengthSquared(socket.local.rotation) <= kLengthEpsilon ||
            !frameFromMatrix(grips[side], grip) || !frameFromMatrix(global->joints[arm.shoulder], shoulder) ||
            !frameFromMatrix(global->joints[arm.elbow], elbow) || !frameFromMatrix(global->joints[arm.wrist], wrist) ||
            (parentIndex >= 0 && !frameFromMatrix(global->joints[static_cast<std::size_t>(parentIndex)], parent)) ||
            !unit(arm.poleParent, pole) || !unit(arm.bendAxisMid, referenceAxis)) {
            return invalid();
        }
        pole = animation::rotate(parent.rotation, pole);
        const auto desiredHand = animation::multiply(animation::multiply(animation::conjugate(armsWorld.rotation),
            grip.rotation), animation::conjugate(animation::normalize(socket.local.rotation)));
        const auto gripModel = animation::rotate(animation::conjugate(armsWorld.rotation), grip.position - armsWorld.position) *
            (1.0F / armsWorld.scale);
        const auto wristGoal = gripModel - animation::rotate(desiredHand, socket.local.translation * wrist.scale);
        const auto upper = elbow.position - shoulder.position;
        const auto lower = wrist.position - elbow.position;
        math::Vec3 upperDirection, lowerDirection;
        if (!animation::isFinite(wristGoal) || !unit(upper, upperDirection) || !unit(lower, lowerDirection)) {
            return invalid();
        }
        math::Vec3 axis;
        const auto bend = cross(upperDirection, lowerDirection) * -1.0F;
        if (bend.lengthSquared() > 0.00000001F && unit(bend, axis)) {
            axis = animation::rotate(animation::conjugate(elbow.rotation), axis);
            if (dot(axis, referenceAxis) < 0.0F) { axis = axis * -1.0F; }
        } else {
            axis = referenceAxis;
        }
        auto solverGoal = wristGoal;
        math::Vec3 targetDirection;
        if (!unit(wristGoal - shoulder.position, targetDirection)) {
            // A coincident target cannot define a rotation axis. Keep the
            // authored shoulder-to-wrist direction for the closest folded pose.
            if (!unit(wrist.position - shoulder.position, targetDirection)) { targetDirection = upperDirection; }
            solverGoal = shoulder.position + targetDirection * (kLengthEpsilon * 2.0F);
        }
        if (cross(targetDirection, pole).lengthSquared() <= kLengthEpsilon) {
            // At the pole singularity, use the other authored bend-plane axis.
            // This remains deterministic and never feeds a zero plane to ozz.
            pole = animation::rotate(elbow.rotation, referenceAxis);
            if (cross(targetDirection, pole).lengthSquared() <= kLengthEpsilon) { return invalid(); }
        }
        const auto m0 = toOzz(global->joints[arm.shoulder]);
        const auto m1 = toOzz(global->joints[arm.elbow]);
        const auto m2 = toOzz(global->joints[arm.wrist]);
        ozz::math::SimdQuaternion q0, q1;
        ozz::animation::IKTwoBoneJob job;
        job.start_joint = &m0;
        job.mid_joint = &m1;
        job.end_joint = &m2;
        job.target = ozz::math::simd_float4::Load(solverGoal.x, solverGoal.y, solverGoal.z, 0);
        job.mid_axis = ozz::math::simd_float4::Load(axis.x, axis.y, axis.z, 0);
        job.pole_vector = ozz::math::simd_float4::Load(pole.x, pole.y, pole.z, 0);
        job.start_joint_correction = &q0;
        job.mid_joint_correction = &q1;
        job.reached = &result.reached[side];
        if (!job.Run()) { return invalid(); }
        const auto first = fromOzz(q0);
        const auto second = fromOzz(q1);
        if (!animation::isFinite(first) || !animation::isFinite(second) ||
            animation::quaternionLengthSquared(first) <= kLengthEpsilon ||
            animation::quaternionLengthSquared(second) <= kLengthEpsilon) {
            return invalid();
        }
        outLocal.joints[arm.shoulder].rotation = animation::multiply(local->joints[arm.shoulder].rotation, first);
        outLocal.joints[arm.elbow].rotation = animation::multiply(local->joints[arm.elbow].rotation, second);
        const auto correctedParent = animation::multiply(animation::multiply(parent.rotation,
            outLocal.joints[arm.shoulder].rotation), outLocal.joints[arm.elbow].rotation);
        outLocal.joints[arm.wrist].rotation = animation::multiply(animation::conjugate(correctedParent), desiredHand);
    }
    if (!animation::evaluateGlobalPose(skeleton, outLocal, outGlobal)) {
        return invalid();
    }
    for (std::size_t side = 0; side < rig.arms.size(); ++side) {
        animation::Mat4 socket;
        if (!animation::evaluateSocket(skeleton, outGlobal, rig.arms[side].socketIndex, socket)) { return invalid(); }
        const auto actual = animation::transformPoint(targets.armsWorld, animation::matrixTranslation(socket));
        result.residualMeters[side] = std::sqrt((actual - animation::matrixTranslation(grips[side])).lengthSquared());
        if (!std::isfinite(result.residualMeters[side])) { return invalid(); }
        result.reached[side] = result.reached[side] && result.residualMeters[side] < 0.001F;
        if (!result.reached[side]) { result.status = HandConstraintStatus::Unreachable; }
    }
    return result;
}

} // namespace nemisis::player

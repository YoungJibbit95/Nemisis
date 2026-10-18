#include "ProjectHandConstraints.hpp"

#include "ozz/animation/runtime/ik_two_bone_job.h"
#include "ozz/base/maths/simd_quaternion.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace nemisis::player::detail {
namespace {
using namespace novacore;
using M3 = std::array<double, 9>;
constexpr float kPositionTolerance = 0.001F;
constexpr double kOrientationTolerance = 0.5 * 3.141592653589793 / 180.0;
constexpr int kMaximumPasses = 4;

float dot(math::Vec3 a, math::Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
math::Vec3 cross(math::Vec3 a, math::Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
bool unit(math::Vec3 v, math::Vec3& out) {
    const auto n = v.lengthSquared();
    if (!animation::isFinite(v) || !std::isfinite(n) || n < 1.e-12F) {
        return false;
    }
    out = v * (1.0F / std::sqrt(n));
    return true;
}
bool positive(math::Vec3 s) { return animation::isFinite(s) && s.x > 1.e-6F && s.y > 1.e-6F && s.z > 1.e-6F; }
M3 linear(const animation::Mat4& m) {
    return {m.at(0, 0), m.at(0, 1), m.at(0, 2), m.at(1, 0), m.at(1, 1), m.at(1, 2), m.at(2, 0), m.at(2, 1), m.at(2, 2)};
}
M3 transpose(const M3& m) { return {m[0], m[3], m[6], m[1], m[4], m[7], m[2], m[5], m[8]}; }
M3 multiply(const M3& a, const M3& b) {
    M3 r{};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            for (int k = 0; k < 3; ++k) {
                r[i * 3 + j] += a[i * 3 + k] * b[k * 3 + j];
            }
        }
    }
    return r;
}
math::Vec3 transform(const M3& m, math::Vec3 v) {
    return {static_cast<float>(m[0] * v.x + m[1] * v.y + m[2] * v.z),
            static_cast<float>(m[3] * v.x + m[4] * v.y + m[5] * v.z),
            static_cast<float>(m[6] * v.x + m[7] * v.y + m[8] * v.z)};
}
double determinant(const M3& m) {
    return m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6]) + m[2] * (m[3] * m[7] - m[4] * m[6]);
}
double normSquared(const M3& m) {
    double r = 0;
    for (double x : m) {
        r += x * x;
    }
    return r;
}
bool inverse(const M3& m, M3& out) {
    const auto det = determinant(m);
    if (!std::isfinite(det) || det <= 1.e-12) {
        return false;
    }
    out = {m[4] * m[8] - m[5] * m[7], m[2] * m[7] - m[1] * m[8], m[1] * m[5] - m[2] * m[4],
           m[5] * m[6] - m[3] * m[8], m[0] * m[8] - m[2] * m[6], m[2] * m[3] - m[0] * m[5],
           m[3] * m[7] - m[4] * m[6], m[1] * m[6] - m[0] * m[7], m[0] * m[4] - m[1] * m[3]};
    for (auto& x : out) {
        x /= det;
    }
    return std::isfinite(normSquared(out)) && normSquared(m) * normSquared(out) < 1.e8;
}
bool affineInverse(const animation::Mat4& m, animation::Mat4& out) {
    if (!std::ranges::all_of(m.values, [](float x) { return std::isfinite(x); }) ||
        std::abs(m.at(3, 0)) + std::abs(m.at(3, 1)) + std::abs(m.at(3, 2)) + std::abs(m.at(3, 3) - 1) > 1.e-5F) {
        return false;
    }
    M3 inv;
    if (!inverse(linear(m), inv)) {
        return false;
    }
    out = {};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            out.at(i, j) = static_cast<float>(inv[i * 3 + j]);
        }
    }
    const auto t = transform(inv, animation::matrixTranslation(m)) * -1.0F;
    out.at(0, 3) = t.x;
    out.at(1, 3) = t.y;
    out.at(2, 3) = t.z;
    return true;
}
// Polar rotation retains the orientation of a sheared/nonuniform affine frame.
// Column normalization would silently discard the actual wrist-parent metric.
bool polar(M3 x, M3& rotation) {
    M3 inv;
    if (!inverse(x, inv)) {
        return false;
    }
    const auto scale = std::sqrt(normSquared(x) / 3.0);
    for (auto& v : x) {
        v /= scale;
    }
    for (int iteration = 0; iteration < 12; ++iteration) {
        if (!inverse(x, inv)) {
            return false;
        }
        inv = transpose(inv);
        double delta = 0;
        for (int i = 0; i < 9; ++i) {
            const auto next = 0.5 * (x[i] + inv[i]);
            delta = std::max(delta, std::abs(next - x[i]));
            x[i] = next;
        }
        if (delta < 1.e-10) {
            break;
        }
    }
    const auto check = multiply(transpose(x), x);
    double error = 0;
    for (int i = 0; i < 9; ++i) {
        error = std::max(error, std::abs(check[i] - (i % 4 == 0 ? 1.0 : 0.0)));
    }
    if (error > 1.e-5 || determinant(x) < 0.99999) {
        return false;
    }
    rotation = x;
    return true;
}
math::Quat quaternion(const M3& m) {
    math::Quat q;
    const double trace = m[0] + m[4] + m[8];
    if (trace > 0) {
        const double s = 2 * std::sqrt(trace + 1);
        q = {float((m[7] - m[5]) / s), float((m[2] - m[6]) / s), float((m[3] - m[1]) / s), float(s / 4)};
    } else if (m[0] > m[4] && m[0] > m[8]) {
        const double s = 2 * std::sqrt(1 + m[0] - m[4] - m[8]);
        q = {float(s / 4), float((m[1] + m[3]) / s), float((m[2] + m[6]) / s), float((m[7] - m[5]) / s)};
    } else if (m[4] > m[8]) {
        const double s = 2 * std::sqrt(1 + m[4] - m[0] - m[8]);
        q = {float((m[1] + m[3]) / s), float(s / 4), float((m[5] + m[7]) / s), float((m[2] - m[6]) / s)};
    } else {
        const double s = 2 * std::sqrt(1 + m[8] - m[0] - m[4]);
        q = {float((m[2] + m[6]) / s), float((m[5] + m[7]) / s), float(s / 4), float((m[3] - m[1]) / s)};
    }
    return animation::normalize(q);
}
animation::Mat4 preScale(const animation::Mat4& global, math::Vec3 scale) {
    auto frame = global;
    const std::array s{scale.x, scale.y, scale.z};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            frame.at(r, c) /= s[c];
        }
    }
    return frame;
}
ozz::math::Float4x4 toOzz(const animation::Mat4& m) {
    ozz::math::Float4x4 out;
    for (int c = 0; c < 4; ++c) {
        out.cols[c] = ozz::math::simd_float4::Load(m.at(0, c), m.at(1, c), m.at(2, c), m.at(3, c));
    }
    return out;
}
math::Quat fromOzz(const ozz::math::SimdQuaternion& q) {
    float p[4];
    ozz::math::StorePtrU(q.xyzw, p);
    return animation::normalize({p[0], p[1], p[2], p[3]});
}
bool chain(const animation::Skeleton& skeleton, const HandIKArm& arm, std::string_view side) {
    const std::array names{
        std::string("DEF-upper_arm.") + std::string(side), std::string("DEF-upper_arm.") + std::string(side) + ".001",
        std::string("DEF-forearm.") + std::string(side), std::string("DEF-forearm.") + std::string(side) + ".001",
        std::string("DEF-hand.") + std::string(side)};
    std::array<animation::JointIndex, 5> joints;
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (std::ranges::count_if(skeleton.joints, [&](const auto& joint) { return joint.name == names[i]; }) != 1) {
            return false;
        }
        const auto j = animation::findJoint(skeleton, names[i]);
        if (!j) {
            return false;
        }
        joints[i] = *j;
        if (i > 0 && skeleton.joints[*j].parent != static_cast<int>(joints[i - 1])) {
            return false;
        }
    }
    return arm.shoulder == joints[0] && arm.elbow == joints[2] && arm.wrist == joints[4] &&
           arm.socketIndex < skeleton.sockets.size() && skeleton.sockets[arm.socketIndex].joint == arm.wrist;
}
bool orientWrist(const animation::Skeleton& skeleton, const HandIKArm& arm, const M3& desired, const M3& modelWorld,
                 animation::LocalPose& local, const animation::GlobalPose& global, math::Vec3& offset) {
    const auto parent = skeleton.joints[arm.wrist].parent;
    if (parent < 0) {
        return false;
    }
    const auto& wrist = local.joints[arm.wrist];
    const auto& socket = skeleton.sockets[arm.socketIndex].local;
    if (!positive(wrist.scale) || !positive(socket.scale)) {
        return false;
    }
    M3 invParent, rotation;
    const auto parentLinear = linear(global.joints[static_cast<std::size_t>(parent)]);
    const auto p = multiply(modelWorld, parentLinear);
    auto b = linear(animation::matrixFromTransform(socket));
    const std::array scales{wrist.scale.x, wrist.scale.y, wrist.scale.z};
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            b[r * 3 + c] *= scales[r];
        }
    }
    if (!inverse(p, invParent) || !polar(multiply(multiply(invParent, desired), transpose(b)), rotation)) {
        return false;
    }
    local.joints[arm.wrist].rotation = quaternion(rotation);
    offset =
        transform(multiply(parentLinear,
                           linear(animation::matrixFromTransform({{}, local.joints[arm.wrist].rotation, wrist.scale}))),
                  socket.translation);
    return animation::isFinite(offset);
}
} // namespace

bool buildProjectHandConstraintRig(const animation::Skeleton& skeleton, PlayerHandConstraintRig& outRig) {
    outRig = {};
    PlayerHandConstraintRig rig;
    if (!animation::validateSkeleton(skeleton).valid()) {
        return false;
    }
    const auto local = animation::makeBindPose(skeleton);
    animation::GlobalPose global;
    if (!animation::evaluateGlobalPose(skeleton, local, global)) {
        return false;
    }
    for (std::size_t side = 0; side < 2; ++side) {
        const std::string suffix = side == 0 ? "R" : "L";
        auto& a = rig.arms[side];
        const auto shoulder = animation::findJoint(skeleton, "DEF-upper_arm." + suffix),
                   elbow = animation::findJoint(skeleton, "DEF-forearm." + suffix),
                   wrist = animation::findJoint(skeleton, "DEF-hand." + suffix);
        auto socket = animation::findSocket(skeleton, side == 0 ? "socket_palm_r" : "socket_palm_l");
        if (!socket) {
            socket = animation::findSocket(skeleton, side == 0 ? "socket_palm_r_proposal" : "socket_palm_l_proposal");
        }
        if (!shoulder || !elbow || !wrist || !socket) {
            return false;
        }
        a.shoulder = *shoulder;
        a.elbow = *elbow;
        a.wrist = *wrist;
        a.socketIndex = *socket;
        if (!chain(skeleton, a, suffix)) {
            return false;
        }
        const auto s = animation::matrixTranslation(global.joints[a.shoulder]),
                   e = animation::matrixTranslation(global.joints[a.elbow]),
                   w = animation::matrixTranslation(global.joints[a.wrist]);
        math::Vec3 direction, pole;
        animation::Mat4 invMid, invParent;
        const auto parent = skeleton.joints[a.shoulder].parent;
        if (parent < 0 || !positive(local.joints[a.elbow].scale) || !unit(w - s, direction) ||
            !unit((e - s) - direction * dot(e - s, direction), pole) ||
            !affineInverse(global.joints[static_cast<std::size_t>(parent)], invParent) ||
            !affineInverse(preScale(global.joints[a.elbow], local.joints[a.elbow].scale), invMid) ||
            !unit(cross(animation::transformPoint(invMid, s), animation::transformPoint(invMid, w)), a.bendAxisMid)) {
            return false;
        }
        a.poleParent = animation::transformDirection(invParent, pole);
    }
    for (std::size_t side = 0; side < 2; ++side) {
        const auto& other = rig.arms[1 - side];
        auto parent = skeleton.joints[rig.arms[side].shoulder].parent;
        while (parent >= 0) {
            if (parent == static_cast<int>(other.shoulder) || parent == static_cast<int>(other.elbow) ||
                parent == static_cast<int>(other.wrist)) {
                return false;
            }
            parent = skeleton.joints[static_cast<std::size_t>(parent)].parent;
        }
    }
    rig.valid = true;
    outRig = rig;
    return true;
}

PlayerHandConstraintResult applyProjectHandConstraints(const PlayerHandConstraintRig& rig,
                                                       const animation::Skeleton& skeleton,
                                                       const animation::LocalPose& baseLocal,
                                                       const animation::GlobalPose& baseGlobal,
                                                       const PlayerHandTargets& targets, animation::LocalPose& outLocal,
                                                       animation::GlobalPose& outGlobal) {
    const auto localCopy = baseLocal;
    const auto globalCopy = baseGlobal;
    outLocal = localCopy;
    outGlobal = globalCopy;
    const auto invalid = [&]() {
        outLocal = localCopy;
        outGlobal = globalCopy;
        PlayerHandConstraintResult r;
        r.status = HandConstraintStatus::Invalid;
        return r;
    };
    animation::Mat4 invWorld;
    if (!rig.valid || !animation::validateSkeleton(skeleton).valid() ||
        !animation::validatePose(skeleton, localCopy).valid() || globalCopy.joints.size() != skeleton.joints.size() ||
        !chain(skeleton, rig.arms[0], "R") || !chain(skeleton, rig.arms[1], "L") ||
        !affineInverse(targets.armsWorld, invWorld)) {
        return invalid();
    }
    PlayerHandConstraintResult result;
    result.status = HandConstraintStatus::Applied;
    const std::array grips{targets.rightGripWorld, targets.leftGripWorld};
    const auto modelWorld = linear(targets.armsWorld);
    for (std::size_t side = 0; side < 2; ++side) {
        const auto& a = rig.arms[side];
        const auto modelGrip = animation::multiply(invWorld, grips[side]);
        M3 desired;
        if (!polar(linear(grips[side]), desired)) {
            return invalid();
        }
        const auto target = animation::matrixTranslation(modelGrip);
        // Preserve the authored elbow plane. The bind calibration is only a
        // fallback for a straight/degenerate base arm, never a T-pose override.
        const auto startPosition = animation::matrixTranslation(baseGlobal.joints[a.shoulder]);
        const auto elbowPosition = animation::matrixTranslation(baseGlobal.joints[a.elbow]);
        const auto wristPosition = animation::matrixTranslation(baseGlobal.joints[a.wrist]);
        math::Vec3 direction, authoredPole;
        const auto parent = skeleton.joints[a.shoulder].parent;
        if (!unit(wristPosition - startPosition, direction) ||
            !unit((elbowPosition - startPosition) - direction * dot(elbowPosition - startPosition, direction),
                  authoredPole)) {
            if (parent < 0 ||
                !unit(animation::transformDirection(baseGlobal.joints[static_cast<std::size_t>(parent)], a.poleParent),
                      authoredPole)) {
                return invalid();
            }
        }
        auto bestLocal = outLocal;
        auto bestGlobal = outGlobal;
        double bestScore = 1.e30;
        for (int pass = 0; pass < kMaximumPasses; ++pass) {
            math::Vec3 offset;
            if (!orientWrist(skeleton, a, desired, modelWorld, outLocal, outGlobal, offset) ||
                !positive(outLocal.joints[a.shoulder].scale) || !positive(outLocal.joints[a.elbow].scale)) {
                return invalid();
            }
            const auto start = preScale(outGlobal.joints[a.shoulder], outLocal.joints[a.shoulder].scale);
            const auto mid = preScale(outGlobal.joints[a.elbow], outLocal.joints[a.elbow].scale);
            animation::Mat4 invMid;
            if (!affineInverse(mid, invMid)) {
                return invalid();
            }
            math::Vec3 axis;
            if (!unit(cross(animation::transformPoint(invMid, animation::matrixTranslation(start)),
                            animation::transformPoint(invMid, animation::matrixTranslation(outGlobal.joints[a.wrist]))),
                      axis)) {
                axis = a.bendAxisMid;
            }
            if (dot(axis, a.bendAxisMid) < 0) {
                axis = axis * -1.0F;
            }
            const auto goal = target - offset;
            const auto m0 = toOzz(start), m1 = toOzz(mid), m2 = toOzz(outGlobal.joints[a.wrist]);
            ozz::math::SimdQuaternion q0, q1;
            ozz::animation::IKTwoBoneJob job;
            job.start_joint = &m0;
            job.mid_joint = &m1;
            job.end_joint = &m2;
            job.target = ozz::math::simd_float4::Load(goal.x, goal.y, goal.z, 0);
            job.mid_axis = ozz::math::simd_float4::Load(axis.x, axis.y, axis.z, 0);
            job.pole_vector = ozz::math::simd_float4::Load(authoredPole.x, authoredPole.y, authoredPole.z, 0);
            job.start_joint_correction = &q0;
            job.mid_joint_correction = &q1;
            if (!job.Run()) {
                return invalid();
            }
            outLocal.joints[a.shoulder].rotation =
                animation::multiply(outLocal.joints[a.shoulder].rotation, fromOzz(q0));
            outLocal.joints[a.elbow].rotation = animation::multiply(outLocal.joints[a.elbow].rotation, fromOzz(q1));
            if (!animation::evaluateGlobalPose(skeleton, outLocal, outGlobal) ||
                !orientWrist(skeleton, a, desired, modelWorld, outLocal, outGlobal, offset) ||
                !animation::evaluateGlobalPose(skeleton, outLocal, outGlobal)) {
                return invalid();
            }
            animation::Mat4 palm;
            if (!animation::evaluateSocket(skeleton, outGlobal, a.socketIndex, palm)) {
                return invalid();
            }
            M3 actual;
            if (!polar(multiply(modelWorld, linear(palm)), actual)) {
                return invalid();
            }
            const auto relative = multiply(transpose(actual), desired);
            const auto angle = std::acos(std::clamp((relative[0] + relative[4] + relative[8] - 1) * 0.5, -1.0, 1.0));
            const auto residual =
                std::sqrt((animation::transformPoint(targets.armsWorld, animation::matrixTranslation(palm)) -
                           animation::matrixTranslation(grips[side]))
                              .lengthSquared());
            const auto score = residual + 0.02 * angle;
            if (!std::isfinite(score)) {
                return invalid();
            }
            if (score < bestScore) {
                bestScore = score;
                bestLocal = outLocal;
                bestGlobal = outGlobal;
                result.residualMeters[side] = residual;
                result.orientationResidualDegrees[side] = static_cast<float>(angle * 180.0 / 3.141592653589793);
                result.iterations[side] = static_cast<unsigned>(pass + 1);
                result.reached[side] = residual <= kPositionTolerance && angle <= kOrientationTolerance;
            }
            if (result.reached[side]) {
                break;
            }
        }
        outLocal = std::move(bestLocal);
        outGlobal = std::move(bestGlobal);
        if (!result.reached[side]) {
            result.status = HandConstraintStatus::Unreachable;
        }
    }
    return result;
}
} // namespace nemisis::player::detail

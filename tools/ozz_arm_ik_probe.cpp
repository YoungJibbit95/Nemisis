#include "novacore/assets/GltfAnimationBridge.hpp"

#include "ozz/animation/runtime/ik_two_bone_job.h"
#include "ozz/base/maths/simd_quaternion.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
using namespace novacore;

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

math::Quat globalRotation(const animation::Skeleton& skeleton, const animation::LocalPose& local, int joint) {
    math::Quat result;
    while (joint >= 0) {
        result = animation::multiply(local.joints[joint].rotation, result);
        joint = skeleton.joints[joint].parent;
    }
    return result;
}

bool solveArm(const assets::GltfAnimationAsset& asset, const animation::GlobalPose& baseGlobal,
              animation::LocalPose& local, const char* side, float inwardFraction) {
    const auto& skeleton = asset.skeleton;
    const auto start = animation::findJoint(skeleton, std::string("upper_arm.") + side);
    const auto middle = animation::findJoint(skeleton, std::string("lower_arm.") + side);
    const auto end = animation::findJoint(skeleton, std::string("hand.") + side);
    const auto socketIndex = animation::findSocket(skeleton, side[0] == 'R' ? "socket_hand_r" : "socket_hand_l");
    if (!start || !middle || !end || !socketIndex) {
        std::cerr << "Missing required authored arm chain/socket: " << side << '\n';
        return false;
    }
    const auto& socket = skeleton.sockets[*socketIndex];
    if (socket.joint != *end || skeleton.joints[*end].parent != static_cast<int>(*middle) ||
        skeleton.joints[*middle].parent != static_cast<int>(*start)) {
        std::cerr << "Probe expects the verified direct upper/forearm/hand chain\n";
        return false;
    }
    const auto a = animation::matrixTranslation(baseGlobal.joints[*start]);
    const auto b = animation::matrixTranslation(baseGlobal.joints[*middle]);
    const auto c = animation::matrixTranslation(baseGlobal.joints[*end]);
    const auto upper = b - a;
    const auto lower = c - b;
    const auto bend = math::Vec3{upper.y * lower.z - upper.z * lower.y,
                                upper.z * lower.x - upper.x * lower.z,
                                upper.x * lower.y - upper.y * lower.x};
    if (bend.lengthSquared() < 0.0000001F || upper.lengthSquared() < 0.0000001F) {
        std::cerr << "Degenerate authored bend plane; no arbitrary pole invented\n";
        return false;
    }

    const auto handRotation = globalRotation(skeleton, local, static_cast<int>(*end));
    const auto originalOffset = animation::transformDirection(baseGlobal.joints[*end], socket.local.translation);
    // Small reachable targets in model space, deliberately inside each chain's
    // reach. This is adoption evidence, not the game's final M4 grip placement.
    const auto gripGoal = c + originalOffset + (a - c) * inwardFraction + math::Vec3{0.003F, 0.003F, 0};
    const auto wristGoal = gripGoal - originalOffset;
    const auto axis = animation::rotate(
        animation::conjugate(globalRotation(skeleton, local, static_cast<int>(*middle))),
        bend * (-1.0F / std::sqrt(bend.lengthSquared())));
    const auto pole = upper * (1.0F / std::sqrt(upper.lengthSquared()));
    const auto m0 = toOzz(baseGlobal.joints[*start]);
    const auto m1 = toOzz(baseGlobal.joints[*middle]);
    const auto m2 = toOzz(baseGlobal.joints[*end]);
    ozz::animation::IKTwoBoneJob job;
    job.start_joint = &m0;
    job.mid_joint = &m1;
    job.end_joint = &m2;
    job.target = ozz::math::simd_float4::Load(wristGoal.x, wristGoal.y, wristGoal.z, 0);
    job.mid_axis = ozz::math::simd_float4::Load(axis.x, axis.y, axis.z, 0);
    job.pole_vector = ozz::math::simd_float4::Load(pole.x, pole.y, pole.z, 0);
    ozz::math::SimdQuaternion q0, q1;
    bool reached = false;
    job.start_joint_correction = &q0;
    job.mid_joint_correction = &q1;
    job.reached = &reached;
    if (!job.Run() || !reached) {
        std::cerr << "ozz rejected or could not reach the probe target\n";
        return false;
    }
    local.joints[*start].rotation = animation::multiply(local.joints[*start].rotation, fromOzz(q0));
    local.joints[*middle].rotation = animation::multiply(local.joints[*middle].rotation, fromOzz(q1));
    // Preserve desired hand orientation, then its authored socket offset, after
    // solving the wrist position. A hand joint is not the authored hand socket.
    const auto parentRotation = globalRotation(skeleton, local, skeleton.joints[*end].parent);
    local.joints[*end].rotation = animation::multiply(animation::conjugate(parentRotation), handRotation);
    animation::GlobalPose solved;
    animation::Mat4 actual, expected;
    if (!animation::evaluateGlobalPose(skeleton, local, solved) ||
        !animation::evaluateSocket(skeleton, solved, *socketIndex, actual) ||
        !animation::evaluateSocket(skeleton, baseGlobal, *socketIndex, expected)) {
        std::cerr << "Corrected pose/socket evaluation failed\n";
        return false;
    }
    const auto residual = std::sqrt((animation::matrixTranslation(actual) - gripGoal).lengthSquared());
    const auto firstLength = std::sqrt((animation::matrixTranslation(solved.joints[*middle]) -
                                      animation::matrixTranslation(solved.joints[*start])).lengthSquared());
    const auto secondLength = std::sqrt((animation::matrixTranslation(solved.joints[*end]) -
                                       animation::matrixTranslation(solved.joints[*middle])).lengthSquared());
    bool valid = std::isfinite(residual) && residual < 0.001F &&
        std::abs(firstLength - std::sqrt(upper.lengthSquared())) < 0.0001F &&
        std::abs(secondLength - std::sqrt(lower.lengthSquared())) < 0.0001F;
    for (const auto axisVector : {math::Vec3{0, 0, 1}, math::Vec3{0, 1, 0}}) {
        valid = valid && (animation::transformDirection(actual, axisVector) -
                          animation::transformDirection(expected, axisVector)).lengthSquared() < 0.000001F;
    }
    std::cout << "arm=" << side << " inward=" << inwardFraction << " residual_m=" << residual << '\n';
    return valid;
}
} // namespace

int main() {
    using namespace novacore;
    assets::GltfMeshData mesh;
    const auto path = std::filesystem::path(NEMISIS_SOURCE_ROOT) / "assets/export/gltf/characters/chr_a1_fp_arms_01.glb";
    const auto loaded = assets::loadGltfMeshData(path, mesh);
    assets::GltfAnimationAsset asset;
    if (!loaded.ok() || !mesh.animationData || !assets::buildGltfAnimationAsset(*mesh.animationData, 0U, asset).ok()) {
        std::cerr << "Cannot import the actual FP arms skin/animation\n";
        return 1;
    }
    // Bind pose and every available authored pose; NovaCore owns all sampling.
    for (std::size_t poseIndex = 0; poseIndex <= asset.clips.size(); ++poseIndex) {
        auto base = animation::makeBindPose(asset.skeleton);
        if (poseIndex > 0 && !animation::sampleClip(asset.clips[poseIndex - 1], asset.skeleton,
                asset.clips[poseIndex - 1].duration * 0.35F, animation::WrapMode::Clamp, base)) {
            std::cerr << "Authored clip sampling failed\n";
            return 1;
        }
        animation::GlobalPose global;
        if (!animation::evaluateGlobalPose(asset.skeleton, base, global)) {
            return 1;
        }
        assets::GltfMeshData baseMesh, correctedMesh;
        if (!assets::skinGltfMesh(mesh, asset, global, baseMesh).ok()) {
            std::cerr << "Base-pose skinning failed\n";
            return 1;
        }
        for (const float fraction : {0.04F, 0.08F, 0.12F}) {
            auto corrected = base;
            if (!solveArm(asset, global, corrected, "R", fraction) || !solveArm(asset, global, corrected, "L", fraction)) {
                std::cerr << "Arm IK contract failed in pose " << poseIndex << '\n';
                return 1;
            }
            animation::GlobalPose correctedGlobal;
            if (!animation::evaluateGlobalPose(asset.skeleton, corrected, correctedGlobal) ||
                !assets::skinGltfMesh(mesh, asset, correctedGlobal, correctedMesh).ok()) {
                std::cerr << "Constrained-pose skinning failed\n";
                return 1;
            }
            bool deformed = false;
            for (std::size_t p = 0; p < baseMesh.primitives.size(); ++p) {
                if (baseMesh.primitives[p].skinIndex < 0) {
                    continue;
                }
                for (std::size_t v = 0; v < baseMesh.primitives[p].positions.size(); ++v) {
                    deformed = deformed || (baseMesh.primitives[p].positions[v] -
                        correctedMesh.primitives[p].positions[v]).lengthSquared() > 0.00000001F;
                }
            }
            if (!deformed) {
                std::cerr << "IK changed no weighted arm/hand geometry\n";
                return 1;
            }
        }
    }
    std::cout << "ozz arm IK probe passed; productive game rig is unchanged\n";
    return 0;
}

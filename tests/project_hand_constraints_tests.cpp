#include "nemisis/player/PlayerHandConstraints.hpp"
#include "novacore/assets/GltfAnimationBridge.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace novacore;
using namespace nemisis::player;
void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}
bool equal(math::Vec3 a, math::Vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
bool equal(math::Quat a, math::Quat b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
float dot(math::Vec3 a, math::Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
math::Vec3 unit(math::Vec3 v) { return v * (1.0F / std::sqrt(v.lengthSquared())); }
math::Vec3 cross(math::Vec3 a, math::Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Deliberately author a rigid target using an orthogonal basis. This defines a
// target; it does not repeat the adapter's polar decomposition implementation.
animation::Mat4 rigidTarget(const animation::Mat4& palm, math::Vec3 offset) {
    const auto x = unit(animation::transformDirection(palm, {1, 0, 0}));
    const auto initialY = animation::transformDirection(palm, {0, 1, 0});
    const auto y = unit(initialY - x * dot(x, initialY));
    const auto z = cross(x, y);
    animation::Mat4 target;
    const std::array axes{x, y, z};
    for (std::size_t c = 0; c < 3; ++c) {
        target.at(0, c) = axes[c].x;
        target.at(1, c) = axes[c].y;
        target.at(2, c) = axes[c].z;
    }
    const auto p = animation::matrixTranslation(palm) + offset;
    target.at(0, 3) = p.x;
    target.at(1, 3) = p.y;
    target.at(2, 3) = p.z;
    return target;
}

void preserved(const animation::LocalPose& before, const animation::LocalPose& after,
               const PlayerHandConstraintRig& rig, bool allRotations = false) {
    require(before.joints.size() == after.joints.size(), "all weighted/helper joints retained");
    for (std::size_t j = 0; j < before.joints.size(); ++j) {
        require(equal(before.joints[j].translation, after.joints[j].translation) &&
                    equal(before.joints[j].scale, after.joints[j].scale),
                "all authored translations/scales retained exactly");
        bool selected = false;
        for (const auto& arm : rig.arms) {
            selected = selected || j == arm.shoulder || j == arm.elbow || j == arm.wrist;
        }
        if (!selected || allRotations) {
            require(equal(before.joints[j].rotation, after.joints[j].rotation),
                    "intermediate/finger/non-selected rotations retained exactly");
        }
    }
}

// For a positive affine A, polar(A)=R iff R^T A is symmetric positive definite.
// This independently checks rigid palm orientation without copying the solver.
void orientation(const animation::Mat4& actual, const animation::Mat4& desired) {
    float metric[3][3]{};
    for (std::size_t r = 0; r < 3; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            for (std::size_t k = 0; k < 3; ++k) {
                metric[r][c] += desired.at(k, r) * actual.at(k, c);
            }
        }
    }
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            require(std::abs(metric[r][c] - metric[c][r]) < 0.0001F,
                    "actual palm rigid orientation matches the authored target");
        }
    }
    require(metric[0][0] > 0 && metric[0][0] * metric[1][1] - metric[0][1] * metric[1][0] > 0,
            "palm orientation does not reflect or collapse the real affine frame");
}
} // namespace

int main() {
    try {
        assets::GltfMeshData mesh;
        require(assets::loadGltfMeshData(
                    std::filesystem::path(NEMISIS_PROJECT_FPS_CANDIDATE_ROOT) / "arms_palm_proposal.glb", mesh)
                    .ok(),
                "real Project palm candidate imports");
        assets::GltfAnimationAsset asset;
        require(assets::buildGltfAnimationAsset(*mesh.animationData, 0, asset).ok(),
                "real Project animation bridge succeeds");
        PlayerHandConstraintRig rig;
        require(buildPlayerHandConstraintRig(asset.skeleton, rig), "actual weighted five-segment chains calibrate");
        auto broken = asset.skeleton;
        const auto intermediate = animation::findJoint(broken, "DEF-forearm.R.001");
        require(intermediate.has_value(), "weighted intermediate is present");
        broken.joints[*intermediate].parent = static_cast<int>(rig.arms[0].shoulder);
        PlayerHandConstraintRig invalidRig;
        require(!buildPlayerHandConstraintRig(broken, invalidRig) && !invalidRig.valid,
                "unexpected intermediate ancestry rejects instead of flattening the chain");
        std::size_t cases = 0;
        float largestResidual = 0;
        for (int clip = -1; clip < static_cast<int>(asset.clips.size()); ++clip) {
            if (clip >= 0 && asset.clips[clip].duration <= 0) {
                continue;
            }
            for (float phase : {0.13F, 0.73F}) {
                animation::LocalPose base;
                if (clip < 0) {
                    base = animation::makeBindPose(asset.skeleton);
                } else {
                    require(animation::sampleClip(asset.clips[clip], asset.skeleton, asset.clips[clip].duration * phase,
                                                  animation::WrapMode::Clamp, base),
                            "authored clip samples without dropping scale channels");
                }
                animation::GlobalPose global;
                require(animation::evaluateGlobalPose(asset.skeleton, base, global),
                        "complete sampled hierarchy evaluates");
                for (const auto model :
                     {animation::Transform{},
                      animation::Transform{
                          {3, -0.2F, 5}, animation::normalize({0.07F, 0.31F, -0.03F, 0.93F}), {1, 1, 1}},
                      animation::Transform{
                          {-2, 0.4F, 1}, animation::normalize({-0.04F, 0.2F, 0.08F, 0.97F}), {1.1F, 0.9F, 1.05F}}}) {
                    PlayerHandTargets targets;
                    targets.armsWorld = animation::matrixFromTransform(model);
                    std::array<animation::Mat4, 2> modelTargets;
                    for (std::size_t side = 0; side < 2; ++side) {
                        animation::Mat4 palm;
                        require(animation::evaluateSocket(asset.skeleton, global, rig.arms[side].socketIndex, palm),
                                "actual palm socket evaluates");
                        const auto shoulder = animation::matrixTranslation(global.joints[rig.arms[side].shoulder]);
                        const auto inward = unit(shoulder - animation::matrixTranslation(palm)) * 0.02F;
                        modelTargets[side] = rigidTarget(palm, inward);
                    }
                    targets.rightGripWorld = rigidTarget(animation::multiply(targets.armsWorld, modelTargets[0]), {});
                    targets.leftGripWorld = rigidTarget(animation::multiply(targets.armsWorld, modelTargets[1]), {});
                    animation::LocalPose solved;
                    animation::GlobalPose final;
                    const auto result =
                        applyPlayerHandConstraints(rig, asset.skeleton, base, global, targets, solved, final);
                    require(result.status == HandConstraintStatus::Applied && result.reached[0] && result.reached[1],
                            "both real Project palms reach the authored rigid targets");
                    preserved(base, solved, rig);
                    for (std::size_t side = 0; side < 2; ++side) {
                        animation::Mat4 palm;
                        require(animation::evaluateSocket(asset.skeleton, final, rig.arms[side].socketIndex, palm),
                                "constrained actual palm evaluates");
                        const auto actual =
                            animation::transformPoint(targets.armsWorld, animation::matrixTranslation(palm));
                        const auto expected =
                            animation::matrixTranslation(side == 0 ? targets.rightGripWorld : targets.leftGripWorld);
                        const auto residual = std::sqrt((actual - expected).lengthSquared());
                        require(residual <= 0.001F && std::abs(residual - result.residualMeters[side]) < 0.000001F,
                                "independent world-space actual socket residual stays "
                                "within one millimetre");
                        largestResidual = std::max(largestResidual, residual);
                        orientation(animation::multiply(targets.armsWorld, palm),
                                    side == 0 ? targets.rightGripWorld : targets.leftGripWorld);
                        require(result.orientationResidualDegrees[side] <= 0.5F && result.iterations[side] >= 1 &&
                                    result.iterations[side] <= 4,
                                "actual orientation and bounded ozz iteration budget reported");
                    }
                    assets::GltfMeshData before, after;
                    require(assets::skinGltfMesh(mesh, asset, global, before).ok() &&
                                assets::skinGltfMesh(mesh, asset, final, after).ok(),
                            "final full-affine pose skins actual Project geometry");
                    float drift = 0;
                    for (std::size_t p = 0; p < before.primitives.size(); ++p) {
                        require(before.primitives[p].joints == after.primitives[p].joints &&
                                    before.primitives[p].weights == after.primitives[p].weights,
                                "skin palette/weights remain unchanged");
                        for (std::size_t v = 0; v < before.primitives[p].positions.size(); ++v) {
                            drift = std::max(
                                drift,
                                (before.primitives[p].positions[v] - after.primitives[p].positions[v]).lengthSquared());
                        }
                    }
                    require(drift > 0.00000001F, "actual weighted vertices deform, not only debug sockets");
                    auto invalidTargets = targets;
                    invalidTargets.rightGripWorld.at(0, 0) = std::numeric_limits<float>::quiet_NaN();
                    auto aliasLocal = base;
                    auto aliasGlobal = global;
                    require(applyPlayerHandConstraints(rig, asset.skeleton, aliasLocal, aliasGlobal, invalidTargets,
                                                       aliasLocal, aliasGlobal)
                                    .status == HandConstraintStatus::Invalid,
                            "invalid affine data rejects transactionally even with "
                            "aliased outputs");
                    preserved(base, aliasLocal, rig, true);
                    auto unreachable = targets;
                    unreachable.rightGripWorld.at(0, 3) += 20;
                    const auto failed =
                        applyPlayerHandConstraints(rig, asset.skeleton, base, global, unreachable, solved, final);
                    require(failed.status == HandConstraintStatus::Unreachable && !failed.reached[0] &&
                                failed.residualMeters[0] > 10,
                            "real unreachable goal reports its actual large residual");
                    preserved(base, solved, rig);
                    ++cases;
                }
            }
        }
        std::cout << "Project affine ozz cases=" << cases << " max_world_residual_m=" << largestResidual
                  << "; physical weapon contact/artist acceptance remains separate\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

#include "nemisis/player/PlayerSkeletalAnimator.hpp"

#include "novacore/assets/GltfDocument.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

[[nodiscard]] bool positionsDiffer(
    const novacore::assets::GltfMeshData& lhs,
    const novacore::assets::GltfMeshData& rhs) {
    if (lhs.primitives.size() != rhs.primitives.size()) {
        return true;
    }
    for (std::size_t primitiveIndex = 0; primitiveIndex < lhs.primitives.size(); ++primitiveIndex) {
        const auto& a = lhs.primitives[primitiveIndex].positions;
        const auto& b = rhs.primitives[primitiveIndex].positions;
        if (a.size() != b.size()) {
            return true;
        }
        for (std::size_t vertexIndex = 0; vertexIndex < a.size(); vertexIndex += 97U) {
            const auto delta = a[vertexIndex] - b[vertexIndex];
            if (delta.lengthSquared() > 0.0000001F) {
                return true;
            }
        }
    }
    return false;
}

void testCookedCharacterImportsAndAnimates() {
    const auto path = std::filesystem::path(NEMISIS_SOURCE_ROOT) /
        "assets/export/gltf/characters/chr_a1_stylized_operator_01.glb";
    novacore::assets::GltfMeshData mesh{};
    const auto load = novacore::assets::loadGltfMeshData(path, mesh);
    expect(load.ok(), "cooked A1 character mesh imports");
    if (!load.ok()) {
        return;
    }

    nemisis::player::PlayerSkeletalAnimator animator;
    expect(animator.initialize(mesh), "cooked character skeletal animator initializes");
    const auto initialStats = animator.stats();
    expect(initialStats.jointCount > 10U, "cooked character exposes a real joint hierarchy");
    expect(initialStats.clipCount >= 2U, "cooked character exposes authored looping animation clips");
    expect(initialStats.skinnedVertexCount > 400U, "cooked character has weighted skinned vertices");

    const auto* initialMesh = animator.skinnedMesh();
    expect(initialMesh != nullptr, "initial bind/idle pose produces a skinned mesh");
    if (initialMesh == nullptr) {
        return;
    }
    const auto idleMesh = *initialMesh;

    nemisis::player::CharacterAnimationFrame walk{};
    walk.locomotionClip = nemisis::player::CharacterAnimationClip::Walk;
    walk.stride01 = 0.65F;
    for (int frame = 0; frame < 8; ++frame) {
        expect(animator.update(walk, 1.0F / 60.0F), "walk pose evaluates and skins");
    }
    expect(
        animator.stats().currentClip.find("walk") != std::string::npos ||
            animator.stats().currentClip.find("Walk") != std::string::npos,
        "walk state selects authored Walk clip");
    expect(positionsDiffer(idleMesh, *animator.skinnedMesh()), "walk clip deforms character vertices away from idle pose");

    expect(animator.stats().evaluatedFrameCount >= 8U, "skeletal runtime tracks evaluated frames");

    novacore::animation::Mat4 hand{};
    expect(animator.socketTransform("socket_hand_r", hand), "right-hand runtime socket follows the animated skeleton");
}

} // namespace

int main() {
    testCookedCharacterImportsAndAnimates();
    if (failures != 0) {
        std::cerr << failures << " player skeletal animator test(s) failed\n";
        return 1;
    }
    std::cout << "player skeletal animator tests passed\n";
    return 0;
}

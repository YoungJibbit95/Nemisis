#include "nemisis/dev/DevRangeRenderScene.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"

#include "novacore/assets/GltfDocument.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

novacore::assets::GltfMeshData makeMesh() {
    novacore::assets::GltfPrimitiveData primitive{};
    primitive.positions = {
        {-0.5F, 0.0F, 0.0F},
        {0.5F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F},
    };
    primitive.normals = {
        {0.0F, 1.0F, 0.0F},
        {0.0F, 1.0F, 0.0F},
        {0.0F, 1.0F, 0.0F},
    };
    primitive.texcoords = {
        {0.0F, 0.0F},
        {1.0F, 0.0F},
        {0.5F, 1.0F},
    };
    primitive.indices = {0, 1, 2};

    novacore::assets::GltfMeshData mesh{};
    mesh.primitives.push_back(std::move(primitive));
    return mesh;
}

nemisis::dev::MeshResourceLookup registerSceneMeshes(novacore::render::Renderer& renderer) {
    static constexpr std::array<std::string_view, 13> kSceneMeshes{
        "env_test_arena_kit_01",
        "prop_target_dummy_01",
        "chr_dev_soldier_a",
        "chr_proto_humanoid_01",
        "map_floor_tile_01",
        "map_wall_panel_01",
        "map_cover_crate_01",
        "map_ramp_01",
        "map_target_stand_01",
        "wpn_ar_01",
        "wpn_proto_smg_01",
        "chr_dev_arms_a",
        "unused_spare_scene_probe",
    };

    nemisis::dev::MeshResourceLookup lookup;
    const auto mesh = makeMesh();
    for (const auto assetId : kSceneMeshes) {
        lookup.emplace(std::string(assetId), renderer.registerMeshResource(std::string(assetId), mesh));
    }
    return lookup;
}

void testDevRangeRenderSceneBuildsExpectedSubmissions() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::DebugTargetState target{};
    target.eliminated = false;

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = {1.0F, 0.0F, 2.0F};
    player.view.yawDegrees = 32.0F;
    player.view.pitchDegrees = -7.0F;
    player.hasMovementState = true;

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &target,
            &lookup,
            player,
        });

    expect(frame.camera3D.enabled, "dev range render scene enables 3D camera");
    expect(frame.camera3D.position.x == 1.0F, "dev range render scene copies player x");
    expect(frame.camera3D.position.y > 1.6F && frame.camera3D.position.y < 1.7F, "dev range camera uses eye height");
    expect(frame.camera3D.position.z == 2.0F, "dev range render scene copies player z");
    expect(frame.camera3D.yawDegrees == 32.0F, "dev range render scene copies yaw");
    expect(frame.camera3D.pitchDegrees == -7.0F, "dev range render scene copies pitch");
    expect(frame.camera3D.verticalFovDegrees == 74.0F, "dev range render scene uses default FOV");

    expect(!world.primitives.empty(), "greybox world fixture has primitives");
    expect(stats.worldBoxCount == world.primitives.size() + 6U, "dev range render scene emits world, weapon, and aim boxes");
    expect(frame.worldBoxes.size() == stats.worldBoxCount, "world box count matches frame");
    expect(stats.meshInstanceCount == 13, "dev range render scene emits all mesh instances");
    expect(frame.worldMeshes.size() == 13, "frame receives all mesh instances");
    expect(stats.skippedMeshInstanceCount == 0, "dev range render scene skips no mesh when lookup is complete");
    expect(stats.firstPersonMeshCount == 3, "dev range render scene emits three first-person mesh anchors");
    expect(stats.aimMarkerBoxCount == 5, "dev range render scene emits five aim marker boxes");

    const auto firstMesh = frame.worldMeshes.front();
    expect(firstMesh.assetId == "env_test_arena_kit_01", "first dev mesh is the arena kit");
    expect(firstMesh.mesh.isValid(), "first dev mesh has a valid renderer resource handle");
}

void testDevRangeRenderSceneCountsMissingMeshHandles() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    lookup.erase("wpn_proto_smg_01");
    lookup.erase("chr_dev_arms_a");

    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::DebugTargetState target{};
    target.eliminated = true;

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 180.0F;

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &target,
            &lookup,
            player,
        });

    expect(stats.meshInstanceCount == 11, "dev range render scene still emits available meshes");
    expect(frame.worldMeshes.size() == 11, "frame mesh count drops missing handles");
    expect(stats.skippedMeshInstanceCount == 2, "dev range render scene counts missing handles");
    expect(stats.firstPersonMeshCount == 1, "first-person mesh stats reflect missing weapon/arms handles");
}

void testDevRangeRenderSceneHandlesMissingInputs() {
    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(frame, {});

    expect(!frame.camera3D.enabled, "dev range render scene leaves camera disabled without world/input");
    expect(frame.worldBoxes.empty(), "dev range render scene emits no boxes without world/input");
    expect(frame.worldMeshes.empty(), "dev range render scene emits no meshes without world/input");
    expect(stats.worldBoxCount == 0, "dev range render scene reports zero boxes without world/input");
    expect(stats.meshInstanceCount == 0, "dev range render scene reports zero meshes without world/input");
}

} // namespace

int main() {
    testDevRangeRenderSceneBuildsExpectedSubmissions();
    testDevRangeRenderSceneCountsMissingMeshHandles();
    testDevRangeRenderSceneHandlesMissingInputs();

    if (failures > 0) {
        std::cerr << failures << " dev range render scene test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Dev range render scene tests passed\n";
    return EXIT_SUCCESS;
}

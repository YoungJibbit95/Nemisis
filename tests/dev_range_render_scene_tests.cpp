#include "nemisis/dev/DevRangeRenderScene.hpp"
#include "nemisis/dev/DevRangeSession.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"
#include "nemisis/player/PlayerFirstPersonRig.hpp"
#if defined(NEMISIS_PROJECT_FPS_CANDIDATE_ROOT)
#include "nemisis/player/PlayerSkeletalAnimator.hpp"
#endif

#include "novacore/assets/GltfDocument.hpp"
#include "novacore/animation/AnimationTypes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
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
    static constexpr std::array<std::string_view, 36> kSceneMeshes{
        nemisis::dev::kLocalPlayerBodyResourceId,
        "env_test_arena_kit_01",
        "env_project_skybox1",
        "prop_target_dummy_01",
        "chr_dev_soldier_a",
        "chr_project_male1",
        "chr_proto_humanoid_01",
        "chr_a1_stylized_operator_01",
        "map_floor_tile_01",
        "map_wall_panel_01",
        "map_cover_crate_01",
        "map_ramp_01",
        "map_target_stand_01",
        "wpn_ar_01",
        "wpn_smg_01",
        "wpn_sidearm_01",
        "wpn_proto_smg_01",
        "wpn_a1_compact_rifle_01",
        "wpn_a1_modern_rifle_01",
        "wpn_a1_compact_sidearm_01",
        "chr_dev_arms_a",
        "chr_a1_fp_arms_01",
        "wpn_a2_blackout_carbine_01",
        "wpn_a2_modular_rifle_01",
        "wpn_a2_striker_sidearm_01",
        "chr_a2_pilot_operator_01",
        "map_a2_wallrun_panel_01",
        "map_a2_slide_ramp_01",
        "map_a2_cover_crate_01",
        "prop_a2_range_hero_01",
        "wpn_project_rifle_m4a1",
        "wpn_project_rifle_afr120",
        "wpn_project_rifle_ncar",
        "wpn_project_smg_fr17",
        "wpn_project_sidearm_glock19",
        "wpn_project_sidearm_p320",
    };

    nemisis::dev::MeshResourceLookup lookup;
    const auto mesh = makeMesh();
    for (const auto assetId : kSceneMeshes) {
        lookup.emplace(std::string(assetId), renderer.registerMeshResource(std::string(assetId), mesh));
    }
    return lookup;
}

novacore::render::MeshCatalog makeSocketCatalogForFirstPersonWeapon(
    bool rotatedGrips = false,
    bool invalidGrips = false) {
    novacore::assets::AssetRecord record{};
    record.id = "wpn_project_rifle_m4a1";
    record.kind = novacore::assets::AssetKind::Mesh;
    record.cookedPath = "assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb";

    novacore::assets::GltfAssetMetadata metadata{};
    metadata.id = record.id;
    metadata.exportPath = record.cookedPath;
    metadata.scaleMeters = true;
    metadata.runtimeUpAxis = "Y";
    metadata.gameplayForwardAxis = "+Z";
    metadata.sockets = {"socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_eject"};
    metadata.license = "original_project_asset";

    auto meshData = makeMesh();
    meshData.nodeMarkers = {
        novacore::assets::GltfNodeMarker{"socket_muzzle", {0.0F, 0.15F, 0.44F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, -1},
        novacore::assets::GltfNodeMarker{"socket_grip_r", {0.03F, 0.08F, -0.09F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, -1},
        novacore::assets::GltfNodeMarker{"socket_grip_l", {-0.02F, 0.10F, 0.16F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, -1},
        novacore::assets::GltfNodeMarker{"socket_eject", {0.04F, 0.14F, 0.08F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, -1},
     };
    if (rotatedGrips) {
        meshData.nodeMarkers[1].worldForward = {1.0F, 0.0F, 0.0F};
        meshData.nodeMarkers[2].worldForward = {0.0F, 1.0F, 0.0F};
        meshData.nodeMarkers[2].worldUp = {0.0F, 0.0F, -1.0F};
    }
    if (invalidGrips) {
        meshData.nodeMarkers[1].worldForward = {};
        meshData.nodeMarkers[2].worldForward = meshData.nodeMarkers[2].worldUp;
    }

    novacore::render::MeshCatalog catalog;
    (void)catalog.registerImportedGltfAsset(record, metadata, std::move(meshData));

    novacore::assets::AssetRecord armsRecord{};
    armsRecord.id = "chr_a1_fp_arms_01";
    armsRecord.kind = novacore::assets::AssetKind::Mesh;
    armsRecord.cookedPath = "assets/export/gltf/characters/chr_a1_fp_arms_01.glb";

    novacore::assets::GltfAssetMetadata armsMetadata{};
    armsMetadata.id = armsRecord.id;
    armsMetadata.exportPath = armsRecord.cookedPath;
    armsMetadata.scaleMeters = true;
    armsMetadata.runtimeUpAxis = "Y";
    armsMetadata.gameplayForwardAxis = "+Z";
    armsMetadata.sockets = {"socket_camera", "socket_weapon_root", "socket_hand_r", "socket_hand_l"};
    armsMetadata.license = "original_project_asset";

    auto armsMeshData = makeMesh();
    armsMeshData.nodeMarkers = {
        novacore::assets::GltfNodeMarker{"socket_camera", {0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, -1},
        novacore::assets::GltfNodeMarker{"socket_weapon_root", {0.0F, -0.39F, -0.645F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, -1},
        novacore::assets::GltfNodeMarker{"socket_hand_r", {0.122F, -0.42F, -0.81F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, -1},
        novacore::assets::GltfNodeMarker{"socket_hand_l", {-0.122F, -0.424F, -0.69F}, {0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, -1},
    };
    (void)catalog.registerImportedGltfAsset(armsRecord, armsMetadata, std::move(armsMeshData));
    return catalog;
}

std::optional<novacore::render::RenderMesh3D> findMesh(
    const novacore::render::RenderFrameInfo& frame,
    std::string_view assetId) {
    for (const auto& mesh : frame.worldMeshes) {
        if (mesh.assetId == assetId) {
            return mesh;
        }
    }
    return std::nullopt;
}

std::optional<novacore::render::RenderMesh3D> findLastMesh(
    const novacore::render::RenderFrameInfo& frame,
    std::string_view assetId) {
    for (auto it = frame.worldMeshes.rbegin(); it != frame.worldMeshes.rend(); ++it) {
        if (it->assetId == assetId) {
            return *it;
        }
    }
    return std::nullopt;
}

void useFirstPersonCamera(nemisis::dev::DevRangePlayerRenderState& player) {
    player.hasCameraRig = true;
    player.cameraPosition = player.position + novacore::math::Vec3{0.0F, 1.65F, 0.0F};
    player.cameraView = player.view;
}

novacore::animation::Mat4 renderedMeshMatrix(const novacore::render::RenderMesh3D& mesh) {
    // Independent oracle for the renderer's published T * Ry * Rx * Rz * S
    // convention. Do not use the presentation conversion under test here.
    constexpr float halfRadians = 3.14159265358979323846F / 360.0F;
    const float yaw = mesh.yawDegrees * halfRadians;
    const float pitch = mesh.pitchDegrees * halfRadians;
    const float roll = mesh.rollDegrees * halfRadians;
    const auto rotation = novacore::animation::multiply(
        novacore::math::Quat{0.0F, std::sin(yaw), 0.0F, std::cos(yaw)},
        novacore::animation::multiply(
            novacore::math::Quat{std::sin(pitch), 0.0F, 0.0F, std::cos(pitch)},
            novacore::math::Quat{0.0F, 0.0F, std::sin(roll), std::cos(roll)}));
    return novacore::animation::matrixFromTransform({mesh.position, rotation, mesh.scale});
}

bool matricesNear(const novacore::animation::Mat4& lhs, const novacore::animation::Mat4& rhs) {
    for (std::size_t index = 0; index < lhs.values.size(); ++index) {
        if (!std::isfinite(lhs.values[index]) || !std::isfinite(rhs.values[index]) ||
            std::abs(lhs.values[index] - rhs.values[index]) > 0.000003F) {
            return false;
        }
    }
    return true;
}

void expectMarkerFrame(
    const novacore::animation::Mat4& markerWorld,
    const novacore::animation::Mat4& renderedWorld,
    const novacore::assets::GltfNodeMarker& marker) {
    const auto near = [](novacore::math::Vec3 lhs, novacore::math::Vec3 rhs) {
        return (lhs - rhs).lengthSquared() < 0.000000001F;
    };
    const auto normalized = [](novacore::math::Vec3 vector) {
        return vector * (1.0F / std::sqrt(vector.lengthSquared()));
    };
    expect(near(novacore::animation::matrixTranslation(markerWorld),
                novacore::animation::transformPoint(renderedWorld, marker.worldPosition)),
        "shared socket position comes from the exact submitted weapon transform");
    expect(near(normalized(novacore::animation::transformDirection(markerWorld, {0.0F, 0.0F, 1.0F})),
                normalized(novacore::animation::transformDirection(renderedWorld, marker.worldForward))),
        "shared socket forward retains the authored marker orientation");
    expect(near(normalized(novacore::animation::transformDirection(markerWorld, {0.0F, 1.0F, 0.0F})),
                normalized(novacore::animation::transformDirection(renderedWorld, marker.worldUp))),
        "shared socket up retains the authored marker orientation");
}

void testDevRangeRenderSceneBuildsExpectedSubmissions() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = {1.0F, 0.0F, 2.0F};
    player.view.yawDegrees = 32.0F;
    player.view.pitchDegrees = -7.0F;
    player.cameraRollDegrees = 12.0F;
    player.hasMovementState = true;
    useFirstPersonCamera(player);
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.grounded = true;
    collision.groundPrimitiveId = "floor_main";
    collision.groundNormal = {0.0F, 1.0F, 0.0F};
    collision.contacts.push_back(nemisis::dev::GreyboxContact{
        "floor_main",
        nemisis::dev::GreyboxPrimitiveKind::Floor,
        nemisis::dev::GreyboxContactRole::Ground,
        {1.0F, 0.0F, 2.0F},
        {0.0F, 1.0F, 0.0F},
        0.0F,
        1.0F,
        0.0F,
        false,
        true,
    });

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            &collision,
            &lookup,
            player,
        });

    expect(frame.camera3D.enabled, "dev range render scene enables 3D camera");
    expect(frame.camera3D.position.x == 1.0F, "dev range render scene copies player x");
    expect(frame.camera3D.position.y > 1.6F && frame.camera3D.position.y < 1.7F, "dev range camera uses eye height");
    expect(frame.camera3D.position.z == 2.0F, "dev range render scene copies player z");
    expect(frame.camera3D.yawDegrees == 32.0F, "dev range render scene copies yaw");
    expect(frame.camera3D.pitchDegrees == -7.0F, "dev range render scene copies pitch");
    expect(frame.camera3D.rollDegrees == player.cameraRollDegrees,
           "world and sky receive the same presented camera roll");
    expect(frame.camera3D.verticalFovDegrees == 74.0F, "dev range render scene uses default FOV");
    expect(frame.lighting.ambientIntensity > 0.33F && frame.lighting.ambientIntensity < 0.35F, "dev range render scene applies lighting profile");
    expect(frame.lighting.sunDirection.y > 0.30F && frame.lighting.sunDirection.y < 0.40F,
           "Dev Range uses a visible low-elevation sun above the horizon");
    expect(frame.lighting.fillIntensity > 0.20F, "dev range lighting applies fill light for imported meshes");
    expect(frame.lighting.rimIntensity > 0.20F, "dev range lighting applies rim light for mesh readability");
    expect(frame.sky.enabled, "dev range render scene enables renderer sky pass");
    expect(stats.skyPassEnabled, "dev range render scene reports active sky pass");

    expect(!world.primitives.empty(), "greybox world fixture has primitives");
    expect(stats.worldBoxCount == (world.primitives.size() - 1U) + targetRange.lanes.size() + 13U, "dev range render scene emits world, lane, asset stage, and pickup pads without duplicate arm proxies");
    expect(frame.worldBoxes.size() == stats.worldBoxCount, "world box count matches frame");
    expect(stats.meshInstanceCount == 34, "first-person scene emits world, target, showcase, weapon, and arms meshes without a duplicate local world model");
    expect(frame.worldMeshes.size() == 34, "frame receives all world and exclusive viewmodel mesh instances");
    expect(stats.skyFallbackMeshCount == 0, "dev range render scene skips legacy skybox mesh while sky pass is active");
    expect(stats.skippedMeshInstanceCount == 0, "dev range render scene skips no mesh when lookup is complete");
    expect(stats.materialFallbackProfileCount == stats.meshInstanceCount, "dev range render scene assigns material fallback profiles to submitted GLB meshes");
    expect(stats.contactShadowCount >= targetRange.lanes.size(), "dev range render scene emits soft shadows for grounded actors and props");
    expect(frame.contactShadows.size() == stats.contactShadowCount, "contact shadow submissions match scene statistics");
    expect(stats.firstPersonMeshCount == 2, "dev range render scene emits weapon and one arms viewmodel mesh for first-person");
    expect(stats.firstPersonBodyPrimitiveCount == 0, "real arms are not overdrawn by hand and forearm proxies");
    expect(stats.firstPersonSocketCount == nemisis::player::kFirstPersonRigSocketCount, "first-person rig exposes all camera, hand, weapon, muzzle, and ejection sockets");
    expect(stats.targetMeshCount == targetRange.lanes.size(), "dev range render scene emits one actor mesh per target lane");
    expect(stats.worldLineCount == 3, "dev range render scene emits aim, ground-normal, and contact lines");
    expect(frame.worldLines.size() == 3, "frame receives world debug lines");

    expect(findMesh(frame, "env_test_arena_kit_01").has_value(), "dev range render scene still submits the arena kit");

    const auto firstPersonRifle = findLastMesh(frame, "wpn_project_rifle_m4a1");
    expect(firstPersonRifle.has_value(), "first-person Project rifle mesh is submitted");
    if (firstPersonRifle.has_value()) {
        expect(firstPersonRifle->position.z > player.position.z + 0.45F, "first-person rifle sits in front of the camera");
        expect(firstPersonRifle->yawDegrees > 31.0F && firstPersonRifle->yawDegrees < 33.0F, "first-person rifle uses normalized Project asset forward axis");
        expect(std::abs(firstPersonRifle->rollDegrees + player.cameraRollDegrees) < 5.0F,
               "rifle stays upright relative to the rolled camera without legacy import correction");
        expect(firstPersonRifle->scale.x > 0.9F && firstPersonRifle->scale.x < 1.1F, "first-person rifle uses normalized Project asset scale");
        expect(firstPersonRifle->material.specularScale > 1.30F, "first-person rifle uses a weapon material fallback profile");
        expect(firstPersonRifle->layer == novacore::render::RenderMeshLayer::ViewModel, "first-person rifle uses isolated viewmodel depth");
    }
    const auto firstPersonArms = findLastMesh(frame, "chr_a1_fp_arms_01");
    expect(firstPersonArms.has_value(), "first-person arms mesh is submitted");
    if (firstPersonArms.has_value()) {
        expect(firstPersonArms->layer == novacore::render::RenderMeshLayer::ViewModel, "first-person arms use isolated viewmodel depth");
    }
}

void testDevRangeRenderSceneMovesWeaponTowardSightlineInAds() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState hipPlayer{};
    hipPlayer.position = world.playerSpawn;
    hipPlayer.view.yawDegrees = 0.0F;
    hipPlayer.view.pitchDegrees = 0.0F;
    hipPlayer.hasMovementState = true;
    hipPlayer.activeWeaponId = "ar_01";
    hipPlayer.activeWeaponClass = nemisis::weapons::WeaponClass::AssaultRifle;
    useFirstPersonCamera(hipPlayer);

    auto adsPlayer = hipPlayer;
    adsPlayer.adsAlpha = 1.0F;

    novacore::render::RenderFrameInfo hipFrame{};
    (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        hipFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, hipPlayer});

    novacore::render::RenderFrameInfo adsFrame{};
    (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        adsFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, adsPlayer});

    const auto hipRifle = findLastMesh(hipFrame, "wpn_project_rifle_m4a1");
    const auto adsRifle = findLastMesh(adsFrame, "wpn_project_rifle_m4a1");
    expect(hipRifle.has_value() && adsRifle.has_value(), "hip and ADS frames submit first-person rifle");
    if (hipRifle.has_value() && adsRifle.has_value()) {
        expect(std::abs(adsRifle->position.x) < std::abs(hipRifle->position.x), "ADS pulls rifle closer to camera centerline");
        expect(adsRifle->position.z < hipRifle->position.z, "ADS pulls rifle slightly closer to the player");
        expect(adsRifle->position.y > hipRifle->position.y, "ADS raises rifle toward eye line");
    }
}

void testDevRangeRenderSceneViewmodelFollowsCameraAngles() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 47.0F;
    player.view.pitchDegrees = 28.0F;
    player.cameraRollDegrees = 13.0F;
    player.hasMovementState = true;
    player.activeWeaponId = "ar_01";
    useFirstPersonCamera(player);

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, player});

    const auto weapon = findLastMesh(frame, "wpn_project_rifle_m4a1");
    const auto arms = findLastMesh(frame, "chr_a1_fp_arms_01");
    expect(stats.firstPersonViewmodelActive, "camera angles test uses the viewmodel path");
    expect(weapon.has_value() && arms.has_value(), "camera angles test submits weapon and arms meshes");
    if (weapon.has_value() && arms.has_value()) {
        expect(std::abs(weapon->yawDegrees - 47.0F) < 0.01F, "weapon mesh follows camera yaw");
        expect(std::abs(weapon->pitchDegrees + 28.0F) < 0.01F, "weapon mesh converts camera pitch to renderer pitch");
        expect(std::abs(weapon->rollDegrees + 13.0F) < 0.01F, "proper mesh roll uses the opposite sign to camera roll");
        expect(std::abs(arms->yawDegrees - 47.0F) < 0.01F, "arms mesh follows camera yaw");
        expect(std::abs(arms->pitchDegrees + 28.0F) < 0.01F, "arms mesh converts camera pitch to renderer pitch");
        expect(std::abs(arms->rollDegrees + 13.0F) < 0.01F, "proper arms mesh roll uses the opposite sign to camera roll");
    }
}

void testDevRangeRenderSceneAlignsWeaponMeshToCookedMuzzleSocket() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    auto socketCatalog = makeSocketCatalogForFirstPersonWeapon();
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 0.0F;
    player.view.pitchDegrees = 0.0F;
    player.hasMovementState = true;
    player.activeWeaponId = "ar_01";
    player.activeWeaponClass = nemisis::weapons::WeaponClass::AssaultRifle;
    useFirstPersonCamera(player);

    novacore::render::RenderFrameInfo fallbackFrame{};
    (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        fallbackFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, player});

    novacore::render::RenderFrameInfo socketFrame{};
    nemisis::dev::DevRangeRenderSceneDesc desc{&world, &targetRange, nullptr, &lookup, player};
    desc.meshCatalog = &socketCatalog;
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(socketFrame, desc);

    const auto fallbackRifle = findLastMesh(fallbackFrame, "wpn_project_rifle_m4a1");
    const auto socketRifle = findLastMesh(socketFrame, "wpn_project_rifle_m4a1");
    expect(fallbackRifle.has_value() && socketRifle.has_value(), "socket alignment test submits first-person rifle in both paths");
    expect(stats.firstPersonCookedSocketCount >= 4U, "first-person scene consumes cooked weapon socket nodes");
    expect(stats.firstPersonSocketAlignedMeshCount == 1U, "first-person weapon mesh aligns to cooked muzzle socket");
    expect(stats.firstPersonGripSocketBoundCount == 2U, "first-person hand rig binds to cooked right and left weapon grips");
    expect(stats.firstPersonArmsSocketBoundCount >= 2U, "first-person arms mesh binds to cooked hand sockets instead of floating from a static mount");
    if (fallbackRifle.has_value() && socketRifle.has_value()) {
        expect(socketRifle->position.z > fallbackRifle->position.z + 0.25F, "positive runtime muzzle socket keeps the weapon root behind the barrel and in front of the camera");
        expect(std::abs(socketRifle->yawDegrees - fallbackRifle->yawDegrees) < 0.01F, "socket alignment keeps normalized asset yaw stable");
        expect(std::abs(socketRifle->rollDegrees - fallbackRifle->rollDegrees) < 0.01F, "socket alignment keeps normalized asset roll stable");
    }
}

// Independent quaternion oracle for renderer T * Ry * Rx * Rz * S.
novacore::math::Vec3 worldPoint(const novacore::render::RenderMesh3D& mesh, novacore::math::Vec3 point) {
    constexpr float halfRadians = 3.14159265358979323846F / 360.0F;
    const float y = mesh.yawDegrees * halfRadians;
    const float x = mesh.pitchDegrees * halfRadians;
    const float z = mesh.rollDegrees * halfRadians;
    const auto rotation = novacore::animation::multiply(
        novacore::math::Quat{0.0F, std::sin(y), 0.0F, std::cos(y)},
        novacore::animation::multiply(novacore::math::Quat{std::sin(x), 0.0F, 0.0F, std::cos(x)},
            novacore::math::Quat{0.0F, 0.0F, std::sin(z), std::cos(z)}));
    return novacore::animation::transformPoint(
        novacore::animation::matrixFromTransform({mesh.position, rotation, mesh.scale}), point);
}

void testCookedMuzzleAndGripsFollowRenderedWeaponTransform() {
    novacore::render::Renderer renderer;
    const auto lookup = registerSceneMeshes(renderer);
    const auto catalog = makeSocketCatalogForFirstPersonWeapon();
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targets = nemisis::dev::makeDefaultDevTargetRange();
    const auto* source = catalog.findByAssetId("wpn_project_rifle_m4a1");
    expect(source != nullptr && source->meshData.has_value(), "socket fixture has imported markers");
    if (source == nullptr || !source->meshData.has_value()) {
        return;
    }
    const auto near = [](novacore::math::Vec3 a, novacore::math::Vec3 b) {
        return (a - b).lengthSquared() < 0.0000001F;
    };
    for (const float ads : {0.0F, 1.0F}) {
        nemisis::dev::DevRangePlayerRenderState player{};
        player.position = {3.0F, 2.0F, -5.0F};
        player.view.yawDegrees = 37.0F;
        player.view.pitchDegrees = 19.0F;
        player.cameraRollDegrees = -13.0F;
        player.adsAlpha = ads;
        player.weapon.adsAlpha = ads;
        player.weapon.recoilPitchOffsetDegrees = 3.0F;
        player.weapon.recoilYawOffsetDegrees = 2.0F;
        player.hasAnimationFrame = true;
        useFirstPersonCamera(player);
        novacore::render::RenderFrameInfo fallbackFrame{};
        nemisis::dev::DevRangeRenderSceneDesc desc{&world, &targets, nullptr, &lookup, player};
        // The proxy boxes visualize the mathematical grip targets; they do not
        // prove that the separate skinned hands currently meet those targets.
        desc.showFirstPersonRigDiagnostics = true;
        (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(fallbackFrame, desc);
        desc.meshCatalog = &catalog;
        novacore::render::RenderFrameInfo cookedFrame{};
        (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(cookedFrame, desc);
        const auto fallback = findLastMesh(fallbackFrame, "wpn_project_rifle_m4a1");
        const auto cooked = findLastMesh(cookedFrame, "wpn_project_rifle_m4a1");
        expect(fallback.has_value() && cooked.has_value(), "hip and ADS submit the representative viewmodel");
        if (!fallback.has_value() || !cooked.has_value()) {
            continue;
        }
        const auto muzzle = std::ranges::find_if(source->meshData->nodeMarkers, [](const auto& marker) { return marker.name == "socket_muzzle"; });
        expect(muzzle != source->meshData->nodeMarkers.end(), "cooked fixture contains muzzle");
        if (muzzle != source->meshData->nodeMarkers.end()) {
            expect(near(worldPoint(*cooked, muzzle->worldPosition),
                        worldPoint(*fallback, nemisis::player::FirstPersonRigSocketLayout{}.muzzle)),
                "cooked muzzle reaches the procedural target under yaw, pitch, roll, recoil and ADS scale");
        }
        for (const auto name : {"socket_grip_r", "socket_grip_l"}) {
            const auto grip = std::ranges::find_if(source->meshData->nodeMarkers, [name](const auto& marker) { return marker.name == name; });
            expect(grip != source->meshData->nodeMarkers.end(), "cooked fixture contains required grip");
            if (grip != source->meshData->nodeMarkers.end()) {
                const auto expected = worldPoint(*cooked, grip->worldPosition);
                expect(std::ranges::any_of(cookedFrame.worldBoxes, [&](const auto& box) { return near(box.center, expected); }),
                    "presented hand primitive coincides with the transformed cooked grip without fixing stage counts");
            }
        }
    }
}

void testSharedPresentationUsesActualCookedM4AndOneSnapshot() {
    novacore::render::MeshCatalog catalog;
    for (const auto& [id, relativePath] : std::array{
             std::pair{"wpn_project_rifle_m4a1", "assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb"},
             std::pair{"chr_a1_fp_arms_01", "assets/export/gltf/characters/chr_a1_fp_arms_01.glb"}}) {
        novacore::assets::GltfMeshData imported;
        const auto load = novacore::assets::loadGltfMeshData(
            std::filesystem::path(NEMISIS_SOURCE_ROOT) / relativePath, imported);
        expect(load.ok(), "shared presentation imports its actual cooked weapon and arms assets");
        if (!load.ok()) {
            return;
        }
        novacore::assets::AssetRecord record;
        record.id = id;
        record.kind = novacore::assets::AssetKind::Mesh;
        record.cookedPath = relativePath;
        novacore::assets::GltfAssetMetadata metadata;
        metadata.id = id;
        metadata.exportPath = relativePath;
        metadata.scaleMeters = true;
        metadata.runtimeUpAxis = "Y";
        metadata.gameplayForwardAxis = "+Z";
        metadata.license = "original_project_asset";
        expect(catalog.registerImportedGltfAsset(record, metadata, std::move(imported)).isValid(),
            "actual cooked asset registers for shared presentation");
    }
    const auto* weaponSource = catalog.findByAssetId("wpn_project_rifle_m4a1");
    expect(weaponSource != nullptr && weaponSource->meshData.has_value(), "actual M4 has imported marker data");
    if (weaponSource == nullptr || !weaponSource->meshData.has_value()) {
        return;
    }

    novacore::render::Renderer renderer;
    const auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto targets = nemisis::dev::makeDefaultDevTargetRange();
    const nemisis::dev::DevRangeRenderSceneBuilder builder;
    for (int poseIndex = 0; poseIndex < 3; ++poseIndex) {
        nemisis::dev::DevRangePlayerRenderState player;
        player.position = {3.0F, 2.0F, -5.0F};
        player.adsAlpha = poseIndex == 0 ? 0.0F : poseIndex == 1 ? 1.0F : 0.65F;
        player.weapon.adsAlpha = player.adsAlpha;
        player.hasAnimationFrame = true;
        if (poseIndex == 2) {
            player.view.yawDegrees = 37.0F;
            player.view.pitchDegrees = 19.0F;
            player.cameraRollDegrees = -13.0F;
            player.weapon.recoilPitchOffsetDegrees = 3.0F;
            player.weapon.recoilYawOffsetDegrees = -2.0F;
            player.weapon.reloading = true;
            player.weapon.reloadProgress = 0.4F;
            player.weaponSwayOffset = {0.03F, -0.015F, -0.02F};
            player.animation.reloadAlpha = 0.8F;
            player.animation.firstPersonWeaponOffset = {0.01F, -0.02F, 0.025F};
            player.animation.firstPersonWeaponYawAddDegrees = 1.5F;
            player.animation.firstPersonWeaponPitchAddDegrees = -2.0F;
            player.animation.firstPersonWeaponRollAddDegrees = 3.0F;
        }
        useFirstPersonCamera(player);
        nemisis::dev::DevRangeRenderSceneDesc desc{&world, &targets, nullptr, &lookup, player};
        desc.meshCatalog = &catalog;
        const auto presentation = nemisis::dev::evaluateFirstPersonPresentation(desc);
        expect(presentation.weaponAssetId == "wpn_project_rifle_m4a1", "actual M4 owns the evaluated snapshot");
        expect(presentation.muzzleWorld.has_value() && presentation.rightGripWorld.has_value() &&
                presentation.leftGripWorld.has_value(),
            "actual M4 exposes complete muzzle and both grip targets");

        novacore::render::RenderFrameInfo immediate;
        const auto immediateStats = builder.append(immediate, desc);
        desc.firstPersonPresentation = &presentation;
        novacore::render::RenderFrameInfo cached;
        const auto cachedStats = builder.append(cached, desc);
        const auto weapon = findLastMesh(cached, "wpn_project_rifle_m4a1");
        const auto arms = findLastMesh(cached, "chr_a1_fp_arms_01");
        const auto immediateWeapon = findLastMesh(immediate, "wpn_project_rifle_m4a1");
        const auto immediateArms = findLastMesh(immediate, "chr_a1_fp_arms_01");
        expect(weapon.has_value() && arms.has_value() && immediateWeapon.has_value() && immediateArms.has_value(),
            "cached and immediate presentation submit weapon and arms");
        if (!weapon || !arms || !immediateWeapon || !immediateArms) {
            continue;
        }
        const auto renderedWorld = renderedMeshMatrix(*weapon);
        expect(matricesNear(presentation.weaponWorld, renderedWorld),
            "renderer uses the exact final weapon matrix offered to hand IK");
        expect(matricesNear(nemisis::player::firstPersonAttachmentMatrix(presentation.rig.weapon), renderedWorld),
            "attachment conversion matches the renderer model convention");
        expect(matricesNear(renderedMeshMatrix(*immediateWeapon), renderedWorld) &&
                matricesNear(renderedMeshMatrix(*immediateArms), renderedMeshMatrix(*arms)),
            "extracting a cached presentation preserves legacy weapon and arms transforms");
        expect(immediateStats.firstPersonCookedSocketCount == cachedStats.firstPersonCookedSocketCount &&
                immediateStats.firstPersonArmsSocketBoundCount == cachedStats.firstPersonArmsSocketBoundCount &&
                cachedStats.firstPersonSocketAlignedMeshCount == 1U && cachedStats.firstPersonGripSocketBoundCount == 2U,
            "cached presentation preserves socket diagnostics and applies muzzle alignment once");

        for (const auto& marker : weaponSource->meshData->nodeMarkers) {
            const std::optional<novacore::animation::Mat4>* target = nullptr;
            if (marker.name == "socket_muzzle") {
                target = &presentation.muzzleWorld;
            } else if (marker.name == "socket_grip_r") {
                target = &presentation.rightGripWorld;
            } else if (marker.name == "socket_grip_l") {
                target = &presentation.leftGripWorld;
            }
            if (target != nullptr && target->has_value()) {
                expectMarkerFrame(**target, renderedWorld, marker);
            }
        }
        // A frame may select a loadout after the most recent fixed update.
        // Its existing snapshot must stay paired with the already skinned hands.
        desc.player.activeWeaponId = "sidearm_01";
        desc.player.activeWeaponClass = nemisis::weapons::WeaponClass::Sidearm;
        desc.player.weapon.recoilPitchOffsetDegrees = 25.0F;
        desc.player.adsAlpha = 1.0F - player.adsAlpha;
        desc.player.weapon.adsAlpha = desc.player.adsAlpha;
        novacore::render::RenderFrameInfo changedState;
        (void)builder.append(changedState, desc);
        const auto retainedWeapon = findLastMesh(changedState, "wpn_project_rifle_m4a1");
        const auto retainedArms = findLastMesh(changedState, "chr_a1_fp_arms_01");
        expect(retainedWeapon && retainedWeapon->layer == novacore::render::RenderMeshLayer::ViewModel &&
                matricesNear(renderedMeshMatrix(*retainedWeapon), renderedWorld),
            "a cached tick retains its selected M4 and exact pose despite subsequent loadout state changes");
        expect(retainedArms && matricesNear(renderedMeshMatrix(*retainedArms), renderedMeshMatrix(*arms)),
            "cached arms and weapon retain the same presentation snapshot");
        expect(std::ranges::count_if(changedState.worldMeshes, [](const auto& mesh) {
                   return mesh.assetId == "chr_a1_fp_arms_01" && mesh.layer == novacore::render::RenderMeshLayer::ViewModel;
               }) == 1,
            "consuming a shared snapshot emits exactly one arms submission");
    }
}

void testSharedPresentationPreservesAuthoredGripOrientations() {
    const auto catalog = makeSocketCatalogForFirstPersonWeapon(true);
    novacore::render::Renderer renderer;
    const auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::DevRangePlayerRenderState player;
    player.view = {28.0F, -17.0F};
    player.cameraRollDegrees = 9.0F;
    player.adsAlpha = 0.7F;
    player.weapon.adsAlpha = player.adsAlpha;
    useFirstPersonCamera(player);
    nemisis::dev::DevRangeRenderSceneDesc desc{&world, nullptr, nullptr, &lookup, player};
    desc.meshCatalog = &catalog;
    const auto presentation = nemisis::dev::evaluateFirstPersonPresentation(desc);
    desc.firstPersonPresentation = &presentation;
    novacore::render::RenderFrameInfo frame;
    (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(frame, desc);
    const auto weapon = findLastMesh(frame, "wpn_project_rifle_m4a1");
    const auto* source = catalog.findByAssetId("wpn_project_rifle_m4a1");
    expect(weapon && source && source->meshData, "rotated-grip fixture renders its weapon");
    if (!weapon || !source || !source->meshData) {
        return;
    }
    expect(presentation.rightGripWorld.has_value() && presentation.leftGripWorld.has_value(),
        "authored non-canonical grip rotations yield complete transforms");
    for (const auto& marker : source->meshData->nodeMarkers) {
        if (marker.name == "socket_grip_r" && presentation.rightGripWorld) {
            expectMarkerFrame(*presentation.rightGripWorld, renderedMeshMatrix(*weapon), marker);
        } else if (marker.name == "socket_grip_l" && presentation.leftGripWorld) {
            expectMarkerFrame(*presentation.leftGripWorld, renderedMeshMatrix(*weapon), marker);
        }
    }
    const auto invalidCatalog = makeSocketCatalogForFirstPersonWeapon(false, true);
    desc.meshCatalog = &invalidCatalog;
    desc.firstPersonPresentation = nullptr;
    const auto invalidPresentation = nemisis::dev::evaluateFirstPersonPresentation(desc);
    expect(!invalidPresentation.rightGripWorld && !invalidPresentation.leftGripWorld,
        "zero and collinear authored grip bases are rejected as IK targets");
    expect(matricesNear(invalidPresentation.weaponWorld, invalidPresentation.weaponWorld),
        "invalid grip orientation does not contaminate the final weapon matrix with non-finite values");
}

void testDevRangeRenderSceneKeepsCookedWeaponAxes() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState smgPlayer{};
    smgPlayer.position = world.playerSpawn;
    smgPlayer.view.yawDegrees = 0.0F;
    smgPlayer.view.pitchDegrees = 0.0F;
    smgPlayer.activeWeaponId = "smg_01";
    smgPlayer.activeWeaponClass = nemisis::weapons::WeaponClass::Smg;
    useFirstPersonCamera(smgPlayer);

    novacore::render::RenderFrameInfo smgFrame{};
    (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        smgFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, smgPlayer});

    const auto smgMesh = findLastMesh(smgFrame, "wpn_project_smg_fr17");
    expect(smgMesh.has_value(), "SMG first-person Project mesh is submitted");
    if (smgMesh.has_value()) {
        expect(std::abs(smgMesh->yawDegrees) < 1.0F, "SMG uses normalized forward axis without legacy yaw correction");
        expect(std::abs(smgMesh->rollDegrees) < 5.0F, "SMG uses upright normalized roll");
        expect(std::abs(smgMesh->scale.x - 1.15F) < 0.01F, "SMG keeps its asset-specific viewmodel scale");
    }

    auto sidearmPlayer = smgPlayer;
    sidearmPlayer.activeWeaponId = "sidearm_01";
    sidearmPlayer.activeWeaponClass = nemisis::weapons::WeaponClass::Sidearm;

    novacore::render::RenderFrameInfo sidearmFrame{};
    (void)nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        sidearmFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, sidearmPlayer});

    const auto sidearmMesh = findLastMesh(sidearmFrame, "wpn_project_sidearm_glock19");
    expect(sidearmMesh.has_value(), "sidearm first-person Project mesh is submitted");
    if (sidearmMesh.has_value()) {
        expect(std::abs(sidearmMesh->yawDegrees) < 1.0F, "sidearm uses normalized forward axis without legacy yaw correction");
        expect(std::abs(sidearmMesh->rollDegrees) < 5.0F, "sidearm uses upright normalized roll");
        expect(std::abs(sidearmMesh->scale.x - 1.35F) < 0.01F, "sidearm keeps its asset-specific viewmodel scale");
    }
}

void testDevRangeRenderScenePlacesShowcaseAssetsInSpawnView() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 0.0F;
    player.view.pitchDegrees = 0.0F;
    useFirstPersonCamera(player);

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            nullptr,
            &lookup,
            player,
        });

    expect(stats.meshInstanceCount == 34, "spawn view scene emits expected meshes without the old first-person fullbody T-pose mesh");
    expect(stats.skippedMeshInstanceCount == 0, "spawn view scene has no skipped A2 meshes");
    expect(stats.skyPassEnabled, "spawn view uses renderer sky pass");
    expect(frame.sky.enabled, "spawn view frame carries sky pass data");
    expect(stats.firstPersonBodyPrimitiveCount == 0, "spawn view keeps the actual arms without duplicate proxy geometry");
    expect(stats.materialFallbackProfileCount == stats.meshInstanceCount, "spawn view assigns fallback material response to imported meshes");

    const auto operatorMesh = findMesh(frame, "chr_a2_pilot_operator_01");
    const auto carbineMesh = findMesh(frame, "wpn_a2_blackout_carbine_01");
    const auto rifleMesh = findMesh(frame, "wpn_a2_modular_rifle_01");
    const auto sidearmMesh = findMesh(frame, "wpn_a2_striker_sidearm_01");
    const auto heroMesh = findMesh(frame, "prop_a2_range_hero_01");
    const auto projectCharacterMesh = findMesh(frame, "chr_project_male1");
    const auto projectRifleMesh = findMesh(frame, "wpn_project_rifle_m4a1");
    const auto projectSmgMesh = findMesh(frame, "wpn_project_smg_fr17");
    const auto projectSidearmMesh = findMesh(frame, "wpn_project_sidearm_glock19");

    expect(operatorMesh.has_value(), "A2 operator mesh is submitted");
    expect(carbineMesh.has_value(), "A2 carbine mesh is submitted");
    expect(rifleMesh.has_value(), "A2 rifle mesh is submitted");
    expect(sidearmMesh.has_value(), "A2 sidearm mesh is submitted");
    expect(heroMesh.has_value(), "A2 hero prop mesh is submitted");
    expect(projectCharacterMesh.has_value(), "Project character mesh is submitted");
    expect(!findMesh(frame, "env_project_skybox1").has_value(), "Project skybox GLB mesh is replaced by renderer sky pass");
    expect(projectRifleMesh.has_value(), "Project rifle mesh is submitted");
    expect(projectSmgMesh.has_value(), "Project SMG mesh is submitted");
    expect(projectSidearmMesh.has_value(), "Project sidearm mesh is submitted");

    if (operatorMesh.has_value()) {
        expect(operatorMesh->position.z > world.playerSpawn.z + 5.0F && operatorMesh->position.z < world.playerSpawn.z + 8.5F, "A2 operator starts in front of spawn");
        expect(operatorMesh->position.x > -4.0F && operatorMesh->position.x < -1.0F, "A2 operator is inside initial horizontal view");
        expect(operatorMesh->scale.y > 1.0F, "A2 operator is scaled for visible review");
    }
    if (carbineMesh.has_value() && rifleMesh.has_value() && sidearmMesh.has_value()) {
        expect(carbineMesh->position.y > 1.0F, "A2 carbine is raised on the review rack");
        expect(rifleMesh->position.y > 1.0F, "A2 rifle is raised on the review rack");
        expect(sidearmMesh->position.y > 1.0F, "A2 sidearm is raised on the review rack");
        expect(carbineMesh->position.z > world.playerSpawn.z + 4.5F && sidearmMesh->position.z > world.playerSpawn.z + 4.5F, "A2 weapons start in front of spawn");
    }
    if (heroMesh.has_value()) {
        expect(heroMesh->position.z > world.playerSpawn.z + 7.5F, "A2 hero prop anchors the visible asset stage");
    }
    if (projectCharacterMesh.has_value() && projectRifleMesh.has_value() && projectSmgMesh.has_value() && projectSidearmMesh.has_value()) {
        expect(projectCharacterMesh->material.saturationScale < 0.90F, "Project character uses a desaturated fabric/body fallback material profile");
        expect(projectRifleMesh->material.specularScale > 1.30F, "Project rifle uses a boosted weapon fallback material profile");
        expect(
            std::any_of(
                frame.worldMeshes.begin(),
                frame.worldMeshes.end(),
                [&world](const novacore::render::RenderMesh3D& mesh) {
                    return mesh.assetId == "chr_project_male1" &&
                        mesh.position.z > world.playerSpawn.z + 5.0F &&
                        mesh.position.x < -6.5F;
                }),
            "Project character starts in the visible asset stage");
        expect(projectRifleMesh->position.y > 0.9F, "Project rifle is raised on the review rack");
        expect(projectSmgMesh->position.y > 0.9F, "Project SMG is raised on the review rack");
        expect(projectSidearmMesh->position.y > 0.8F, "Project sidearm is raised on the review rack");
    }
}

void testDevRangeRenderSceneHidesLocalWorldBodyWhenCameraRigIsActive() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState worldBodyPlayer{};
    worldBodyPlayer.position = world.playerSpawn;
    worldBodyPlayer.view.yawDegrees = 12.0F;
    worldBodyPlayer.hasMovementState = true;

    auto cameraRigPlayer = worldBodyPlayer;
    cameraRigPlayer.hasCameraRig = true;
    cameraRigPlayer.cameraPosition = world.playerSpawn + novacore::math::Vec3{0.0F, 1.68F, 0.0F};
    cameraRigPlayer.cameraView.yawDegrees = 12.0F;
    cameraRigPlayer.cameraView.pitchDegrees = -2.0F;

    novacore::render::RenderFrameInfo worldBodyFrame{};
    const auto worldBodyStats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        worldBodyFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, worldBodyPlayer});

    novacore::render::RenderFrameInfo cameraRigFrame{};
    const auto cameraRigStats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        cameraRigFrame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, cameraRigPlayer});

    expect(worldBodyStats.localWorldModelActive, "world presentation enables the local third-person model");
    const auto localBody = findLastMesh(worldBodyFrame, nemisis::dev::kLocalPlayerBodyAssetId);
    expect(localBody.has_value(), "external view uses the explicitly selected Project character");
    if (localBody.has_value()) {
        expect(localBody->mesh.index == lookup.at(std::string(nemisis::dev::kLocalPlayerBodyResourceId)).index,
            "player body uses its own animated resource handle");
        expect(localBody->mesh.index != lookup.at(std::string(nemisis::dev::kLocalPlayerBodyAssetId)).index,
            "static showcase and targets cannot share the local animated handle");
        expect(localBody->scale.y == 1.0F && localBody->position.y == worldBodyPlayer.position.y,
            "selected metre-scale character keeps its grounded feet pivot");
    }
    expect(!worldBodyStats.firstPersonViewmodelActive, "world presentation does not submit a first-person viewmodel");
    expect(worldBodyStats.firstPersonMeshCount == 0U, "world presentation has no first-person weapon or arms meshes");
    expect(worldBodyStats.firstPersonBodyPrimitiveCount == 0U, "world presentation has no camera-linked arm primitives");
    expect(cameraRigStats.firstPersonViewmodelActive, "camera rig enables the first-person viewmodel");
    expect(!cameraRigStats.localWorldModelActive, "camera rig suppresses the duplicate local world model");
    expect(cameraRigStats.firstPersonMeshCount == 2U, "camera rig submits one weapon and one arms viewmodel mesh");
    expect(cameraRigStats.meshInstanceCount == worldBodyStats.meshInstanceCount + 1U, "exclusive first-person weapon and arms replace the single local world body");
    expect(cameraRigFrame.camera3D.position.y > world.playerSpawn.y + 1.60F, "camera rig still drives the render camera eye height");
}

void testDevRangeRenderSceneRigsFirstPersonBodyForLookDown() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.hasMovementState = true;
    player.hasCameraRig = true;
    player.cameraPosition = world.playerSpawn + novacore::math::Vec3{0.0F, 1.66F, 0.0F};
    player.cameraView.yawDegrees = 0.0F;
    player.cameraView.pitchDegrees = -55.0F;
    player.activeWeaponId = "ar_01";
    player.activeWeaponClass = nemisis::weapons::WeaponClass::AssaultRifle;
    player.adsAlpha = 0.0F;
    player.animation.firstPersonBodyOffset = {0.0F, -0.04F, 0.02F};
    player.animation.firstPersonBodyPitchDegrees = -2.0F;
    player.hasAnimationFrame = true;

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, player});

    const auto firstPersonArms = findLastMesh(frame, "chr_a1_fp_arms_01");
    const auto firstPersonWeapon = findLastMesh(frame, "wpn_project_rifle_m4a1");
    expect(stats.firstPersonMeshCount == 2, "look-down first-person rig keeps arms and weapon mesh submissions");
    expect(stats.firstPersonBodyPrimitiveCount == 0, "look-down does not place diagnostic torso and arm boxes in front of the actual viewmodel");
    expect(stats.firstPersonSocketCount == nemisis::player::kFirstPersonRigSocketCount, "look-down first-person rig keeps socket map live for camera-linked viewmodel placement");
    expect(firstPersonArms.has_value(), "look-down first-person arms mesh is submitted");
    expect(firstPersonWeapon.has_value(), "look-down first-person weapon mesh is submitted");
    expect(
        std::none_of(
            frame.worldMeshes.begin(),
            frame.worldMeshes.end(),
            [&frame](const novacore::render::RenderMesh3D& mesh) {
                const float dx = mesh.position.x - frame.camera3D.position.x;
                const float dz = mesh.position.z - frame.camera3D.position.z;
                return mesh.assetId == "chr_project_male1" &&
                    ((dx * dx) + (dz * dz)) < 0.75F &&
                    mesh.position.y < frame.camera3D.position.y - 1.0F &&
                    mesh.position.y > frame.camera3D.position.y - 3.0F;
            }),
        "look-down first-person rig no longer submits the full T-pose character mesh under the camera");
    if (firstPersonArms.has_value() && firstPersonWeapon.has_value()) {
        expect(firstPersonArms->position.y < frame.camera3D.position.y, "first-person arms stay below the camera while looking down");
        expect(firstPersonWeapon->position.y < frame.camera3D.position.y, "first-person weapon stays below the camera while looking down");
        expect(firstPersonArms->position.z > firstPersonWeapon->position.z - 1.2F, "first-person arms are socket-bound near the held weapon instead of left at the old static mount");
    }
}

void testFirstPersonArmsHaveOnePresentationOwner() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targets = nemisis::dev::makeDefaultDevTargetRange();
    nemisis::dev::DevRangePlayerRenderState player;
    player.position = world.playerSpawn;
    useFirstPersonCamera(player);
    player.movementTech.gravityInvertersActive = true;
    player.movementTech.wallRunArmTriggerPressed = true;
    player.movementTech.doubleJumpPlatformThrown = true;
    player.movementTech.energyPlatformCenter = {10,0,4};
    player.movementTech.mantleReachTriggered = true;
    player.movementTech.mantleClimbTriggered = true;
    player.movementTech.mantleTargetPosition = {8,1,2};
    nemisis::dev::DevRangeRenderSceneDesc desc{&world, &targets, nullptr, &lookup, player};
    desc.showWorldDebugLines = true;
    const auto countArms = [](const auto& frame) {
        return std::ranges::count_if(frame.worldMeshes, [](const auto& mesh) {
            return mesh.assetId == "chr_a1_fp_arms_01" && mesh.layer == novacore::render::RenderMeshLayer::ViewModel;
        });
    };
    novacore::render::RenderFrameInfo frame;
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(frame, desc);
    expect(countArms(frame) == 1, "wallrun, throw and mantle effects cannot add copies of the entire arms mesh");
    expect(stats.firstPersonBodyPrimitiveCount == 0U, "visible arms have no duplicate body/hand proxy geometry by default");
    for (const auto center : {novacore::math::Vec3{10,0,4}, novacore::math::Vec3{8,1.08F,2}}) {
        expect(std::ranges::any_of(frame.worldBoxes, [center](const auto& box) { return (box.center - center).lengthSquared() < 0.000001F; }),
            "energy platform and mantle target visual effects remain");
    }
    desc.showFirstPersonRigDiagnostics = true;
    novacore::render::RenderFrameInfo diagnostic;
    const auto diagnosticStats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(diagnostic, desc);
    expect(countArms(diagnostic) == 1 && diagnosticStats.firstPersonBodyPrimitiveCount == 4U,
        "explicit rig diagnostics visualize targets without another arm mesh");
    desc.showFirstPersonRigDiagnostics = false;
    lookup.erase("chr_a1_fp_arms_01");
    novacore::render::RenderFrameInfo fallback;
    const auto fallbackStats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(fallback, desc);
    expect(countArms(fallback) == 0 && fallbackStats.firstPersonBodyPrimitiveCount == 4U,
        "missing arms retain the existing safe procedural fallback");
}

void testDevRangeRenderSceneDrawsSocketDrivenShotFeedback() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.hasMovementState = true;
    player.hasCameraRig = true;
    player.cameraPosition = world.playerSpawn + novacore::math::Vec3{0.0F, 1.66F, 0.0F};
    player.cameraView.yawDegrees = 0.0F;
    player.cameraView.pitchDegrees = -4.0F;
    player.activeWeaponId = "ar_01";
    player.activeWeaponClass = nemisis::weapons::WeaponClass::AssaultRifle;
    player.weapon.shotIndex = 2;
    player.weapon.timeSinceLastShotSeconds = 0.02F;
    player.animation.fireAlpha = 1.0F;
    player.hasAnimationFrame = true;
    player.firedThisFrame = true;
    player.hitThisFrame = true;
    player.hasLatestShotTrace = true;
    player.latestShotOrigin = player.cameraPosition;
    player.latestShotEnd = targetRange.lanes[1].target.position;
    player.hitDistanceMeters = 18.0F;

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, player});

    expect(stats.firstPersonFeedbackPrimitiveCount >= 4U, "first-person shot feedback emits muzzle, casing, and hit primitives");
    expect(stats.hitFeedbackLineCount == 1U, "first-person shot feedback emits a socket-driven trace line");
    expect(frame.worldLines.size() == stats.worldLineCount, "shot feedback line contributes to world line stats");
}

void testDevRangeRenderSceneDrawsLanePressure() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    targetRange.lanes[1].pressure01 = 0.82F;
    targetRange.lanes[1].threatSeconds = 1.4F;
    targetRange.lanes[1].pressureActive = true;

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 0.0F;
    player.view.pitchDegrees = 0.0F;
    useFirstPersonCamera(player);

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, player});

    expect(stats.activeLanePressurePrimitiveCount == 3U, "lane pressure emits ring, beam, and threat line");
    expect(stats.worldBoxCount == (world.primitives.size() - 1U) + targetRange.lanes.size() + 15U, "lane pressure adds two world-box primitives");
    expect(stats.worldLineCount == 2U, "lane pressure adds one threat line plus aim line");
}

void testDevRangeRenderSceneAcceptsSpeedClearTargetLayout() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDevTargetRangeForDrillVariant(nemisis::dev::DevRangeDrillVariant::SpeedClear);

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 0.0F;
    player.view.pitchDegrees = 0.0F;
    useFirstPersonCamera(player);

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{&world, &targetRange, nullptr, &lookup, player});

    expect(targetRange.lanes.size() == 5U, "speed-clear drill configures five target lanes");
    expect(stats.targetMeshCount == targetRange.lanes.size(), "render scene emits one actor mesh per speed-clear target lane");
    expect(stats.worldBoxCount >= targetRange.lanes.size(), "render scene emits lane target boxes for speed-clear layout");
    expect(nemisis::dev::activeTargetLane(targetRange) != nullptr, "speed-clear target layout exposes an active lane");
}

void testDevRangeRenderSceneCountsMissingMeshHandles() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    lookup.erase("wpn_project_rifle_m4a1");
    lookup.erase("wpn_a2_modular_rifle_01");
    lookup.erase("chr_a1_fp_arms_01");
    lookup.erase("chr_dev_arms_a");

    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    targetRange.lanes[1].target.eliminated = true;

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 180.0F;
    useFirstPersonCamera(player);

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            nullptr,
            &lookup,
            player,
        });

    expect(stats.meshInstanceCount == 30, "dev range render scene still emits available static, A2, Project, and target lane meshes");
    expect(frame.worldMeshes.size() == 30, "frame mesh count drops missing weapon, arms, and first-person T-pose body handles");
    expect(stats.skippedMeshInstanceCount == 6, "dev range render scene counts missing showcase, primary, fallback, and first-person arm handles");
    expect(stats.materialFallbackProfileCount == stats.meshInstanceCount, "missing handles do not count material fallback profiles for skipped meshes");
    expect(stats.firstPersonMeshCount == 0, "first-person mesh stats reports no valid arms or weapon when those handles are missing");
    expect(stats.firstPersonBodyPrimitiveCount == 4, "first-person rig primitives still render hands and forearms when mesh handles are missing");
    expect(stats.worldLineCount == 1, "dev range render scene still emits aim line without collision sample");
}

void testDevRangeRenderSceneKeepsSkyboxMeshFallbackWhenSkyPassDisabled() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();

    nemisis::dev::DevRangePlayerRenderState player{};
    player.position = world.playerSpawn;
    player.view.yawDegrees = 90.0F;

    nemisis::dev::DevRangeRenderSceneDesc desc{
        &world,
        &targetRange,
        nullptr,
        &lookup,
        player,
    };
    desc.sky.enabled = false;

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(frame, desc);

    expect(!frame.sky.enabled, "disabled sky pass propagates to the render frame");
    expect(!stats.skyPassEnabled, "disabled sky pass is reported in scene stats");
    expect(stats.skyFallbackMeshCount == 1, "legacy skybox mesh is used as a controlled fallback");
    expect(findMesh(frame, "env_project_skybox1").has_value(), "legacy Project skybox mesh is submitted only as fallback");
    expect(stats.materialFallbackProfileCount == stats.meshInstanceCount, "fallback sky mesh still receives material fallback profile data");
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

void testDevRangeRenderSceneCanDisableDebugLines() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.grounded = true;
    collision.groundNormal = {0.0F, 1.0F, 0.0F};

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            &collision,
            &lookup,
            {},
            {},
            {},
            false,
        });

    expect(stats.worldLineCount == 0, "dev range render scene can disable world debug lines");
    expect(frame.worldLines.empty(), "frame receives no world lines when disabled");
}

void testDevRangeRenderSceneDrawsMantleCandidateDebugLines() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.mantleCandidate = true;
    collision.mantleObstaclePoint = {3.8F, 1.3F, -7.5F};
    collision.mantleTargetPosition = {3.8F, 1.3F, -6.95F};
    collision.mantlePrimitiveId = "ledge_training_mid";

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            &collision,
            &lookup,
            {},
        });

    expect(stats.worldLineCount == 3, "dev range render scene emits aim and mantle candidate lines");
    expect(frame.worldLines.size() == 3, "frame receives mantle candidate world lines");
}

void testDevRangeRenderSceneDrawsSweepDebugLines() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.swept = true;
    collision.sweepHit = true;
    collision.sweepStartPosition = {3.8F, 0.0F, -10.0F};
    collision.requestedDisplacement = {0.0F, 0.0F, 7.0F};
    collision.appliedDisplacement = {0.0F, 0.0F, 2.05F};
    collision.sweepNormal = {0.0F, 0.0F, -1.0F};
    collision.sweepPrimitiveId = "ledge_training_mid";
    collision.contacts.push_back(nemisis::dev::GreyboxContact{
        "ledge_training_mid",
        nemisis::dev::GreyboxPrimitiveKind::Ledge,
        nemisis::dev::GreyboxContactRole::Sweep,
        {3.8F, 0.16F, -7.95F},
        {0.0F, 0.0F, -1.0F},
        0.0F,
        0.42F,
        0.0F,
        true,
        false,
    });

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            &collision,
            &lookup,
            {},
        });

    expect(stats.worldLineCount == 5, "dev range render scene emits aim, sweep, and contact debug lines");
    expect(frame.worldLines.size() == 5, "frame receives sweep debug world lines");
}

void testDevRangeRenderSceneDrawsContactDebugLines() {
    novacore::render::Renderer renderer;
    auto lookup = registerSceneMeshes(renderer);
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto targetRange = nemisis::dev::makeDefaultDevTargetRange();
    nemisis::dev::GreyboxCollisionResult collision{};
    collision.contacts.push_back(nemisis::dev::GreyboxContact{
        "step_training_low",
        nemisis::dev::GreyboxPrimitiveKind::Cover,
        nemisis::dev::GreyboxContactRole::Step,
        {-3.5F, 0.36F, -7.0F},
        {0.0F, 1.0F, 0.0F},
        0.0F,
        1.0F,
        0.0F,
        false,
        true,
    });
    collision.contacts.push_back(nemisis::dev::GreyboxContact{
        "wallrun_left_panel_a",
        nemisis::dev::GreyboxPrimitiveKind::WallRunPanel,
        nemisis::dev::GreyboxContactRole::Wall,
        {-18.0F, 1.0F, -5.5F},
        {1.0F, 0.0F, 0.0F},
        0.0F,
        1.0F,
        0.0F,
        true,
        false,
    });

    novacore::render::RenderFrameInfo frame{};
    const auto stats = nemisis::dev::DevRangeRenderSceneBuilder{}.append(
        frame,
        nemisis::dev::DevRangeRenderSceneDesc{
            &world,
            &targetRange,
            &collision,
            &lookup,
            {},
        });

    expect(stats.worldLineCount == 3, "dev range render scene emits aim plus two contact lines");
    expect(frame.worldLines.size() == 3, "frame receives contact debug world lines");
}

#if defined(NEMISIS_PROJECT_FPS_CANDIDATE_ROOT)
void testActualProjectFPSPreviewSubmitsAnimatedSubsets() {
    namespace dev = nemisis::dev;
    namespace player = nemisis::player;
    using namespace novacore;
    render::Renderer renderer;
    auto resources = registerSceneMeshes(renderer); // Includes A1 to detect accidental fallback.
    assets::GltfMeshData body, arms;
    const std::filesystem::path root{NEMISIS_PROJECT_FPS_CANDIDATE_ROOT};
    const bool loaded = assets::loadGltfMeshData(root / "body_runtime.glb", body).ok() &&
        assets::loadGltfMeshData(root / "arms_runtime.glb", arms).ok();
    expect(loaded && body.vertexCount() == 40532U && arms.vertexCount() == 9536U,
           "preview imports actual Project candidate geometry");
    if (!loaded) { return; }
    player::PlayerSkeletalAnimator bodyAnimator, armsAnimator, commonAnimator;
    if (!bodyAnimator.initialize(body) || !armsAnimator.initialize(arms) || !commonAnimator.initialize(arms, &body)) {
        expect(false, "both actual Project preview subsets initialize authored animation"); return;
    }
    for (const auto name : {"arms", "body"}) {
        assets::GltfMeshData baseline;
        player::PlayerSkeletalAnimator reference;
        if (!assets::loadGltfMeshData(root / (std::string(name) + ".glb"), baseline).ok() || !reference.initialize(baseline)) {
            expect(false, "original subset initializes for independent canonical-frame parity"); return;
        }
        const auto* corrected = std::string_view(name) == "arms" ? armsAnimator.skinnedMesh() : bodyAnimator.skinnedMesh();
        for (std::size_t p = 0; p < corrected->primitives.size(); ++p) {
            const auto& before = reference.skinnedMesh()->primitives[p];
            const auto& after = corrected->primitives[p];
            expect(before.indices == after.indices && before.weights == after.weights && before.joints == after.joints,
                   "canonical frame preserves original topology, fingers and weights");
            for (std::size_t v = 0; v < before.positions.size(); ++v) {
                const auto q = before.positions[v];
                const math::Vec3 expected{0.10315F-q.x, q.y, 0.10893F-q.z};
                if ((expected-after.positions[v]).lengthSquared() > 0.000000001F) {
                    expect(false, "actual skinned vertices receive the canonical scene transform exactly once"); break;
                }
            }
        }
    }
    resources[std::string(dev::kProjectFPSBodyResourceId)] = renderer.registerMeshResource(
        std::string(dev::kProjectFPSBodyResourceId), *bodyAnimator.skinnedMesh(), render::MeshResourceUsage::DynamicVertices);
    resources[std::string(dev::kProjectFPSArmsResourceId)] = renderer.registerMeshResource(
        std::string(dev::kProjectFPSArmsResourceId), *armsAnimator.skinnedMesh(), render::MeshResourceUsage::DynamicVertices);
    const auto world = dev::createDevRangeGreyboxWorld();
    dev::DevRangeRenderSceneDesc desc;
    desc.greyboxWorld = &world;
    desc.meshResources = &resources;
    desc.projectFPSPreview = true;
    desc.player.hasMovementState = true;
    desc.player.hasCameraRig = true;
    desc.player.hasAnimationFrame = true;
    desc.player.position = {3, 0, -5};
    animation::Mat4 eye;
    expect(bodyAnimator.socketTransform("socket_fps_eye", eye), "actual Project body exposes an animated anatomical eye reference");
    desc.player.cameraPosition = desc.player.position + animation::matrixTranslation(eye);
    desc.showWorldDebugLines = true;
    dev::DevRangeRenderSceneBuilder builder;
    const auto initialArmVertex = armsAnimator.skinnedMesh()->primitives[0].positions[0];
    // CPU frustum sanity check for the two required screenshot cases. This is
    // not a pixel/occlusion oracle and does not replace actual Vulkan images.
    const auto hip = dev::evaluateFirstPersonPresentation(desc);
    const auto armsMatrix = player::firstPersonAttachmentMatrix(hip.rig.arms);
    std::size_t hipVertices = 0, lookDownVertices = 0;
    for (const auto& primitive : armsAnimator.skinnedMesh()->primitives) {
        for (const auto p : primitive.positions) {
            const auto v = animation::transformPoint(armsMatrix, p) - desc.player.cameraPosition;
            if (v.z > 0.004F && std::abs(v.y) < v.z * 0.6745F && std::abs(v.x) < v.z * 1.199F) { ++hipVertices; }
        }
    }
    for (const auto& primitive : bodyAnimator.skinnedMesh()->primitives) {
        for (const auto p : primitive.positions) {
            const auto v = p + desc.player.position - desc.player.cameraPosition;
            const float depth = -0.939693F * v.y + 0.342020F * v.z;
            const float screenY = 0.342020F * v.y + 0.939693F * v.z;
            if (depth > 0.03F && std::abs(screenY) < depth * 0.7536F && std::abs(v.x) < depth * 1.3397F) { ++lookDownVertices; }
        }
    }
    expect(lookDownVertices > 100U, "actual player-root body intersects the anatomical eye look-down frustum");
    std::cout << "Project preview CPU frustum: hip_arm_vertices=" << hipVertices << " look_down_body_vertices=" << lookDownVertices << '\n';
    for (const auto clip : {player::CharacterAnimationClip::Idle, player::CharacterAnimationClip::Walk,
                            player::CharacterAnimationClip::Sprint}) {
        desc.player.animation.locomotionClip = clip;
        desc.player.animation.stride01 = 0.65F;
        for (int tick = 0; tick < 12; ++tick) {
            expect(bodyAnimator.update(desc.player.animation, 1.0F / 60.0F) &&
                   armsAnimator.update(desc.player.animation, 1.0F / 60.0F), "authored Project locomotion samples and skins");
            expect(commonAnimator.update(desc.player.animation, 1.0F / 60.0F), "one common Project pose advances once");
            expect(renderer.updateMeshResourceVertices(resources.at(std::string(dev::kProjectFPSBodyResourceId)), *bodyAnimator.skinnedMesh()) &&
                   renderer.updateMeshResourceVertices(resources.at(std::string(dev::kProjectFPSArmsResourceId)), *armsAnimator.skinnedMesh()),
                   "both real Project dynamic resources accept the new pose");
        }
        for (const auto pair : {std::pair{armsAnimator.skinnedMesh(), commonAnimator.skinnedMesh()},
                                std::pair{bodyAnimator.skinnedMesh(), commonAnimator.skinnedCompanion()}}) {
            bool same = pair.first && pair.second && pair.first->primitives.size() == pair.second->primitives.size();
            if (same) {
                for (std::size_t p = 0; p < pair.first->primitives.size(); ++p) {
                    const auto& a = pair.first->primitives[p];
                    const auto& b = pair.second->primitives[p];
                    same &= a.positions.size() == b.positions.size() && a.joints == b.joints && a.weights == b.weights;
                    for (std::size_t v = 0; same && v < a.positions.size(); ++v) {
                        same &= (a.positions[v]-b.positions[v]).lengthSquared() < 1e-10F;
                    }
                }
            }
            expect(same, "merged source channels/common time preserve each original palette's actual skinned geometry");
        }
        for (const auto view : {player::PlayerViewComponent{0, 0}, player::PlayerViewComponent{38, -70},
                               player::PlayerViewComponent{-46, 65}}) {
            desc.player.view = view;
            desc.player.cameraView = view;
            desc.player.animation.thirdPersonBodyOffset = {0, -0.07F, 0.02F};
            auto presentation = dev::evaluateFirstPersonPresentation(desc);
            desc.firstPersonPresentation = &presentation;
            render::RenderFrameInfo frame;
            const auto stats = builder.append(frame, desc);
            expect(stats.projectFPSBodyActive && stats.projectFPSArmsActive && !stats.localWorldModelActive,
                   "FPS preview submits head-excluded body and matching arms, never full local world body");
            expect(presentation.armsAssetId == dev::kProjectFPSArmsAssetId && presentation.armsBoundCount == 0,
                   "Project preview owns its selected arms without A1 averaging");
            std::size_t bodyCount = 0, armsCount = 0;
            for (const auto& mesh : frame.worldMeshes) {
                expect(mesh.assetId != "chr_a1_fp_arms_01", "Project preview submits no A1 mesh, including diagnostics");
                if (mesh.assetId == dev::kProjectFPSBodyAssetId) {
                    ++bodyCount;
                    expect(mesh.mesh == resources.at(std::string(dev::kProjectFPSBodyResourceId)) &&
                           mesh.layer == render::RenderMeshLayer::World && mesh.yawDegrees == view.yawDegrees &&
                           (mesh.position - desc.player.position - desc.player.animation.thirdPersonBodyOffset).lengthSquared() < 0.000001F &&
                           mesh.scale.x == 1 && mesh.scale.y == 1 && mesh.scale.z == 1,
                           "Project body uses dedicated dynamic resource at the real player root and metre scale");
                } else if (mesh.assetId == dev::kProjectFPSArmsAssetId) {
                    ++armsCount;
                    expect(mesh.mesh == resources.at(std::string(dev::kProjectFPSArmsResourceId)) &&
                           mesh.layer == render::RenderMeshLayer::World &&
                           (mesh.position - desc.player.position - desc.player.animation.thirdPersonBodyOffset).lengthSquared() < 0.000001F &&
                           mesh.yawDegrees == view.yawDegrees && mesh.pitchDegrees == desc.player.animation.thirdPersonBodyPitchDegrees,
                           "actual Project arms share the body's player root, projection and depth");
                }
            }
            expect(bodyCount == 1 && armsCount == 1 && stats.firstPersonBodyPrimitiveCount == 0,
                   "exactly one submission of each Project subset and no procedural body/arm proxies");
            const auto samples = armsAnimator.stats().sampledFrameCount;
            render::RenderFrameInfo repeat;
            (void)builder.append(repeat, desc);
            expect(armsAnimator.stats().sampledFrameCount == samples, "extra render frame does not sample or skin again");
            desc.firstPersonPresentation = nullptr;
        }
    }
    expect(bodyAnimator.stats().sampledFrameCount == 36U && armsAnimator.stats().sampledFrameCount == 36U &&
           bodyAnimator.stats().skinnedFrameCount == 36U && armsAnimator.stats().skinnedFrameCount == 36U,
           "one authored sample/skin per requested tick, same clocks for Project subsets");
    expect((initialArmVertex - armsAnimator.skinnedMesh()->primitives[0].positions[0]).lengthSquared() > 0.000001F,
           "authored locomotion deforms actual Project sleeve vertices");
    // Exercise interrupted locomotion fades and complete looping clips; short
    // startup-only sampling cannot expose failures during real movement.
    for (int tick = 0; tick < 360; ++tick) {
        const auto phase = (tick / 7) % 3;
        desc.player.animation.locomotionClip = phase == 0 ? player::CharacterAnimationClip::Idle :
            phase == 1 ? player::CharacterAnimationClip::Walk : player::CharacterAnimationClip::Sprint;
        const bool bodyReady = bodyAnimator.update(desc.player.animation, 1.0F / 60.0F);
        const bool armsReady = armsAnimator.update(desc.player.animation, 1.0F / 60.0F);
        if (!bodyReady || !armsReady) {
            for (const auto& error : bodyAnimator.errors()) { std::cerr << "Project body: " << error << '\n'; }
            for (const auto& error : armsAnimator.errors()) { std::cerr << "Project arms: " << error << '\n'; }
            expect(false, "Project subsets survive repeated interrupted locomotion fades");
            break;
        }
    }
    desc.player.animation.locomotionClip = player::CharacterAnimationClip::Idle;
    for (int tick = 0; tick < 40; ++tick) {
        expect(bodyAnimator.update(desc.player.animation, 1.0F / 60.0F) &&
               armsAnimator.update(desc.player.animation, 1.0F / 60.0F), "latest locomotion request settles after an active fade");
    }
    expect(bodyAnimator.stats().currentClip.find("stand") != std::string::npos &&
           bodyAnimator.stats().currentClip == armsAnimator.stats().currentClip &&
           bodyAnimator.stats().failedFrameCount == 0 && armsAnimator.stats().failedFrameCount == 0,
           "both Project subsets settle to authored idle without a rejected overlapping fade");
    resources.erase(std::string(dev::kProjectFPSArmsResourceId));
    render::RenderFrameInfo missing;
    expect(!builder.append(missing, desc).projectFPSArmsActive, "missing Project resource is reported, not replaced with A1");
    expect(std::ranges::none_of(missing.worldMeshes, [](const auto& mesh) { return mesh.assetId == "chr_a1_fp_arms_01"; }),
           "missing Project resource never silently falls back");
    desc.player.hasCameraRig = false;
    render::RenderFrameInfo external;
    const auto externalStats = builder.append(external, desc);
    expect(externalStats.localWorldModelActive && !externalStats.projectFPSBodyActive && !externalStats.projectFPSArmsActive,
           "external scene retains separate complete Project world representation");
}
#endif

#if defined(NEMISIS_PROJECT_FPS_CANDIDATE_ROOT) && defined(NEMISIS_ENABLE_HAND_IK)
void testProjectReadyUsesOneConstrainedPose() {
    using namespace novacore;
    namespace dev = nemisis::dev;
    namespace player = nemisis::player;
    const std::filesystem::path root{NEMISIS_PROJECT_FPS_CANDIDATE_ROOT};
    assets::GltfMeshData arms, body, weapon;
    if (!assets::loadGltfMeshData(root / "arms_ready.glb", arms).ok() ||
        !assets::loadGltfMeshData(root / "body_ready.glb", body).ok() ||
        !assets::loadGltfMeshData(root / "rifle_ready.glb", weapon).ok()) {
        expect(false, "actual Ready variants import");
        return;
    }
    player::PlayerSkeletalAnimator animator;
    if (!animator.initialize(arms, &body)) {
        for (const auto& error : animator.errors()) {
            std::cerr << error << '\n';
        }
        expect(false, "one animator accepts real Project subsets");
        return;
    }
    expect(animator.stats().currentClip == "Developer Rifle Ready Idle", "actual new Ready clip is selected");
    expect(animator.skinnedMesh() && animator.skinnedCompanion(),
           "both original palettes are skinned from the common pose");
    expect(arms.animationData->skins[0].joints.size() == 81 && body.animationData->skins[0].joints.size() == 57,
           "optimized input palettes remain independent and unchanged");
    // Project palms blend several hand/finger bones: no vertex has >.75 weight
    // on DEF-hand alone (actual maximum is .572). Select the real anatomical
    // hand subtree by source hierarchy, retaining the .75 total-weight threshold.
    const auto& handPalette = arms.animationData->skins[0].joints;
    std::vector<std::array<bool, 2>> handOwnership(handPalette.size());
    for (std::size_t slot = 0; slot < handPalette.size(); ++slot) {
        for (std::size_t side = 0; side < 2; ++side) {
            int cursor = static_cast<int>(slot);
            for (std::size_t depth = 0; cursor >= 0 && depth < handPalette.size(); ++depth) {
                const auto& joint = handPalette[static_cast<std::size_t>(cursor)];
                if (joint.name == (side == 0 ? "DEF-hand.R" : "DEF-hand.L")) {
                    handOwnership[slot][side] = true;
                    break;
                }
                cursor = joint.parentJointIndex;
            }
        }
    }
    render::MeshCatalog catalog;
    render::Renderer renderer;
    auto resources = registerSceneMeshes(renderer);
    for (auto entry :
         {std::pair{dev::kProjectFPSArmsAssetId, &arms}, std::pair{dev::kProjectReadyWeaponAssetId, &weapon}}) {
        assets::AssetRecord record;
        record.id = entry.first;
        record.kind = assets::AssetKind::Mesh;
        record.cookedPath = entry.second->path;
        assets::GltfAssetMetadata metadata;
        metadata.id = entry.first;
        metadata.exportPath = record.cookedPath;
        metadata.scaleMeters = true;
        metadata.runtimeUpAxis = "Y";
        metadata.gameplayForwardAxis = "+Z";
        metadata.license = "original_project_asset";
        expect(catalog.registerImportedGltfAsset(record, metadata, *entry.second).isValid(),
               "Ready marker asset registers");
    }
    resources[std::string(dev::kProjectFPSArmsResourceId)] =
        renderer.registerMeshResource(std::string(dev::kProjectFPSArmsResourceId), *animator.skinnedMesh(),
                                      render::MeshResourceUsage::DynamicVertices);
    resources[std::string(dev::kProjectFPSBodyResourceId)] =
        renderer.registerMeshResource(std::string(dev::kProjectFPSBodyResourceId), *animator.skinnedCompanion(),
                                      render::MeshResourceUsage::DynamicVertices);
    resources[std::string(dev::kProjectReadyWeaponAssetId)] =
        renderer.registerMeshResource(std::string(dev::kProjectReadyWeaponAssetId), weapon);
    const auto world = dev::createDevRangeGreyboxWorld();
    dev::DevRangeRenderSceneDesc desc;
    desc.greyboxWorld = &world;
    desc.meshCatalog = &catalog;
    desc.meshResources = &resources;
    desc.projectFPSPreview = true;
    desc.projectRifleReady = true;
    desc.useHandConstraints = true;
    desc.player.hasCameraRig = true;
    desc.player.hasAnimationFrame = true;
    desc.player.hasMovementState = true;
    desc.player.activeWeaponId = "ar_01";
    float largestResidual = 0;
    float smallestForwardDot = 1;
    float smallestRightMargin = 1000;
    std::size_t ticks = 0;
    std::array<std::size_t, 2> visibleHandVertices{};
    for (auto clip : {player::CharacterAnimationClip::Idle, player::CharacterAnimationClip::Walk,
                      player::CharacterAnimationClip::Sprint, player::CharacterAnimationClip::Idle}) {
        desc.player.animation.locomotionClip = clip;
        desc.player.animation.stride01 = .65F;
        for (int phase = 0; phase < 72; ++phase) {
            desc.player.animation.thirdPersonBodyPitchDegrees = clip == player::CharacterAnimationClip::Sprint ? 8 : 0;
            desc.player.animation.thirdPersonBodyRollDegrees = std::sin(static_cast<float>(ticks) * .1F) * 2;
            desc.player.position = {3, 0, -5};
            desc.player.view.yawDegrees = static_cast<float>(ticks) * 2;
            desc.player.cameraView = desc.player.view;
            expect(animator.samplePose(desc.player.animation, 1.F / 60), "one fixed-tick sample");
            expect(!animator.samplePose(desc.player.animation, 1.F / 60), "an unconsumed sample cannot advance twice");
            expect(!animator.skinnedMesh() && !animator.skinnedCompanion(),
                   "sample never exposes stale mesh as the new skin");
            animation::Mat4 eye;
            expect(animator.socketTransform("socket_fps_eye", eye), "eye is available before hand constraints");
            animation::Mat4 chestMount;
            expect(animator.socketTransform("socket_rifle_ready", chestMount),
                   "weapon mount follows the sampled chest ancestor");
            desc.player.rifleReadyMount = chestMount;
            const auto model = player::firstPersonAttachmentMatrix(dev::projectFPSPlayerAttachment(desc.player));
            desc.player.cameraPosition = animation::transformPoint(model, animation::matrixTranslation(eye));
            auto presentation = dev::evaluateFirstPersonPresentation(desc);
            if (!presentation.rightGripWorld || !presentation.leftGripWorld) {
                expect(false, "real M4 contact goals");
                return;
            }
            const player::PlayerHandTargets targets{model, *presentation.rightGripWorld, *presentation.leftGripWorld};
            const auto physicalFiring = std::ranges::find(weapon.nodeMarkers, std::string_view("socket_grip_r"),
                                                          &assets::GltfNodeMarker::name);
            const auto physicalSupport = std::ranges::find(weapon.nodeMarkers, std::string_view("socket_grip_l"),
                                                           &assets::GltfNodeMarker::name);
            expect(physicalFiring != weapon.nodeMarkers.end() && physicalSupport != weapon.nodeMarkers.end(),
                   "actual M4 positional pistol/handguard anchors remain available alongside palm frames");
            if (physicalFiring != weapon.nodeMarkers.end() && physicalSupport != weapon.nodeMarkers.end()) {
                const auto actualFiring = animation::transformPoint(presentation.weaponWorld, physicalFiring->worldPosition);
                const auto actualSupport = animation::transformPoint(presentation.weaponWorld, physicalSupport->worldPosition);
                expect((animation::matrixTranslation(targets.rightGripWorld) - actualFiring).lengthSquared() < .02F * .02F &&
                           (animation::matrixTranslation(targets.leftGripWorld) - actualSupport).lengthSquared() < .01F * .01F,
                       "real DEF-hand.R target is physical pistol grip and DEF-hand.L target is forward handguard, never exchanged");
            }
            // Semantic right-hand contract: camera-relative side, real firing
            // grip and original .R/.L anatomy. No fixed screenshot pixel oracle.
            const float yaw = desc.player.cameraView.yawDegrees * (3.14159265358979323846F / 180.F);
            const math::Vec3 cameraRight{-std::cos(yaw), 0, std::sin(yaw)};
            const auto firingOffset = animation::matrixTranslation(targets.rightGripWorld) - desc.player.cameraPosition;
            const auto dot = [](math::Vec3 a, math::Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; };
            const float rightMargin = dot(firingOffset, cameraRight);
            smallestRightMargin = std::min(smallestRightMargin, rightMargin);
            if (rightMargin <= 0) {
                const auto authoredWorld = animation::multiply(model, chestMount);
                const auto authoredX = animation::transformDirection(authoredWorld, {1, 0, 0});
                const auto eyeLocal = animation::matrixTranslation(eye);
                const auto mountLocal = animation::matrixTranslation(chestMount);
                std::cerr << "Ready hip side tick=" << ticks << " locomotion=" << static_cast<int>(clip)
                          << " camera_right_margin_m=" << rightMargin
                          << " authored_x_derivative=" << dot(authoredX, cameraRight)
                          << " eye_model=" << eyeLocal.x << ',' << eyeLocal.y << ',' << eyeLocal.z
                          << " mount_model=" << mountLocal.x << ',' << mountLocal.y << ',' << mountLocal.z << '\n';
            }
            expect(rightMargin > 0,
                   "hip firing grip stays right of anatomical camera across movement and yaw");
            const auto weaponForward = animation::transformPoint(presentation.weaponWorld, {0, 0, 1}) -
                                       animation::matrixTranslation(presentation.weaponWorld);
            const float forwardDot = dot(weaponForward, {std::sin(yaw), 0, std::cos(yaw)});
            smallestForwardDot = std::min(smallestForwardDot, forwardDot);
            expect(forwardDot > 0,
                   "right-handed weapon points ahead rather than toward the player");
            expect(presentation.rig.weapon.scale.x > 0 && presentation.rig.weapon.scale.y > 0 &&
                       presentation.rig.weapon.scale.z > 0,
                   "right-handed pose never uses negative mesh scale");
            expect(animator.skinPose(&targets), "common constrained pose skins both original meshes");
            const auto& result = animator.stats().handConstraints;
            if (result.status != player::HandConstraintStatus::Applied) {
                std::cerr << "Ready tick=" << ticks << " residual=" << result.residualMeters[0] << ','
                          << result.residualMeters[1] << '\n';
            }
            expect(result.status == player::HandConstraintStatus::Applied,
                   "both actual authored M4 surface targets are reachable");
            for (std::size_t side = 0; side < 2; ++side) {
                animation::Mat4 palm;
                expect(animator.socketTransform(side == 0 ? "socket_palm_r" : "socket_palm_l", palm),
                       "physical palm socket follows final skeleton");
                const auto goal =
                    animation::matrixTranslation(side == 0 ? targets.rightGripWorld : targets.leftGripWorld);
                const auto actual = animation::transformPoint(model, animation::matrixTranslation(palm));
                const float residual = std::sqrt((goal - actual).lengthSquared());
                largestResidual = std::max(largestResidual, residual);
                expect(residual <= .001F && result.orientationResidualDegrees[side] <= .5F,
                       "actual final palm/M4 contact is within 1 mm and 0.5 degrees");
                math::Vec3 skinCenter{};
                std::size_t handVertices = 0;
                for (std::size_t p = 0; p < arms.primitives.size(); ++p) {
                    const auto& source = arms.primitives[p];
                    if (source.skinIndex < 0) { continue; }
                    for (std::size_t v = 0; v < source.positions.size(); ++v) {
                        float handWeight = 0;
                        for (std::size_t slot = 0; slot < 4; ++slot) {
                            if (handOwnership[source.joints[v][slot]][side]) { handWeight += source.weights[v][slot]; }
                        }
                        if (handWeight > .75F) {
                            skinCenter = skinCenter + animator.skinnedMesh()->primitives[p].positions[v]; ++handVertices;
                        }
                    }
                }
                expect(handVertices > 0, "actual visible Project hand skin is weighted to its independently selected source bone");
                if (handVertices) {
                    skinCenter = animation::transformPoint(model, skinCenter * (1.F / static_cast<float>(handVertices)));
                    const auto other = animation::matrixTranslation(side == 0 ? targets.leftGripWorld : targets.rightGripWorld);
                    expect((skinCenter - goal).lengthSquared() < .10F * .10F &&
                               (skinCenter - goal).lengthSquared() < (skinCenter - other).lengthSquared(),
                           "visible real Project hand geometry follows firing/support ownership, not only named socket markers");
                }
            }
            animation::Mat4 afterEye;
            expect(animator.socketTransform("socket_fps_eye", afterEye) && eye.values == afterEye.values,
                   "hand IK never feeds back into the anatomical camera eye");
            animation::Mat4 afterMount;
            expect(animator.socketTransform("socket_rifle_ready", afterMount) && chestMount.values == afterMount.values,
                   "hand IK never feeds back into the sampled chest weapon mount");
            if (ticks == 0) {
                for (const auto& primitive : animator.skinnedMesh()->primitives) {
                    for (std::size_t v = 0; v < primitive.positions.size(); ++v) {
                        const auto p = primitive.positions[v] - animation::matrixTranslation(eye);
                        if (p.z <= .03F || std::abs(p.y) >= p.z * .7536F || std::abs(p.x) >= p.z * 1.3397F) {
                            continue;
                        }
                        for (std::size_t slot = 0; slot < 4; ++slot) {
                            const auto& name = arms.animationData->skins[0].joints[primitive.joints[v][slot]].name;
                            if (primitive.weights[v][slot] > .25F &&
                                (name.starts_with("DEF-hand.") || name.starts_with("DEF-f_") ||
                                 name.starts_with("DEF-thumb."))) {
                                ++visibleHandVertices[name.ends_with(".R") ? 0 : 1];
                                break;
                            }
                        }
                    }
                }
            }
            expect(renderer.updateMeshResourceVertices(resources.at(std::string(dev::kProjectFPSArmsResourceId)),
                                                       *animator.skinnedMesh()) &&
                       renderer.updateMeshResourceVertices(resources.at(std::string(dev::kProjectFPSBodyResourceId)),
                                                           *animator.skinnedCompanion()),
                   "both visible meshes accept exactly this common pose");
            desc.firstPersonPresentation = &presentation;
            for (int renderOnly = 0; renderOnly < 2; ++renderOnly) {
                render::RenderFrameInfo frame;
                const auto stats = dev::DevRangeRenderSceneBuilder{}.append(frame, desc);
                expect(stats.projectFPSBodyActive && stats.projectFPSArmsActive, "both actual subsets submitted");
                const auto rifle = std::ranges::find(frame.worldMeshes, dev::kProjectReadyWeaponAssetId,
                                                     &render::RenderMesh3D::assetId);
                expect(rifle != frame.worldMeshes.end() && rifle->layer == render::RenderMeshLayer::World,
                       "rifle shares the actual body/arms projection");
                if (rifle != frame.worldMeshes.end()) {
                    expect((rifle->position - animation::matrixTranslation(presentation.weaponWorld)).lengthSquared() <
                               1e-10F,
                           "rendered rifle uses the same final pose as both contact targets");
                    for (const auto name : {"socket_muzzle", "socket_palm_contact_r", "socket_palm_contact_l"}) {
                        const auto marker = std::ranges::find(weapon.nodeMarkers, std::string_view(name),
                                                              &assets::GltfNodeMarker::name);
                        const auto& target = std::string_view(name) == "socket_muzzle" ? presentation.muzzleWorld
                                             : std::string_view(name) == "socket_palm_contact_r"
                                                 ? presentation.rightGripWorld
                                                 : presentation.leftGripWorld;
                        expect(marker != weapon.nodeMarkers.end() && target &&
                                   (worldPoint(*rifle, marker->worldPosition) - animation::matrixTranslation(*target))
                                           .lengthSquared() < 1e-9F,
                               "rendered weapon rotation/scale reconstructs the exact cached muzzle and palm goals");
                    }
                }
                expect(std::ranges::none_of(frame.worldMeshes,
                                            [](const auto& mesh) { return mesh.assetId == "chr_a1_fp_arms_01"; }),
                       "armed Project preview has no A1 fallback");
            }
            ++ticks;
            expect(animator.stats().sampledFrameCount == ticks && animator.stats().skinnedFrameCount == ticks,
                   "render-only frames never advance/sample/skin the logical pose");
            expect(!animator.skinPose(&targets), "a sampled pose is consumed only once");
        }
    }
    std::cout << "Project Ready common-pose ticks=" << ticks << " max_world_contact_m=" << largestResidual
              << " min_weapon_camera_forward_dot=" << smallestForwardDot
              << " min_firing_camera_right_m=" << smallestRightMargin
              << " hip_hand_frustum_vertices=" << visibleHandVertices[0] << ',' << visibleHandVertices[1]
              << "; developer Ready over original locomotion, no directional/ADS/reload completeness claim\n";
    expect(visibleHandVertices[0] > 10 && visibleHandVertices[1] > 10,
           "both actual weighted hands intersect the anatomical hip camera frustum (not an occlusion/pixel oracle)");
    auto mismatch = body;
    auto changed = std::make_shared<assets::GltfAnimationData>(*body.animationData);
    changed->nodes[0].name += "_different_source";
    mismatch.animationData = changed;
    player::PlayerSkeletalAnimator rejected;
    expect(!rejected.initialize(arms, &mismatch), "unrelated companion hierarchy fails closed");
}
#endif

} // namespace

int main(int argc, char** argv) {
#if defined(NEMISIS_PROJECT_FPS_CANDIDATE_ROOT) && defined(NEMISIS_ENABLE_HAND_IK)
    if (argc == 2 && std::string_view(argv[1]) == "--ready-only") {
        testProjectReadyUsesOneConstrainedPose();
        return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
#else
    (void)argc;
    (void)argv;
#endif
#if defined(NEMISIS_PROJECT_FPS_CANDIDATE_ROOT)
    testActualProjectFPSPreviewSubmitsAnimatedSubsets();
#endif
    testDevRangeRenderSceneBuildsExpectedSubmissions();
    testDevRangeRenderSceneMovesWeaponTowardSightlineInAds();
    testDevRangeRenderSceneViewmodelFollowsCameraAngles();
    testDevRangeRenderSceneAlignsWeaponMeshToCookedMuzzleSocket();
    testCookedMuzzleAndGripsFollowRenderedWeaponTransform();
    testSharedPresentationUsesActualCookedM4AndOneSnapshot();
    testSharedPresentationPreservesAuthoredGripOrientations();
    testDevRangeRenderSceneKeepsCookedWeaponAxes();
    testDevRangeRenderScenePlacesShowcaseAssetsInSpawnView();
    testDevRangeRenderSceneHidesLocalWorldBodyWhenCameraRigIsActive();
    testDevRangeRenderSceneRigsFirstPersonBodyForLookDown();
    testFirstPersonArmsHaveOnePresentationOwner();
    testDevRangeRenderSceneDrawsSocketDrivenShotFeedback();
    testDevRangeRenderSceneDrawsLanePressure();
    testDevRangeRenderSceneAcceptsSpeedClearTargetLayout();
    testDevRangeRenderSceneCountsMissingMeshHandles();
    testDevRangeRenderSceneKeepsSkyboxMeshFallbackWhenSkyPassDisabled();
    testDevRangeRenderSceneHandlesMissingInputs();
    testDevRangeRenderSceneCanDisableDebugLines();
    testDevRangeRenderSceneDrawsMantleCandidateDebugLines();
    testDevRangeRenderSceneDrawsSweepDebugLines();
    testDevRangeRenderSceneDrawsContactDebugLines();

    if (failures > 0) {
        std::cerr << failures << " dev range render scene test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Dev range render scene tests passed\n";
    return EXIT_SUCCESS;
}

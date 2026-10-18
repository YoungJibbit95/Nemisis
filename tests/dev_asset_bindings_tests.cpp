#include "nemisis/assets/DevAssetBindings.hpp"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

struct AssetSpec final {
    std::string id;
    std::string kind;
    std::string category;
    std::string source;
    std::string cooked;
    std::string collision = "none";
};

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

std::vector<AssetSpec> requiredSpecs() {
    return {
        {"prop_target_dummy_01", "mesh", "prop", "assets/source/blender/props/prop_target_dummy_01.blend", "assets/export/gltf/props/prop_target_dummy_01.glb", "col_prop_target_dummy_01"},
        {"wpn_ar_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_ar_01.blend", "assets/export/gltf/weapons/wpn_ar_01.glb"},
        {"wpn_smg_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_smg_01.blend", "assets/export/gltf/weapons/wpn_smg_01.glb"},
        {"wpn_sidearm_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_sidearm_01.blend", "assets/export/gltf/weapons/wpn_sidearm_01.glb"},
        {"env_test_arena_kit_01", "scene", "environment", "assets/source/blender/environments/env_test_arena_kit_01.blend", "assets/export/gltf/environments/env_test_arena_kit_01.glb", "per_piece_col_prefix"},
        {"chr_player_capsule_proxy_01", "mesh", "character", "assets/source/blender/characters/chr_player_capsule_proxy_01.blend", "assets/export/gltf/characters/chr_player_capsule_proxy_01.glb", "col_chr_player_capsule_proxy_01"},
        {"chr_dev_arms_a", "mesh", "character", "assets/source/blender/characters/chr_dev_arms_a.blend", "assets/export/gltf/characters/chr_dev_arms_a.glb"},
        {"chr_dev_soldier_a", "mesh", "character", "assets/source/blender/characters/chr_dev_soldier_a.blend", "assets/export/gltf/characters/chr_dev_soldier_a.glb", "col_chr_dev_soldier_a_capsule"},
        {"wpn_proto_smg_01", "mesh", "weapon", "tools/blender/make_prototype_pack.py", "assets/generated/prototype_pack/wpn_proto_smg_01.glb"},
        {"chr_proto_humanoid_01", "mesh", "character", "tools/blender/make_prototype_pack.py", "assets/generated/prototype_pack/chr_proto_humanoid_01.glb", "col_chr_proto_humanoid_01_capsule"},
        {"map_wall_panel_01", "mesh", "environment", "tools/blender/make_prototype_pack.py", "assets/generated/prototype_pack/map_wall_panel_01.glb", "box"},
        {"map_floor_tile_01", "mesh", "environment", "tools/blender/make_prototype_pack.py", "assets/generated/prototype_pack/map_floor_tile_01.glb", "walkable_box"},
        {"map_cover_crate_01", "mesh", "environment", "tools/blender/make_prototype_pack.py", "assets/generated/prototype_pack/map_cover_crate_01.glb", "box"},
        {"map_ramp_01", "mesh", "environment", "tools/blender/make_prototype_pack.py", "assets/generated/prototype_pack/map_ramp_01.glb", "ramp_wedge"},
        {"map_target_stand_01", "mesh", "prop", "tools/blender/make_prototype_pack.py", "assets/generated/prototype_pack/map_target_stand_01.glb", "box"},
        {"wpn_a1_compact_rifle_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_a1_compact_rifle_01.blend", "assets/export/gltf/weapons/wpn_a1_compact_rifle_01.glb"},
        {"wpn_a1_modern_rifle_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_a1_modern_rifle_01.blend", "assets/export/gltf/weapons/wpn_a1_modern_rifle_01.glb"},
        {"wpn_a1_compact_sidearm_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_a1_compact_sidearm_01.blend", "assets/export/gltf/weapons/wpn_a1_compact_sidearm_01.glb"},
        {"chr_a1_stylized_operator_01", "mesh", "character", "assets/source/blender/characters/chr_a1_stylized_operator_01.blend", "assets/export/gltf/characters/chr_a1_stylized_operator_01.glb", "col_chr_a1_stylized_operator_01_capsule"},
        {"chr_a1_fp_arms_01", "mesh", "character", "assets/source/blender/characters/chr_a1_fp_arms_01.blend", "assets/export/gltf/characters/chr_a1_fp_arms_01.glb"},
        {"wpn_a2_blackout_carbine_01", "mesh", "weapon", "assets/source/blender/a2_visual_pack/wpn_a2_blackout_carbine_01.blend", "assets/generated/a2_visual_pack/wpn_a2_blackout_carbine_01.glb"},
        {"wpn_a2_modular_rifle_01", "mesh", "weapon", "assets/source/blender/a2_visual_pack/wpn_a2_modular_rifle_01.blend", "assets/generated/a2_visual_pack/wpn_a2_modular_rifle_01.glb"},
        {"wpn_a2_striker_sidearm_01", "mesh", "weapon", "assets/source/blender/a2_visual_pack/wpn_a2_striker_sidearm_01.blend", "assets/generated/a2_visual_pack/wpn_a2_striker_sidearm_01.glb"},
        {"chr_a2_pilot_operator_01", "mesh", "character", "assets/source/blender/a2_visual_pack/chr_a2_pilot_operator_01.blend", "assets/generated/a2_visual_pack/chr_a2_pilot_operator_01.glb", "col_chr_a2_pilot_operator_01_capsule"},
        {"map_a2_wallrun_panel_01", "mesh", "environment", "assets/source/blender/a2_visual_pack/map_a2_wallrun_panel_01.blend", "assets/generated/a2_visual_pack/map_a2_wallrun_panel_01.glb", "visual_only_use_simple_wall_collision"},
        {"map_a2_slide_ramp_01", "mesh", "environment", "assets/source/blender/a2_visual_pack/map_a2_slide_ramp_01.blend", "assets/generated/a2_visual_pack/map_a2_slide_ramp_01.glb", "visual_only_use_simple_ramp_collision"},
        {"map_a2_cover_crate_01", "mesh", "environment", "assets/source/blender/a2_visual_pack/map_a2_cover_crate_01.blend", "assets/generated/a2_visual_pack/map_a2_cover_crate_01.glb", "visual_only_use_box_collision"},
        {"prop_a2_range_hero_01", "mesh", "prop", "assets/source/blender/a2_visual_pack/prop_a2_range_hero_01.blend", "assets/generated/a2_visual_pack/prop_a2_range_hero_01.glb", "visual_only_use_simple_cylinder_or_box_collision"},
        {"chr_project_male1", "mesh", "character", "assets/project_assets/character_male1.glb", "assets/processed/normalized/project_assets/chr_project_male1.glb"},
        {"wpn_project_rifle_m4a1", "mesh", "weapon", "assets/project_assets/weapon_rifle_m4a1.glb", "assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb"},
        {"wpn_project_rifle_afr120", "mesh", "weapon", "assets/project_assets/weapon_rifle_afr120.glb", "assets/processed/normalized/project_assets/wpn_project_rifle_afr120.glb"},
        {"wpn_project_rifle_ncar", "mesh", "weapon", "assets/project_assets/weapon_rifle_ncar.glb", "assets/processed/normalized/project_assets/wpn_project_rifle_ncar.glb"},
        {"wpn_project_smg_fr17", "mesh", "weapon", "assets/project_assets/weapon_smg_fr17.glb", "assets/processed/normalized/project_assets/wpn_project_smg_fr17.glb"},
        {"wpn_project_sidearm_glock19", "mesh", "weapon", "assets/project_assets/weapon_sidearm_glock19.glb", "assets/processed/normalized/project_assets/wpn_project_sidearm_glock19.glb"},
        {"wpn_project_sidearm_p320", "mesh", "weapon", "assets/project_assets/weapon_sidearm_p320.glb", "assets/processed/normalized/project_assets/wpn_project_sidearm_p320.glb"},
        {"env_project_skybox1", "mesh", "environment", "assets/project_assets/skybox1.glb", "assets/project_assets/skybox1.glb"},
    };
}

void writeManifest(const std::filesystem::path& path, std::span<const AssetSpec> specs) {
    std::ofstream file(path);
    file << R"({
        "manifest": { "name": "dev_asset_bindings_test", "root": "assets" },
        "assets": [
)";
    for (std::size_t index = 0; index < specs.size(); ++index) {
        const auto& spec = specs[index];
        file << R"({
                "id": ")" << spec.id << R"(",
                "kind": ")" << spec.kind << R"(",
                "source": ")" << spec.source << R"(",
                "cooked": ")" << spec.cooked << R"(",
                "streamable": true,
                "priority": 90,
                "estimated_bytes": 1024,
                "tags": ["dev_sandbox"]
            })";
        if (index + 1 < specs.size()) {
            file << ',';
        }
        file << '\n';
    }
    file << R"(]
    })";
}

void writeMetadata(const std::filesystem::path& root, const AssetSpec& spec) {
    auto metadataPath = root / spec.cooked;
    metadataPath.replace_extension(".metadata.json");
    std::filesystem::create_directories(metadataPath.parent_path());

    std::ofstream file(metadataPath);
    file << R"({
        "id": ")" << spec.id << R"(",
        "source": ")" << spec.source << R"(",
        "export": ")" << spec.cooked << R"(",
        "category": ")" << spec.category << R"(",
        "scale_meters": true,
        "runtime_up_axis": "Y",
        "gameplay_forward_axis": "+Z",
        "sockets": ["socket_root"],
        "collision": ")" << spec.collision << R"(",
        "lods": [")" << spec.id << R"(_lod0"],
        "license": "original_project_asset",
        "generated_by": "dev_asset_bindings_tests"
    })";
}

void appendU32(std::string& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<char>(value & 0xFFU));
    bytes.push_back(static_cast<char>((value >> 8U) & 0xFFU));
    bytes.push_back(static_cast<char>((value >> 16U) & 0xFFU));
    bytes.push_back(static_cast<char>((value >> 24U) & 0xFFU));
}

void appendU16(std::string& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<char>(value & 0xFFU));
    bytes.push_back(static_cast<char>((value >> 8U) & 0xFFU));
}

void appendF32(std::string& bytes, float value) {
    char raw[sizeof(float)]{};
    std::memcpy(raw, &value, sizeof(float));
    bytes.append(raw, sizeof(float));
}

void appendPaddedChunk(std::string& bytes, std::string chunk, std::uint32_t chunkType, char padding) {
    while ((chunk.size() % 4U) != 0U) {
        chunk.push_back(padding);
    }
    appendU32(bytes, static_cast<std::uint32_t>(chunk.size()));
    appendU32(bytes, chunkType);
    bytes += chunk;
}

void writeTinyGlb(const std::filesystem::path& path, std::string_view assetId) {
    constexpr std::uint32_t kGlbMagic = 0x46546C67U;
    constexpr std::uint32_t kGlbJsonChunk = 0x4E4F534AU;
    constexpr std::uint32_t kGlbBinaryChunk = 0x004E4942U;

    std::filesystem::create_directories(path.parent_path());
    const std::string json = std::string(R"({
        "asset": { "version": "2.0", "generator": "dev_asset_bindings_tests" },
        "scene": 0,
        "scenes": [{ "nodes": [0] }],
        "nodes": [{ "name": ")") + std::string(assetId) + R"(_node", "mesh": 0 }],
        "meshes": [{ "name": ")" + std::string(assetId) + R"(_mesh", "primitives": [{ "attributes": { "POSITION": 0 }, "indices": 1, "material": 0 }] }],
        "materials": [{ "name": ")" + std::string(assetId) + R"(_mat" }],
        "buffers": [{ "byteLength": 42 }],
        "bufferViews": [
            { "buffer": 0, "byteOffset": 0, "byteLength": 36 },
            { "buffer": 0, "byteOffset": 36, "byteLength": 6 }
        ],
        "accessors": [
            { "bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3" },
            { "bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR" }
        ]
    })";
    std::string bin;
    appendF32(bin, 0.0F);
    appendF32(bin, 0.0F);
    appendF32(bin, 0.0F);
    appendF32(bin, 1.0F);
    appendF32(bin, 0.0F);
    appendF32(bin, 0.0F);
    appendF32(bin, 0.0F);
    appendF32(bin, 1.0F);
    appendF32(bin, 0.0F);
    appendU16(bin, 0);
    appendU16(bin, 1);
    appendU16(bin, 2);

    std::string chunks;
    appendPaddedChunk(chunks, json, kGlbJsonChunk, ' ');
    appendPaddedChunk(chunks, bin, kGlbBinaryChunk, '\0');

    std::string glb;
    appendU32(glb, kGlbMagic);
    appendU32(glb, 2);
    appendU32(glb, static_cast<std::uint32_t>(12U + chunks.size()));
    glb += chunks;

    std::ofstream file(path, std::ios::binary);
    file.write(glb.data(), static_cast<std::streamsize>(glb.size()));
}

void testRequiredDevAssetsBindToMeshCatalog() {
    const auto root = std::filesystem::temp_directory_path() / "nemisis_dev_asset_bindings_ready";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto specs = requiredSpecs();
    const auto manifestPath = root / "assets.json";
    writeManifest(manifestPath, specs);
    for (const auto& spec : specs) {
        writeMetadata(root, spec);
        writeTinyGlb(root / spec.cooked, spec.id);
    }

    nemisis::assets::GameAssetCatalog catalog;
    const auto loadResult = catalog.loadFromJson(manifestPath);
    expect(loadResult.ok(), "asset catalog loads required dev assets");

    nemisis::assets::DevAssetBindings bindings;
    const auto summary = bindings.bindRequiredAssets(catalog, root);
    expect(summary.ready(), "required dev assets bind cleanly");
    expect(summary.requiredAssetCount == nemisis::assets::requiredDevSandboxRenderableAssetIds().size(), "summary counts required assets");
    expect(summary.renderableAssetCount == summary.requiredAssetCount, "all required assets become mesh handles");
    expect(summary.metadataAssetCount == summary.requiredAssetCount, "all required assets load metadata");
    expect(summary.importedAssetCount == summary.requiredAssetCount, "all required assets load glb scene info");
    expect(summary.extractedAssetCount == summary.requiredAssetCount, "all required assets extract glb mesh data");
    expect(summary.totalMeshCount == summary.requiredAssetCount, "summary counts imported glb meshes");
    expect(summary.totalNodeCount == summary.requiredAssetCount, "summary counts imported glb nodes");
    expect(summary.totalMaterialCount == summary.requiredAssetCount, "summary counts imported glb materials");
    expect(summary.totalPrimitiveCount == summary.requiredAssetCount, "summary counts imported glb primitives");
    expect(summary.totalVertexCount == summary.requiredAssetCount * 3U, "summary counts imported glb vertices");
    expect(summary.totalIndexCount == summary.requiredAssetCount * 3U, "summary counts imported glb indices");
    expect(summary.totalBinaryBytes == summary.requiredAssetCount * 44U, "summary totals glb binary payload bytes");
    expect(bindings.meshCatalog().contains("wpn_ar_01"), "mesh catalog contains AR handle");
    expect(bindings.meshCatalog().contains("env_test_arena_kit_01"), "mesh catalog contains arena scene handle");
    const auto* ar = bindings.meshCatalog().findByAssetId("wpn_ar_01");
    expect(ar != nullptr && ar->sceneInfo.has_value(), "mesh catalog keeps imported glb scene info");
    expect(ar != nullptr && ar->meshData.has_value(), "mesh catalog keeps imported glb mesh data");

    std::filesystem::remove_all(root);
}

void testMissingMetadataIsReported() {
    const auto root = std::filesystem::temp_directory_path() / "nemisis_dev_asset_bindings_missing";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const std::array<AssetSpec, 1> specs{
        AssetSpec{"wpn_ar_01", "mesh", "weapon", "assets/source/blender/weapons/wpn_ar_01.blend", "assets/export/gltf/weapons/wpn_ar_01.glb"},
    };
    const auto manifestPath = root / "assets.json";
    writeManifest(manifestPath, specs);

    nemisis::assets::GameAssetCatalog catalog;
    const auto loadResult = catalog.loadFromJson(manifestPath);
    expect(loadResult.ok(), "asset catalog loads incomplete dev asset fixture");

    const std::array<std::string_view, 1> ids{"wpn_ar_01"};
    nemisis::assets::DevAssetBindings bindings;
    const auto summary = bindings.bindAssets(catalog, ids, root);
    expect(!summary.ready(), "missing metadata prevents ready state");
    expect(summary.renderableAssetCount == 0, "missing metadata is not registered");
    expect(!summary.errors.empty(), "missing metadata reports error");

    std::filesystem::remove_all(root);
}

void testRepresentativeCookedWeaponContract() {
    const std::filesystem::path sourceRoot{NEMISIS_SOURCE_ROOT};
    nemisis::assets::GameAssetCatalog catalog;
    const auto catalogLoad = catalog.loadFromJson(sourceRoot / "configs/assets/nemisis_assets.json");
    expect(catalogLoad.ok(), "real asset catalog loads for canonical weapon contract");
    const auto* record = catalog.find("wpn_project_rifle_m4a1");
    expect(record != nullptr, "representative weapon is selected through the actual catalog");
    if (record == nullptr) {
        return;
    }
    novacore::assets::GltfAssetMetadata metadata{};
    const auto metadataLoad = novacore::assets::loadGltfAssetMetadataFromJson(
        sourceRoot / novacore::assets::metadataPathForCookedAsset(*record), metadata);
    expect(metadataLoad.ok(), "representative weapon metadata loads");
    expect(novacore::assets::validateGltfAssetMetadata(*record, metadata).empty(), "representative metadata agrees with catalog");
    expect(metadata.scaleMeters && metadata.runtimeUpAxis == "Y" && metadata.gameplayForwardAxis == "+Z", "cooked weapon declares metre Y-up/+Z-forward convention");
    novacore::assets::GltfMeshData mesh{};
    const auto meshLoad = novacore::assets::loadGltfMeshData(sourceRoot / record->cookedPath, mesh);
    expect(meshLoad.ok() && mesh.vertexCount() > 0U, "representative cooked GLB contains valid geometry");
    if (!meshLoad.ok() || mesh.vertexCount() == 0U) {
        return;
    }
    const auto marker = [&](std::string_view name) {
        return std::ranges::find_if(mesh.nodeMarkers, [name](const auto& value) { return value.name == name; });
    };
    for (const auto name : {"socket_muzzle", "socket_grip_r", "socket_grip_l"}) {
        expect(std::ranges::count(metadata.sockets, name) == 1, "required weapon socket is declared once in metadata");
        expect(std::ranges::count_if(mesh.nodeMarkers, [name](const auto& value) { return value.name == name; }) == 1,
            "required weapon socket name is unique in the cooked scene");
        const auto socket = marker(name);
        expect(socket != mesh.nodeMarkers.end(), "required weapon socket exists in actual GLB hierarchy");
        if (socket != mesh.nodeMarkers.end()) {
            expect(socket->worldForward.z > 0.999F && socket->worldUp.y > 0.999F, "canonical weapon socket frame faces +Z with +Y up");
            expect(std::isfinite(socket->worldPosition.x) && std::isfinite(socket->worldPosition.y) && std::isfinite(socket->worldPosition.z), "weapon socket position is finite");
        }
    }
    const auto muzzle = marker("socket_muzzle");
    const auto grip = marker("socket_grip_r");
    if (muzzle != mesh.nodeMarkers.end() && grip != mesh.nodeMarkers.end()) {
        expect(grip->worldPosition.lengthSquared() < 0.000001F, "cooked weapon origin is firing grip");
        expect(muzzle->worldPosition.z > grip->worldPosition.z + 0.1F, "muzzle is ahead of firing grip in canonical local space");
    }
    auto minimum = novacore::math::Vec3{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    auto maximum = minimum * -1.0F;
    for (const auto& primitive : mesh.primitives) {
        expect(primitive.nodeTransformBaked && primitive.skinIndex < 0, "representative rigid weapon has static baked geometry");
        for (const auto position : primitive.positions) {
            expect(std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z), "cooked weapon vertex is finite");
            minimum = {std::min(minimum.x, position.x), std::min(minimum.y, position.y), std::min(minimum.z, position.z)};
            maximum = {std::max(maximum.x, position.x), std::max(maximum.y, position.y), std::max(maximum.z, position.z)};
        }
    }
    const auto dimensions = maximum - minimum;
    expect(dimensions.z > dimensions.x && dimensions.z > dimensions.y, "rifle's long axis is runtime forward");
    expect(dimensions.z > 0.5F && dimensions.z < 1.5F, "rifle is metre-scale, not centimetre or authoring-unit scale");
}

void testFirstPersonArmsExcludeAuthoringHelpers() {
    const std::filesystem::path sourceRoot{NEMISIS_SOURCE_ROOT};
    nemisis::assets::GameAssetCatalog catalog;
    expect(catalog.loadFromJson(sourceRoot / "configs/assets/nemisis_assets.json").ok(), "actual catalog loads for arms presentation selection");
    nemisis::assets::DevAssetBindings bindings;
    const std::array<std::string_view, 1> ids{"chr_a1_fp_arms_01"};
    expect(bindings.bindAssets(catalog, ids, sourceRoot).ready(), "actual FP arms bind after presentation selection");
    const auto* bound = bindings.meshCatalog().findByAssetId(ids.front());
    if (bound == nullptr || !bound->meshData.has_value()) {
        expect(false, "selected arms mesh remains available");
        return;
    }
    novacore::assets::GltfMeshData original;
    expect(novacore::assets::loadGltfMeshData(bound->meshData->path, original).ok(), "unchanged source GLB remains readable");
    const auto weightedVertices = [](const auto& mesh) {
        std::size_t count = 0;
        for (const auto& primitive : mesh.primitives) {
            if (primitive.skinIndex >= 0) {
                count += primitive.positions.size();
            }
        }
        return count;
    };
    expect(weightedVertices(original) > 0 && weightedVertices(original) == weightedVertices(*bound->meshData),
        "all weighted arm/hand geometry survives selection");
    for (const auto& primitive : bound->meshData->primitives) {
        const auto node = std::ranges::find_if(bound->meshData->nodes, [&](const auto& value) {
            return primitive.nodeIndex >= 0 && value.nodeIndex == static_cast<std::size_t>(primitive.nodeIndex);
        });
        expect(node != bound->meshData->nodes.end() && node->name != "lod0_weapon_alignment_bar" && node->name != "lod1_blockout",
            "runtime arm primitives exclude the authoring guide and simultaneous blockout LOD");
    }
    expect(bound->meshData->nodeMarkers.size() == original.nodeMarkers.size(), "selection preserves authored hierarchy and socket markers");
}

} // namespace

int main() {
    testRequiredDevAssetsBindToMeshCatalog();
    testMissingMetadataIsReported();
    testRepresentativeCookedWeaponContract();
    testFirstPersonArmsExcludeAuthoringHelpers();

    if (failures > 0) {
        std::cerr << failures << " dev asset binding test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis dev asset binding tests passed\n";
    return EXIT_SUCCESS;
}

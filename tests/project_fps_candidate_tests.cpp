#include "novacore/assets/GltfAnimationBridge.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace novacore;

void require(bool value, const std::string& message) {
    if (!value) { throw std::runtime_error(message); }
}

std::uint32_t readU32(std::istream& stream) {
    unsigned char bytes[4]{};
    require(static_cast<bool>(stream.read(reinterpret_cast<char*>(bytes), 4)), "truncated candidate oracle map");
    return bytes[0] | (std::uint32_t(bytes[1]) << 8U) | (std::uint32_t(bytes[2]) << 16U) |
           (std::uint32_t(bytes[3]) << 24U);
}

struct PrimitiveMap {
    std::size_t source;
    std::vector<std::uint32_t> vertices;
    std::vector<std::uint32_t> triangles;
};

struct Asset {
    assets::GltfMeshData mesh;
    assets::GltfAnimationAsset animation;
};

Asset load(const std::filesystem::path& path) {
    Asset result;
    require(assets::loadGltfMeshData(path, result.mesh).ok() && result.mesh.animationData != nullptr,
            "import failed: " + path.string());
    require(assets::buildGltfAnimationAsset(*result.mesh.animationData, 0, result.animation).ok(),
            "animation bridge failed: " + path.string());
    return result;
}

bool near(const animation::Mat4& a, const animation::Mat4& b) {
    for (std::size_t i = 0; i < 16; ++i) {
        if (!std::isfinite(a.values[i]) || std::abs(a.values[i] - b.values[i]) > 0.00001F) { return false; }
    }
    return true;
}

animation::GlobalPose pose(const Asset& asset, int clip, float normalizedTime) {
    auto local = animation::makeBindPose(asset.animation.skeleton);
    if (clip >= 0) {
        const auto& c = asset.animation.clips.at(static_cast<std::size_t>(clip));
        require(animation::sampleClip(c, asset.animation.skeleton, c.duration * normalizedTime,
                                     animation::WrapMode::Clamp, local), "sample failed");
    }
    animation::GlobalPose global;
    require(animation::evaluateGlobalPose(asset.animation.skeleton, local, global), "globals failed");
    return global;
}

void check(const Asset& original, const Asset& candidate, const std::string& name) {
    std::ifstream stream(std::filesystem::path(NEMISIS_PROJECT_FPS_CANDIDATE_ROOT) / (name + ".map"), std::ios::binary);
    const auto count = readU32(stream);
    require(count == candidate.mesh.primitives.size(), "map primitive count");
    std::vector<PrimitiveMap> maps;
    for (std::size_t p = 0; p < count; ++p) {
        PrimitiveMap map{readU32(stream), {}, {}};
        const auto vertices = readU32(stream), triangles = readU32(stream);
        require(map.source < original.mesh.primitives.size() && vertices <= original.mesh.vertexCount() &&
                triangles <= original.mesh.indexCount() / 3U, "map bounds");
        for (std::uint32_t i = 0; i < vertices; ++i) { map.vertices.push_back(readU32(stream)); }
        for (std::uint32_t i = 0; i < triangles; ++i) { map.triangles.push_back(readU32(stream)); }
        const auto& a = original.mesh.primitives.at(map.source);
        const auto& b = candidate.mesh.primitives.at(p);
        require(a.materialIndex == b.materialIndex && a.skinIndex == b.skinIndex, "material/skin contract");
        require(b.positions.size() == vertices && b.indices.size() == 3U * triangles, "compacted topology");
        for (std::size_t v = 0; v < vertices; ++v) {
            const auto src = map.vertices[v];
            require(src < a.positions.size(), "source vertex bounds");
            require((a.positions[src] - b.positions[v]).lengthSquared() == 0 &&
                    (a.normals[src] - b.normals[v]).lengthSquared() == 0 &&
                    a.texcoords[src].x == b.texcoords[v].x && a.texcoords[src].y == b.texcoords[v].y &&
                    a.weights[src] == b.weights[v], "source positions/normals/UVs/weights preserved");
            for (std::size_t k = 0; k < 4; ++k) {
                if (b.weights[v][k] <= 0) { continue; }
                const auto sourceSlot = a.joints[src][k], slot = b.joints[v][k];
                require(original.animation.jointNodeIndices.at(sourceSlot) == candidate.animation.jointNodeIndices.at(slot),
                        "JOINTS remap and original node identity");
                require(near(original.animation.inverseBindMatrices.at(sourceSlot), candidate.animation.inverseBindMatrices.at(slot)),
                        "inverse bind row retained with remapped palette");
            }
        }
        for (std::size_t t = 0; t < triangles; ++t) {
            for (std::size_t k = 0; k < 3; ++k) {
                require(map.vertices.at(b.indices.at(t * 3 + k)) == a.indices.at(map.triangles[t] * 3 + k),
                        "explicit source triangle and winding preserved");
            }
        }
        maps.push_back(std::move(map));
    }
    require(stream.peek() == std::char_traits<char>::eof(), "map trailing data");
    require(candidate.animation.clips.size() == original.animation.clips.size(), "all clip names retained");
    require(candidate.animation.skeleton.sockets.size() == 7U, "all seven authored sockets retained");
    const bool arms = name == "arms";
    require(candidate.mesh.vertexCount() == (arms ? 9536U : 40532U) &&
            candidate.mesh.indexCount() == (arms ? 15942U : 53682U) * 3U &&
            candidate.animation.skeleton.joints.size() == (arms ? 84U : 60U), "audited geometry and pose reduction");
    float maxDrift = 0, maxDeformation = 0;
    for (int clip = -1; clip < static_cast<int>(original.animation.clips.size()); ++clip) {
        if (clip >= 0) {
            const auto i = static_cast<std::size_t>(clip);
            require(original.animation.clips[i].name == candidate.animation.clips[i].name &&
                    original.animation.clips[i].duration == candidate.animation.clips[i].duration, "clip name/duration parity");
        }
        for (const float time : {0.0F, 0.37F, 0.83F}) {
            const auto a = pose(original, clip, time), b = pose(candidate, clip, time);
            for (std::size_t j = 0; j < b.joints.size(); ++j) {
                const auto node = candidate.animation.jointNodeIndices[j];
                const auto found = std::ranges::find(original.animation.jointNodeIndices, node);
                require(found != original.animation.jointNodeIndices.end(), "retained ancestor exists in source runtime");
                const auto index = static_cast<std::size_t>(found - original.animation.jointNodeIndices.begin());
                require(near(a.joints.at(index), b.joints[j]), "retained joint/ancestor animated global parity");
            }
            for (const auto& socket : candidate.animation.skeleton.sockets) {
                animation::Mat4 sa, sb;
                require(animation::evaluateSocket(original.animation.skeleton, a, socket.name, sa) &&
                        animation::evaluateSocket(candidate.animation.skeleton, b, socket.name, sb) && near(sa, sb),
                        "authored socket frame parity including offsets/scales");
            }
            assets::GltfMeshData sa, sb;
            require(assets::skinGltfMesh(original.mesh, original.animation, a, sa).ok() &&
                    assets::skinGltfMesh(candidate.mesh, candidate.animation, b, sb).ok(), "actual engine skinning");
            for (std::size_t p = 0; p < maps.size(); ++p) {
                const auto& map = maps[p];
                for (std::size_t v = 0; v < map.vertices.size(); ++v) {
                    const auto delta = sa.primitives[map.source].positions[map.vertices[v]] - sb.primitives[p].positions[v];
                    maxDrift = std::max(maxDrift, delta.lengthSquared());
                    require(delta.lengthSquared() < 0.0000000001F, "skinned subset matches source within 10 micrometres");
                    const auto movement = sb.primitives[p].positions[v] - candidate.mesh.primitives[p].positions[v];
                    maxDeformation = std::max(maxDeformation, movement.lengthSquared());
                }
            }
        }
    }
    require(maxDeformation > 0.0001F, "authored clips actually deform selected weighted vertices");
    std::cout << name << " vertices=" << candidate.mesh.vertexCount() << " pose_nodes="
              << candidate.animation.skeleton.joints.size() << " max_subset_drift_m=" << std::sqrt(maxDrift)
              << " max_authored_deformation_m=" << std::sqrt(maxDeformation) << '\n';
}

// Reproducible CPU-only Release comparison: fixed Walk clip, 20 warmup samples,
// 120 measured samples, existing public sampler/global evaluator/skinner. Import,
// GPU allocation/transfer/draw and filesystem work are outside timing. Allocations
// and validation performed internally by these public CPU APIs remain included.
void benchmark(const Asset& asset, const std::string& name) {
    const auto& clip = asset.animation.clips.back();
    animation::LocalPose local;
    animation::GlobalPose global;
    assets::GltfMeshData skinned;
    std::vector<double> times;
    for (int i = -20; i < 120; ++i) {
        const auto started = std::chrono::steady_clock::now();
        require(animation::sampleClip(clip, asset.animation.skeleton, clip.duration * (float(i + 20) / 139.0F),
                                      animation::WrapMode::Clamp, local) &&
                animation::evaluateGlobalPose(asset.animation.skeleton, local, global) &&
                assets::skinGltfMesh(asset.mesh, asset.animation, global, skinned).ok(), "benchmark pose/skin");
        if (i >= 0) { times.push_back(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count()); }
    }
    std::ranges::sort(times);
    std::cout << "CPU_RELEASE " << name << " clip=" << clip.name << " samples=120 warmup=20 median_ms="
              << (times[59] + times[60]) / 2.0 << " p95_ms=" << times[113]
              << " position_normal_payload_bytes=" << asset.mesh.vertexCount() * 6U * sizeof(float)
              << " (payload only; no GPU transfer measured)\n";
}
} // namespace

int main(int argc, char** argv) {
    try {
        const auto original = load(std::filesystem::path(NEMISIS_SOURCE_ROOT) /
                                   "assets/processed/normalized/project_assets/chr_project_male1.glb");
        for (const std::string name : {"arms", "body"}) {
            const auto candidate = load(std::filesystem::path(NEMISIS_PROJECT_FPS_CANDIDATE_ROOT) / (name + ".glb"));
            check(original, candidate, name);
            if (argc == 2 && std::string(argv[1]) == "--benchmark") { benchmark(candidate, name); }
        }
        if (argc == 2 && std::string(argv[1]) == "--benchmark") { benchmark(original, "world"); }
        std::cout << "Project FPS candidate contracts passed; visual acceptance remains open\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

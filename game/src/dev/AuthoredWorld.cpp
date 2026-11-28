#include "nemisis/dev/AuthoredWorld.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"
#include "novacore/world/WorldJson.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <utility>

namespace nemisis::dev {
namespace {
using novacore::math::Vec3;
using namespace novacore::world;
bool spawnInsideSolid(Vec3 p, const GreyboxPrimitive& box) {
    constexpr float epsilon = 0.001F;
    return box.blocksMovement &&
        std::abs(p.x - box.center.x) < box.halfExtents.x + kPlayerCapsuleRadius &&
        std::abs(p.z - box.center.z) < box.halfExtents.z + kPlayerCapsuleRadius &&
        p.y + epsilon < box.center.y + box.halfExtents.y &&
        p.y + kPlayerStandingHeight - epsilon > box.center.y - box.halfExtents.y;
}
}
std::optional<std::string> buildAuthoredWorld(
    const novacore::world::WorldDocument& doc,
    const assets::GameAssetCatalog& catalog, GreyboxWorld& out) {
    if (auto error = validateWorldDocument(doc)) return error;
    if (doc.objects.size() > kAuthoredPlaytestObjectLimit) return "Playtest object limit exceeded (4096)";
    GreyboxWorld candidate;
    candidate.id = doc.id;
    candidate.authored = true;
    candidate.boundsHalfExtents = {2, 2, 2};
    for (const auto& object : doc.objects) {
        if (const auto* box = std::get_if<WorldBox>(&object.data)) {
            // Existing Cover surface provides a solid box with walkable top.
            candidate.primitives.push_back({object.id, GreyboxPrimitiveKind::Cover,
                object.position, box->halfExtents, {0.56F,0.62F,0.68F,1}, box->blocksMovement});
            auto& bounds = candidate.boundsHalfExtents;
            bounds.x = std::max(bounds.x, std::abs(object.position.x) + box->halfExtents.x + 2);
            bounds.y = std::max(bounds.y, std::abs(object.position.y) + box->halfExtents.y + 2);
            bounds.z = std::max(bounds.z, std::abs(object.position.z) + box->halfExtents.z + 2);
        } else if (const auto* marker = std::get_if<WorldMarker>(&object.data)) {
            if (marker->type != "spawn") return "Unsupported marker '" + marker->type + "' on " + object.id;
            if (!candidate.playerSpawnId.empty()) return "Exactly one player spawn is supported; duplicate spawn: " + object.id;
            candidate.playerSpawnId = object.id;
            candidate.playerSpawn = object.position;
        } else if (const auto* asset = std::get_if<WorldAssetReference>(&object.data)) {
            if (!catalog.contains(asset->assetId)) return "Unknown catalog asset '" + asset->assetId + "' on " + object.id;
            return "Catalog asset '" + asset->assetId + "' on " + object.id +
                " is not supported by the authored-world mesh/collision path yet; replace with a box for this playtest";
        } else return "Unsupported object data: " + object.id;
    }
    if (candidate.playerSpawnId.empty()) return "Add exactly one spawn marker";
    bool support = false;
    const auto spawn = candidate.playerSpawn;
    for (const auto& box : candidate.primitives) {
        if (spawnInsideSolid(spawn, box)) return "Spawn overlaps blocking box: " + box.id;
        if (box.blocksMovement &&
            std::abs(spawn.y - (box.center.y + box.halfExtents.y)) <= 0.001F &&
            std::abs(spawn.x - box.center.x) + kPlayerCapsuleRadius <= box.halfExtents.x &&
            std::abs(spawn.z - box.center.z) + kPlayerCapsuleRadius <= box.halfExtents.z) support = true;
    }
    if (!support) return "Spawn must stand on a blocking box with standing-capsule clearance";
    const auto physics = buildGreyboxPhysicsWorld(candidate);
    novacore::physics::CharacterQuery query{};
    query.position = spawn;
    query.radius = kPlayerCapsuleRadius;
    query.height = kPlayerStandingHeight;
    query.enableStepUp = false;
    query.enableGroundSnap = false;
    const auto resolved = physics.resolveCharacter(query);
    if (!resolved.grounded || resolved.groundColliderId.empty() ||
        novacore::math::length(resolved.position - spawn) > 0.002F)
        return "Spawn failed the actual character-controller support/clearance check";
    out = std::move(candidate);
    return std::nullopt;
}
std::optional<std::string> loadAuthoredWorld(const std::filesystem::path& path,
    const assets::GameAssetCatalog& catalog, GreyboxWorld& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return "Cannot open world file: " + path.string();
    std::string bytes;
    std::array<char, 16384> buffer{};
    while (file) {
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = static_cast<std::size_t>(file.gcount());
        if (count > novacore::world::kWorldJsonByteLimit - bytes.size()) return "World file exceeds 64 MiB";
        bytes.append(buffer.data(), count);
    }
    if (!file.eof() || file.bad()) return "Cannot read world file: " + path.string();
    novacore::world::WorldDocument doc;
    if (auto error = novacore::world::deserializeWorldJson(bytes, doc)) return error;
    return buildAuthoredWorld(doc, catalog, out);
}
} // namespace nemisis::dev

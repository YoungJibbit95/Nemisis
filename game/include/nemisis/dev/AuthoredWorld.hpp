#pragma once
#include "nemisis/assets/GameAssetCatalog.hpp"
#include "nemisis/dev/GreyboxWorld.hpp"
#include "novacore/world/WorldDocument.hpp"
#include <filesystem>
#include <optional>
#include <string>

namespace nemisis::dev {
inline constexpr std::size_t kAuthoredPlaytestObjectLimit = 4096;
// Transactional adapter. One grounded spawn, explicit box geometry.
// Unsupported asset instances and markers are errors, not silent omissions.
[[nodiscard]] std::optional<std::string> buildAuthoredWorld(
    const novacore::world::WorldDocument& document,
    const assets::GameAssetCatalog& catalog, GreyboxWorld& out);
[[nodiscard]] std::optional<std::string> loadAuthoredWorld(
    const std::filesystem::path& path, const assets::GameAssetCatalog& catalog, GreyboxWorld& out);
} // namespace nemisis::dev

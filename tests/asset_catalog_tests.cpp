#include "nemisis/assets/GameAssetCatalog.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

void testCatalogLoadsDevSandboxAssets() {
    const auto path = std::filesystem::temp_directory_path() / "nemisis_asset_catalog_test.json";
    {
        std::ofstream file(path);
        file << R"({
            "manifest": {
                "name": "catalog_test",
                "root": "assets"
            },
            "assets": [
                {
                    "id": "wpn_ar_01",
                    "kind": "mesh",
                    "source": "assets/source/blender/weapons/wpn_ar_01.blend",
                    "cooked": "assets/export/gltf/weapons/wpn_ar_01.glb",
                    "streamable": true,
                    "priority": 95,
                    "tags": ["weapon", "dev_sandbox"]
                },
                {
                    "id": "prop_target_dummy_01",
                    "kind": "mesh",
                    "source": "assets/source/blender/props/prop_target_dummy_01.blend",
                    "cooked": "assets/export/gltf/props/prop_target_dummy_01.glb",
                    "streamable": true,
                    "priority": 90,
                    "tags": ["prop", "dev_sandbox"]
                }
            ]
        })";
    }

    nemisis::assets::GameAssetCatalog catalog;
    const auto result = catalog.loadFromJson(path);
    expect(result.ok(), "game asset catalog loads manifest");
    expect(catalog.assetCount() == 2, "game asset catalog counts assets");
    expect(catalog.contains("wpn_ar_01"), "game asset catalog finds weapon");
    expect(catalog.idsForTag("weapon").size() == 1, "game asset catalog filters by tag");

    const auto devAssets = catalog.devSandboxAssetIds();
    expect(devAssets.size() == 2, "game asset catalog finds dev sandbox assets");

    const auto zone = catalog.devSandboxStreamingZone();
    expect(zone.name == "dev_sandbox", "game asset catalog names dev streaming zone");
    expect(zone.radiusMeters > 60.0F, "game asset catalog assigns useful dev streaming radius");
    expect(zone.preloadAssets.size() == 2, "game asset catalog fills dev streaming zone");

    std::filesystem::remove(path);
}

} // namespace

int main() {
    testCatalogLoadsDevSandboxAssets();

    if (failures > 0) {
        std::cerr << failures << " asset catalog test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis asset catalog tests passed\n";
    return EXIT_SUCCESS;
}

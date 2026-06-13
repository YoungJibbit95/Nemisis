#include "nemisis/dev/GreyboxWorld.hpp"

#include <cstdlib>
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

void testDevRangeGreyboxComposition() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();

    expect(world.id == "dev_range_greybox_01", "greybox id is stable");
    expect(world.primitives.size() >= 14, "dev range has enough primitives for a first blockout");
    expect(nemisis::dev::countPrimitivesByKind(world, nemisis::dev::GreyboxPrimitiveKind::Floor) == 1, "one floor");
    expect(nemisis::dev::countPrimitivesByKind(world, nemisis::dev::GreyboxPrimitiveKind::Wall) >= 4, "walls include asset-stage collision");
    expect(nemisis::dev::countPrimitivesByKind(world, nemisis::dev::GreyboxPrimitiveKind::Cover) >= 10, "cover primitives include asset-stage proxies");
    expect(nemisis::dev::countPrimitivesByKind(world, nemisis::dev::GreyboxPrimitiveKind::Ledge) == 1, "one mantle/ledge training primitive");
    expect(nemisis::dev::countPrimitivesByKind(world, nemisis::dev::GreyboxPrimitiveKind::Ramp) >= 3, "ramp primitives include asset slide proxy");
    expect(nemisis::dev::countPrimitivesByKind(world, nemisis::dev::GreyboxPrimitiveKind::WallRunPanel) >= 5, "wallrun panels include asset proxy");
    expect(nemisis::dev::countPrimitivesByKind(world, nemisis::dev::GreyboxPrimitiveKind::Spawn) == 2, "two spawn markers");
    expect(nemisis::dev::findPrimitive(world, "target_debug_dummy") != nullptr, "target proxy exists");
    expect(nemisis::dev::findPrimitive(world, "asset_a2_slide_ramp_collision") != nullptr, "A2 slide ramp collision proxy exists");
    expect(nemisis::dev::findPrimitive(world, "asset_a2_wallrun_panel_collision") != nullptr, "A2 wallrun panel collision proxy exists");
    expect(nemisis::dev::findPrimitive(world, "asset_a2_cover_crate_collision") != nullptr, "A2 cover crate collision proxy exists");
    expect(world.playerSpawn.z < 0.0F, "player spawn starts near shooting lane entrance");
}

} // namespace

int main() {
    testDevRangeGreyboxComposition();

    if (failures > 0) {
        std::cerr << failures << " greybox world test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis greybox world tests passed\n";
    return EXIT_SUCCESS;
}

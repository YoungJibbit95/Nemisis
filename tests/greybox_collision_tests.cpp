#include "nemisis/dev/GreyboxCollision.hpp"

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

void testBoundsClamp() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        nemisis::dev::GreyboxCollisionQuery{{40.0F, 0.0F, -12.0F}, 0.42F, 1.80F});

    expect(result.blocked, "out-of-bounds player is corrected");
    expect(result.position.x <= world.boundsHalfExtents.x - 0.42F + 0.001F, "bounds correction clamps x");
    expect(!result.lastPrimitiveId.empty(), "bounds correction reports a source");
}

void testCoverPushout() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        nemisis::dev::GreyboxCollisionQuery{{-8.0F, 0.0F, 4.0F}, 0.42F, 1.80F});

    expect(result.blocked, "player inside cover is pushed out");
    expect(result.hitCount >= 1, "cover collision increments hit count");
    expect(result.lastPrimitiveId == "cover_left_mid", "cover correction reports primitive id");
}

void testOpenLaneStaysFree() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        nemisis::dev::GreyboxCollisionQuery{world.playerSpawn, 0.42F, 1.80F});

    expect(!result.blocked, "player spawn is not blocked");
    expect(result.hitCount == 0, "open lane has no blocking collision");
    expect(result.grounded, "feet at floor are grounded");
}

} // namespace

int main() {
    testBoundsClamp();
    testCoverPushout();
    testOpenLaneStaysFree();

    if (failures > 0) {
        std::cerr << failures << " greybox collision test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis greybox collision tests passed\n";
    return EXIT_SUCCESS;
}

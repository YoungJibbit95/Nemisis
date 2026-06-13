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
    expect(result.groundPrimitiveId == "floor_main", "open lane reports floor as ground");
    expect(!result.onRamp, "open lane is not on a ramp");
    expect(!result.stepped, "open lane is not a step");
}

void testRampSurfaceGroundsPlayer() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        nemisis::dev::GreyboxCollisionQuery{{-13.5F, 0.28F, -1.0F}, 0.42F, 1.80F});

    expect(result.grounded, "player snaps to ramp surface");
    expect(result.onRamp, "ramp collision reports ramp state");
    expect(!result.blocked, "walkable ramp does not block movement");
    expect(result.groundPrimitiveId == "ramp_left_slide", "ramp collision reports primitive id");
    expect(result.position.y > 0.40F && result.position.y < 0.50F, "ramp collision resolves sampled height");
    expect(result.groundNormal.y > 0.98F, "ramp normal stays walkable");
    expect(result.groundNormal.z < 0.0F, "positive-z ramp normal leans against slope");
}

void testLowStepIsWalkable() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        nemisis::dev::GreyboxCollisionQuery{{-3.5F, 0.0F, -7.0F}, 0.42F, 1.80F});

    expect(result.grounded, "player remains grounded on low step");
    expect(result.stepped, "low step reports stepped state");
    expect(!result.blocked, "low step does not block movement");
    expect(result.groundPrimitiveId == "step_training_low", "step collision reports primitive id");
    expect(result.position.y > 0.34F && result.position.y < 0.38F, "step collision resolves top height");
}

void testRisingJumpDoesNotSnapBackToGround() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::GreyboxCollisionQuery query{};
    query.position = {0.0F, 0.10F, -12.0F};
    query.radius = 0.42F;
    query.height = 1.80F;
    query.snapDownDistance = 0.35F;
    query.enableGroundSnap = false;

    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(world, query);

    expect(!result.grounded, "rising player does not snap back to floor");
    expect(result.position.y > 0.09F, "rising collision preserves upward clearance");
    expect(result.groundPrimitiveId.empty(), "rising collision does not keep stale ground id");
}

void testAirborneStepUpCanBeDisabled() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::GreyboxCollisionQuery query{};
    query.position = {-3.5F, 0.12F, -7.0F};
    query.radius = 0.42F;
    query.height = 1.80F;
    query.enableGroundSnap = false;
    query.enableStepUp = false;

    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(world, query);

    expect(!result.grounded, "airborne player does not step onto low cover when step-up is disabled");
    expect(!result.stepped, "disabled step-up does not emit step telemetry");
}

void testLeavingRaisedSupportBecomesAirborne() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::GreyboxCollisionQuery query{};
    query.position = {-3.5F, 0.36F, -5.35F};
    query.radius = 0.42F;
    query.height = 1.80F;
    query.snapDownDistance = 0.20F;

    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(world, query);

    expect(!result.grounded, "player past low-step support becomes airborne");
    expect(result.groundPrimitiveId.empty(), "off-support collision does not report stale ground");
}

void testMidLedgeStillBlocksWithoutMantle() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        nemisis::dev::GreyboxCollisionQuery{{3.8F, 0.0F, -6.5F}, 0.42F, 1.80F});

    expect(result.blocked, "mid ledge blocks until mantle exists");
    expect(!result.stepped, "mid ledge is above step height");
    expect(result.lastPrimitiveId == "ledge_training_mid", "mid ledge reports primitive id");
}

void testMidLedgeReportsMantleCandidate() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::GreyboxCollisionQuery query{};
    query.position = {3.8F, 0.0F, -8.05F};
    query.mantleForward = {0.0F, 0.0F, 1.0F};
    query.mantleMaxDistance = 1.45F;
    query.mantleMinHeight = 0.44F;
    query.mantleMaxHeight = 1.45F;

    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(world, query);

    expect(result.mantleCandidate, "mid ledge reports mantle candidate when approached from front");
    expect(result.mantlePrimitiveId == "ledge_training_mid", "mantle candidate reports primitive id");
    expect(result.mantleKind == nemisis::dev::GreyboxPrimitiveKind::Ledge, "mantle candidate reports primitive kind");
    expect(result.mantleTargetPosition.y > 1.25F && result.mantleTargetPosition.y < 1.35F, "mantle target lands on ledge top");
    expect(result.mantleNormal.z < 0.0F, "mantle normal faces approach");

    nemisis::dev::GreyboxCollisionQuery topQuery{};
    topQuery.position = result.mantleTargetPosition;
    const auto topResult = nemisis::dev::resolveGreyboxPlayerCollision(world, topQuery);
    expect(topResult.grounded, "mantle target is grounded");
    expect(!topResult.blocked, "mantle target does not side-block");
    expect(topResult.groundPrimitiveId == "ledge_training_mid", "mantle target grounds on ledge");
}

void testWallRunPanelReportsSurfaceContact() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    nemisis::dev::GreyboxCollisionQuery query{};
    query.position = {-18.1F, 0.72F, -5.5F};
    query.radius = 0.42F;
    query.height = 1.80F;
    query.wallProbeDistance = 0.55F;
    query.enableGroundSnap = false;

    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(world, query);

    expect(result.blocked, "wallrun panel still blocks capsule penetration");
    expect(!result.grounded, "airborne wallrun probe does not snap to floor");
    expect(result.nearWallRunSurface, "wallrun panel reports runnable surface contact");
    expect(result.wallPrimitiveId == "wallrun_left_panel_a", "wallrun contact reports primitive id");
    expect(result.wallKind == nemisis::dev::GreyboxPrimitiveKind::WallRunPanel, "wallrun contact reports primitive kind");
    expect(result.wallTangent.lengthSquared() > 0.5F, "wallrun contact provides tangent direction");
}

} // namespace

int main() {
    testBoundsClamp();
    testCoverPushout();
    testOpenLaneStaysFree();
    testRampSurfaceGroundsPlayer();
    testLowStepIsWalkable();
    testRisingJumpDoesNotSnapBackToGround();
    testAirborneStepUpCanBeDisabled();
    testLeavingRaisedSupportBecomesAirborne();
    testMidLedgeStillBlocksWithoutMantle();
    testMidLedgeReportsMantleCandidate();
    testWallRunPanelReportsSurfaceContact();

    if (failures > 0) {
        std::cerr << failures << " greybox collision test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis greybox collision tests passed\n";
    return EXIT_SUCCESS;
}

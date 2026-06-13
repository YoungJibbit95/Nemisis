#include "nemisis/dev/GreyboxCollision.hpp"

#include <algorithm>
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

nemisis::dev::GreyboxCollisionQuery queryAt(novacore::math::Vec3 position) {
    nemisis::dev::GreyboxCollisionQuery query{};
    query.position = position;
    query.previousPosition = position;
    query.radius = 0.42F;
    query.height = 1.80F;
    return query;
}

void testBoundsClamp() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        queryAt({40.0F, 0.0F, -12.0F}));

    expect(result.blocked, "out-of-bounds player is corrected");
    expect(result.position.x <= world.boundsHalfExtents.x - 0.42F + 0.001F, "bounds correction clamps x");
    expect(!result.lastPrimitiveId.empty(), "bounds correction reports a source");
    expect(
        std::any_of(
            result.contacts.begin(),
            result.contacts.end(),
            [](const nemisis::dev::GreyboxContact& contact) {
                return contact.role == nemisis::dev::GreyboxContactRole::Bounds && contact.blocking;
            }),
        "bounds correction appears in greybox contact list");
}

void testCoverPushout() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        queryAt({-8.0F, 0.0F, 4.0F}));

    expect(result.blocked, "player inside cover is pushed out");
    expect(result.hitCount >= 1, "cover collision increments hit count");
    expect(result.lastPrimitiveId == "cover_left_mid", "cover correction reports primitive id");
}

void testOpenLaneStaysFree() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        queryAt(world.playerSpawn));

    expect(!result.blocked, "player spawn is not blocked");
    expect(result.hitCount == 0, "open lane has no blocking collision");
    expect(result.grounded, "feet at floor are grounded");
    expect(result.groundPrimitiveId == "floor_main", "open lane reports floor as ground");
    expect(!result.onRamp, "open lane is not on a ramp");
    expect(!result.stepped, "open lane is not a step");
    expect(result.contacts.size() == 1, "open lane exposes one greybox contact");
    expect(result.contacts.front().role == nemisis::dev::GreyboxContactRole::Ground, "open lane contact is ground");
    expect(result.contacts.front().walkable, "open lane contact is walkable");
}

void testRampSurfaceGroundsPlayer() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        queryAt({-13.5F, 0.28F, -1.0F}));

    expect(result.grounded, "player snaps to ramp surface");
    expect(result.onRamp, "ramp collision reports ramp state");
    expect(!result.blocked, "walkable ramp does not block movement");
    expect(result.groundPrimitiveId == "ramp_left_slide", "ramp collision reports primitive id");
    expect(result.position.y > 0.40F && result.position.y < 0.50F, "ramp collision resolves sampled height");
    expect(result.groundNormal.y > 0.98F, "ramp normal stays walkable");
    expect(result.groundNormal.z < 0.0F, "positive-z ramp normal leans against slope");

    const auto highResult = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        queryAt({-13.5F, 0.12F, 2.65F}));
    expect(highResult.grounded, "player can ground near ramp top");
    expect(highResult.onRamp, "ramp top still reports ramp state");
    expect(highResult.groundPrimitiveId == "ramp_left_slide", "ramp top reports primitive id");
    expect(highResult.position.y > 0.82F, "ramp top resolves high surface");
}

void testAssetCollisionProxiesBlockAndGroundPlayer() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();

    const auto crate = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        queryAt({3.60F, 0.0F, -3.95F}));
    expect(crate.blocked, "A2 cover crate collision proxy blocks player");
    expect(crate.lastPrimitiveId == "asset_a2_cover_crate_collision", "A2 crate collision reports proxy id");

    const auto ramp = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        queryAt({-5.65F, 0.08F, -5.35F}));
    expect(ramp.grounded, "A2 slide ramp proxy grounds player");
    expect(ramp.onRamp, "A2 slide ramp proxy reports ramp state");
    expect(ramp.groundPrimitiveId == "asset_a2_slide_ramp_collision", "A2 ramp collision reports proxy id");

    nemisis::dev::GreyboxCollisionQuery wallrun{};
    wallrun.position = {4.92F, 0.78F, -5.20F};
    wallrun.radius = 0.42F;
    wallrun.height = 1.80F;
    wallrun.wallProbeDistance = 0.60F;
    wallrun.enableGroundSnap = false;
    const auto wallrunResult = nemisis::dev::resolveGreyboxPlayerCollision(world, wallrun);
    expect(wallrunResult.nearWallRunSurface, "A2 wallrun panel proxy reports runnable contact");
    expect(wallrunResult.wallPrimitiveId == "asset_a2_wallrun_panel_collision", "A2 wallrun proxy reports id");
}

void testLowStepIsWalkable() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        queryAt({-3.5F, 0.0F, -7.0F}));

    expect(result.grounded, "player remains grounded on low step");
    expect(result.stepped, "low step reports stepped state");
    expect(!result.blocked, "low step does not block movement");
    expect(result.groundPrimitiveId == "step_training_low", "step collision reports primitive id");
    expect(result.position.y > 0.34F && result.position.y < 0.38F, "step collision resolves top height");
    expect(
        std::any_of(
            result.contacts.begin(),
            result.contacts.end(),
            [](const nemisis::dev::GreyboxContact& contact) {
                return contact.role == nemisis::dev::GreyboxContactRole::Step &&
                    contact.primitiveId == "step_training_low";
            }),
        "low step appears as explicit contact role");
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
    query.position = {-9.0F, 0.36F, -4.0F};
    query.radius = 0.42F;
    query.height = 1.80F;
    query.snapDownDistance = 0.20F;

    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(world, query);

    expect(!result.grounded, "player past low-step support becomes airborne");
    expect(result.groundPrimitiveId.empty(), "off-support collision does not report stale ground");
}

void testSweepStopsFastLedgeTunneling() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto query = queryAt({3.8F, 0.0F, -3.0F});
    query.previousPosition = {3.8F, 0.0F, -10.0F};
    query.useSweep = true;

    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(world, query);

    expect(result.swept, "greybox collision reports sweep path");
    expect(result.sweepHit, "greybox sweep catches high-speed ledge hit");
    expect(result.sweepPrimitiveId == "ledge_training_mid", "greybox sweep reports ledge primitive");
    expect(result.position.z < -7.75F, "greybox sweep stops before ledge face");
    expect(result.blocked, "greybox sweep marks blocked movement");
    expect(!result.sweepContacts.empty(), "greybox sweep exposes sweep contacts");
    expect(
        std::any_of(
            result.contacts.begin(),
            result.contacts.end(),
            [](const nemisis::dev::GreyboxContact& contact) {
                return contact.role == nemisis::dev::GreyboxContactRole::Sweep &&
                    contact.primitiveId == "ledge_training_mid";
            }),
        "greybox sweep contact is merged into final contact list");
}

void testSweepAllowsValidLowStep() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto query = queryAt({-3.5F, 0.0F, -6.8F});
    query.previousPosition = {-3.5F, 0.0F, -9.0F};
    query.useSweep = true;

    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(world, query);

    expect(result.swept, "low-step query uses sweep");
    expect(!result.sweepHit, "valid low-step movement is not side-blocked by sweep");
    expect(result.grounded, "swept low-step movement resolves as grounded");
    expect(result.stepped, "swept low-step movement preserves step telemetry");
    expect(result.groundPrimitiveId == "step_training_low", "swept low-step movement grounds on step primitive");
}

void testMidLedgeStillBlocksWithoutMantle() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(
        world,
        queryAt({3.8F, 0.0F, -6.5F}));

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

void testSweepReportsWallRunPanelContact() {
    const auto world = nemisis::dev::createDevRangeGreyboxWorld();
    auto query = queryAt({-19.5F, 0.72F, -4.0F});
    query.previousPosition = {-16.5F, 0.72F, -5.5F};
    query.useSweep = true;
    query.enableGroundSnap = false;
    query.wallProbeDistance = 0.55F;

    const auto result = nemisis::dev::resolveGreyboxPlayerCollision(world, query);

    expect(result.sweepHit, "greybox sweep hits wallrun panel");
    expect(result.sweepPrimitiveId == "wallrun_left_panel_a", "greybox sweep reports wallrun panel id");
    expect(result.nearWallRunSurface, "greybox sweep preserves wallrun surface telemetry");
    expect(result.sweepNormal.x > 0.5F, "greybox sweep reports wall normal");
    expect(result.appliedDisplacement.z > 0.8F, "greybox sweep keeps tangential movement");
    expect(
        std::string_view(nemisis::dev::greyboxContactRoleName(nemisis::dev::GreyboxContactRole::Wall)) == "wall",
        "greybox contact role names are stable");
}

} // namespace

int main() {
    testBoundsClamp();
    testCoverPushout();
    testOpenLaneStaysFree();
    testRampSurfaceGroundsPlayer();
    testAssetCollisionProxiesBlockAndGroundPlayer();
    testLowStepIsWalkable();
    testRisingJumpDoesNotSnapBackToGround();
    testAirborneStepUpCanBeDisabled();
    testLeavingRaisedSupportBecomesAirborne();
    testSweepStopsFastLedgeTunneling();
    testSweepAllowsValidLowStep();
    testMidLedgeStillBlocksWithoutMantle();
    testMidLedgeReportsMantleCandidate();
    testWallRunPanelReportsSurfaceContact();
    testSweepReportsWallRunPanelContact();

    if (failures > 0) {
        std::cerr << failures << " greybox collision test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis greybox collision tests passed\n";
    return EXIT_SUCCESS;
}

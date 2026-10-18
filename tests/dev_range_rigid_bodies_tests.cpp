#include "nemisis/dev/DevRangeRigidBodies.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"
#include "nemisis/player/PlayerFirstPersonRig.hpp"
#include "novacore/assets/GltfDocument.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>

namespace {
using namespace novacore;
using namespace nemisis::dev;
int failures = 0;
void expect(bool ok, const char* why) {
    if (!ok) { ++failures; std::cerr << why << '\n'; }
}
bool near(math::Vec3 a, math::Vec3 b) { return (a-b).lengthSquared() < 1.e-8F; }
void testSharedCollisionSource() {
    const auto greybox = createDevRangeGreyboxWorld();
    const auto source = buildGreyboxPhysicsWorld(greybox);
    for (const auto p : {math::Vec3{0,0,-12}, math::Vec3{-13.5F,.5F,-1}, math::Vec3{24,0,4}, math::Vec3{-3.5F,.4F,-7}}) {
        GreyboxCollisionQuery query;
        query.position = p;
        query.previousPosition = p - math::Vec3{.3F,0,.1F};
        query.useSweep = true;
        const auto rebuilt = resolveGreyboxPlayerCollision(greybox, query);
        const auto shared = resolveGreyboxPlayerCollision(greybox, source, query);
        expect(near(rebuilt.position, shared.position) && rebuilt.grounded == shared.grounded &&
            rebuilt.sweepHit == shared.sweepHit && rebuilt.groundPrimitiveId == shared.groundPrimitiveId &&
            rebuilt.mantlePrimitiveId == shared.mantlePrimitiveId,
            "cached canonical collision source preserves existing KCC result");
    }
}

void testRealRangePropsAndVisuals() {
    auto greybox = createDevRangeGreyboxWorld();
    DevRangeRigidBodies props;
    props.reset(greybox);
    const auto firstHandle = props.bodies().bodies().front().handle;
    const auto firstPositions = props.bodies().bodies();
    for (int tick=0; tick<360; ++tick) { props.tick(greybox.playerSpawn); }
    expect(props.tickCount() == 360, "Dev Range uses consecutive fixed solver ticks");
    std::size_t dynamicCount = 0, staticCount = 0;
    for (const auto& body : props.bodies().bodies()) {
        if (body.canonicalCollider) { ++staticCount; }
        if (body.motion != physics::RigidBodyMotion::Dynamic) { continue; }
        ++dynamicCount;
        expect(std::isfinite(body.position.lengthSquared()) && body.position.y > .27F && body.position.y < 2.F,
            "six actual Dev Range crates settle on the same authored floor without NaNs");
    }
    expect(dynamicCount == 6 && staticCount == props.collisionWorld().staticColliders().size(),
        "solver derives all statics from the single KCC source and owns exactly six props");

    assets::GltfMeshData mesh;
    const auto path = std::filesystem::path(NEMISIS_SOURCE_ROOT) /
        "assets/generated/a2_visual_pack/map_a2_cover_crate_01.glb";
    expect(assets::loadGltfMeshData(path, mesh).ok(),
        "existing actual crate imports through public engine API");
    props.setVisual({0,1}, mesh);
    const auto snapshot = props.bodies().bodies();
    const auto crate = std::find_if(snapshot.begin(), snapshot.end(), [](const auto& b) { return b.id == "physics_crate_0"; });
    expect(crate != snapshot.end(), "known crate remains live");
    if (crate != snapshot.end()) {
        expect(props.impact(crate->position + math::Vec3{0,.10F,-2}, {0,0,4}), "real shot query can impulse closest visible crate");
    }
    for (int tick=0; tick<20; ++tick) { props.tick(greybox.playerSpawn); }
    render::RenderFrameInfo frame;
    props.append(frame);
    expect(frame.worldMeshes.size() == 6, "actual crate resource submits six dynamic render instances");
    const auto physical = props.bodies().bodies();
    for (std::size_t i=0; i<frame.worldMeshes.size(); ++i) {
        const auto& rendered = frame.worldMeshes[i];
        const auto id = "physics_crate_" + std::to_string(i);
        const auto found = std::find_if(physical.begin(), physical.end(), [&](const auto& b) { return b.id == id; });
        if (found == physical.end()) { expect(false, "rendered instance has an application body ID"); continue; }
        const auto matrix = nemisis::player::firstPersonAttachmentMatrix({rendered.position,rendered.scale,
            rendered.yawDegrees,rendered.pitchDegrees,rendered.rollDegrees});
        math::Vec3 lo{100,100,100}, hi{-100,-100,-100};
        const auto q = found->rotation;
        const auto inverse = animation::matrixFromTransform({{}, {-q.x,-q.y,-q.z,q.w}, {1,1,1}});
        for (const auto& primitive : mesh.primitives) for (const auto p : primitive.positions) {
            const auto world = animation::transformPoint(matrix,p);
            const auto local = animation::transformPoint(inverse,world-found->position);
            lo = {std::min(lo.x,local.x),std::min(lo.y,local.y),std::min(lo.z,local.z)};
            hi = {std::max(hi.x,local.x),std::max(hi.y,local.y),std::max(hi.z,local.z)};
        }
        expect(near(lo,{-.3F,-.3F,-.3F}) && near(hi,{.3F,.3F,.3F}),
            "actual rendered cooked geometry fits solver OBB once, including physical angular motion");
    }
    // Approach from the open side using bounded KCC steps, never teleport the
    // kinematic proxy across the world or sweep it through the stack first.
    auto feet = greybox.playerSpawn;
    const auto walk = [&](math::Vec3 desired) {
        GreyboxCollisionQuery query;
        query.previousPosition = feet;
        query.position = desired;
        query.useSweep = true;
        feet = resolveGreyboxPlayerCollision(greybox, props.collisionWorld(), query).position;
        props.tick(feet);
    };
    for (int tick=1; tick<=20; ++tick) { walk({0,0,-12.F-tick*.1F}); }
    for (int tick=1; tick<=120; ++tick) { walk({-tick*.1F,0,-14}); }
    for (int tick=1; tick<=20; ++tick) { walk({-12,0,-14.F+tick*.1F}); }
    const auto beforePush = props.bodies().bodies();
    const auto beforeCrate = std::find_if(beforePush.begin(), beforePush.end(), [](const auto& b) { return b.id == "physics_crate_0"; });
    for (int tick=0; tick<=100; ++tick) {
        walk({-12.F + tick*.025F, 0, -12.F});
    }
    const auto afterPush = props.bodies().bodies();
    const auto afterCrate = std::find_if(afterPush.begin(), afterPush.end(), [](const auto& b) { return b.id == "physics_crate_0"; });
    expect(beforeCrate != beforePush.end() && afterCrate != afterPush.end() &&
        afterCrate->position.x > beforeCrate->position.x + .1F,
        "resolved player capsule actually walks into and pushes a game crate");
    if (beforeCrate != beforePush.end() && afterCrate != afterPush.end()) {
        std::cout << "Dev Range fixture: props=6 fixed_ticks=" << props.tickCount()
                  << " walked_push_dx_m=" << afterCrate->position.x-beforeCrate->position.x << '\n';
    }
    props.reset(greybox);
    expect(props.tickCount() == 0 && !props.bodies().body(firstHandle), "range reset invalidates old body handles and restarts tick");
    const auto reset = props.bodies().bodies();
    expect(reset.size() == firstPositions.size(), "range reset recreates bounded initial fixture count");
    for (std::size_t i=0; i<reset.size() && i<firstPositions.size(); ++i) {
        expect(reset[i].id == firstPositions[i].id && near(reset[i].position,firstPositions[i].position),
            "range reset reproduces initial placements");
    }
    greybox.primitives.push_back({"impulse_occluder",GreyboxPrimitiveKind::Wall,{-10,.5F,-14},{1,1,.1F},{},true});
    props.reset(greybox);
    expect(!props.impact({-10,.5F,-16},{0,0,8}), "shot after reset fails closed before static synchronization");
    props.tick(greybox.playerSpawn);
    expect(!props.impact({-10,.5F,-16},{0,0,8}), "canonical wall occludes prop impulse instead of prop-only ray through wall");
}
}

int main() {
    testSharedCollisionSource();
    if (novacore::physics::RigidBodyWorld::backendAvailable()) { testRealRangePropsAndVisuals(); }
    else { std::cout << "Jolt disabled: real dynamic-scene acceptance not run\n"; }
    return failures ? 1 : 0;
}

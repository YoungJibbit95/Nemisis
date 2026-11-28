#include "nemisis/dev/AuthoredWorld.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"
#include "nemisis/dev/DevRangeRenderScene.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "novacore/world/WorldJson.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace {
using namespace nemisis::dev;
using namespace novacore::world;
using novacore::math::Vec3;
int checks=0;
void require(bool condition, std::string_view message) {
    ++checks;
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
bool near(float a,float b) { return std::abs(a-b)<0.01F; }
bool same(Vec3 a,Vec3 b) { return near(a.x,b.x)&&near(a.y,b.y)&&near(a.z,b.z); }
}
int main(int argc, char** argv) {
    require(argc==3, "map and catalog paths required");
    nemisis::assets::GameAssetCatalog catalog;
    require(catalog.loadFromJson(argv[2]).ok(), "actual game asset catalog");
    GreyboxWorld world;
    require(!loadAuthoredWorld(argv[1],catalog,world), "load authored room file");
    require(world.authored && world.id=="editor-room" && world.playerSpawnId=="spawn" &&
        same(world.playerSpawn,{0,0,-2}), "world identity and spawn ID/position");
    require(world.primitives.size()==7, "only authored geometry");
    const auto physics=buildGreyboxPhysicsWorld(world);
    require(!physics.implicitBoundaryCollision() && physics.staticColliders().size()==6, "only blocking authored geometry becomes colliders");
    for (const auto& primitive:world.primitives) {
        const auto* collider=physics.findStaticCollider(primitive.id);
        require(primitive.blocksMovement == (collider!=nullptr), "blocking flag parity");
        if (collider) require(same(collider->center,primitive.center)&&same(collider->halfExtents,primitive.halfExtents), "collider transform parity");
    }
    novacore::render::RenderFrameInfo frame;
    MeshResourceLookup meshes;
    DevRangeRenderSceneDesc desc;
    desc.greyboxWorld=&world;
    desc.meshResources=&meshes;
    desc.showWorldDebugLines=false;
    const auto stats=DevRangeRenderSceneBuilder{}.append(frame,desc);
    require(stats.worldBoxCount==world.primitives.size() && frame.worldBoxes.size()==world.primitives.size()
        && frame.worldMeshes.empty(), "no legacy scenery or targets in authored render extraction");
    for (std::size_t i=0;i<world.primitives.size();++i) {
        require(same(frame.worldBoxes[i].center,world.primitives[i].center)&&
            same(frame.worldBoxes[i].halfExtents,world.primitives[i].halfExtents),"render transform parity");
    }
    GreyboxCollisionQuery q;
    q.useSweep=true; q.enableStepUp=false;
    q.previousPosition=world.playerSpawn; q.position={0,0,3};
    auto result=resolveGreyboxPlayerCollision(world,physics,q);
    require(result.grounded && result.groundPrimitiveId=="floor", "authored floor supports player");
    require(result.position.z>2.8F, "door and nonblocking box traversable");
    q.previousPosition={3,0,0}; q.position={3,0,3};
    result=resolveGreyboxPlayerCollision(world,physics,q);
    require(result.position.z<1.5F && result.sweepHit, "closed wall stops capsule");
    q.previousPosition={6,1,0}; q.position={6,-1,0};
    result=resolveGreyboxPlayerCollision(world,physics,q);
    require(!result.grounded && result.position.y<0, "no hidden floor outside authored room");
    WorldDocument doc{kWorldSchemaVersion,"test",{
        {"floor","Floor",{0,-0.1F,0},WorldBox{{4,0.1F,4},true}},
        {"spawn","Spawn",{0,0,0},WorldMarker{"spawn"}}
    }};
    auto changed=doc;
    changed.objects.push_back({"wall","Wall",{0,1.5F,2},WorldBox{{3,1.5F,0.15F},true}});
    std::string encoded;
    require(!serializeWorldJson(changed,encoded), "encode modified map");
    WorldDocument decoded;
    require(!deserializeWorldJson(encoded,decoded) && !buildAuthoredWorld(decoded,catalog,world),"reopen modified map without code changes");
    require(world.id=="test" && world.primitives.size()==2 && world.primitives[1].id=="wall", "modified authored IDs");
    auto invalid=doc; invalid.objects.pop_back();
    require(buildAuthoredWorld(invalid,catalog,world).has_value() && world.primitives.size()==2, "missing spawn fails transactionally");
    invalid=doc; invalid.objects[0].data=WorldBox{{4,0.1F,4},false};
    require(buildAuthoredWorld(invalid,catalog,world).has_value(),"nonblocking floor cannot support spawn");
    invalid=doc; invalid.objects[1].position={0,1,0};
    require(buildAuthoredWorld(invalid,catalog,world).has_value(),"floating spawn rejected");
    invalid=doc; invalid.objects.push_back({"second","Second",{1,0,1},WorldMarker{"spawn"}});
    require(buildAuthoredWorld(invalid,catalog,world).has_value(),"multiple spawns rejected");
    invalid=doc; invalid.objects.push_back({"asset","Unknown",{},WorldAssetReference{"missing_asset"}});
    auto err=buildAuthoredWorld(invalid,catalog,world);
    require(err && err->find("Unknown catalog asset")!=std::string::npos,"unknown asset diagnosed");
    invalid.objects.back().data=WorldAssetReference{"map_cover_crate_01"};
    err=buildAuthoredWorld(invalid,catalog,world);
    require(err && err->find("not supported")!=std::string::npos,"known unsupported asset diagnosed");
    invalid=doc; invalid.objects.push_back({"marker","Marker",{},WorldMarker{"unsupported"}});
    require(buildAuthoredWorld(invalid,catalog,world).has_value(),"unsupported marker rejected");
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state;
    state.mode=nemisis::movement::MovementMode::Airborne; state.position={0,-1,0};
    const auto legacy=movement.simulate(state,{},1.0F/60);
    const auto authored=movement.simulate(state,{},1.0F/60,{0,0,1},false);
    require(near(legacy.position.y,0) && authored.position.y < -1,"authored movement does not clamp to hidden ground");
    std::cout<<"Authored world: "<<checks<<" checks passed\n";
}

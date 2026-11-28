#include "nemisis/dev/OuterFacility.hpp"

#include <utility>

namespace nemisis::dev {
namespace {
using novacore::math::Vec3;
void box(GreyboxWorld& world, std::string id, GreyboxPrimitiveKind kind,
         Vec3 center, Vec3 extent, std::array<float, 4> color, bool solid = true) {
    world.primitives.push_back({std::move(id), kind, center, extent, color, solid});
}
DevTargetLane target(const char* id, const char* label, Vec3 position) {
    DevTargetLane lane{};
    lane.id = id;
    lane.displayName = label;
    lane.target.position = position;
    lane.target.maxHealth = 150.0F;
    lane.target.health = 150.0F;
    lane.target.radiusMeters = 0.72F;
    lane.target.humanoidHitZones = true;
    return lane;
}
} // namespace

GreyboxWorld createOuterFacilityWorld() {
    GreyboxWorld world{};
    world.id = std::string(kOuterFacilityWorldId);
    world.boundsHalfExtents = {35.0F, 7.0F, 35.0F};
    world.playerSpawn = {0.0F, 0.0F, -26.0F};
    constexpr std::array<float, 4> ground{0.22F, 0.27F, 0.25F, 1.0F};
    constexpr std::array<float, 4> wall{0.36F, 0.42F, 0.43F, 1.0F};
    constexpr std::array<float, 4> roof{0.25F, 0.31F, 0.33F, 1.0F};
    constexpr std::array<float, 4> cover{0.48F, 0.49F, 0.43F, 1.0F};
    // One shared solid ground. Both buildings have doorway openings in their
    // end walls; each roof is the same box used for rendering and collision.
    box(world,"ground",GreyboxPrimitiveKind::Floor,{0,-0.10F,0},{35,0.10F,35},ground);
    box(world,"boundary_north",GreyboxPrimitiveKind::Wall,{0,1.5F,34.75F},{35,1.5F,0.25F},wall);
    box(world,"boundary_south",GreyboxPrimitiveKind::Wall,{0,1.5F,-34.75F},{35,1.5F,0.25F},wall);
    box(world,"boundary_west",GreyboxPrimitiveKind::Wall,{-34.75F,1.5F,0},{0.25F,1.5F,35},wall);
    box(world,"boundary_east",GreyboxPrimitiveKind::Wall,{34.75F,1.5F,0},{0.25F,1.5F,35},wall);

    // West operations building: continuous south-to-north interior route.
    box(world,"west_left",GreyboxPrimitiveKind::WallRunPanel,{-21,1.5F,2},{0.20F,1.5F,10},wall);
    box(world,"west_right",GreyboxPrimitiveKind::Wall,{-7,1.5F,2},{0.20F,1.5F,10},wall);
    box(world,"west_front_left",GreyboxPrimitiveKind::Wall,{-18.5F,1.5F,-8},{2.5F,1.5F,0.20F},wall);
    box(world,"west_front_right",GreyboxPrimitiveKind::Wall,{-9.5F,1.5F,-8},{2.5F,1.5F,0.20F},wall);
    box(world,"west_back_left",GreyboxPrimitiveKind::Wall,{-18.5F,1.5F,12},{2.5F,1.5F,0.20F},wall);
    box(world,"west_back_right",GreyboxPrimitiveKind::Wall,{-9.5F,1.5F,12},{2.5F,1.5F,0.20F},wall);
    box(world,"west_roof",GreyboxPrimitiveKind::Wall,{-14,3.10F,2},{7.2F,0.10F,10.2F},roof);
    box(world,"west_console",GreyboxPrimitiveKind::Cover,{-18.0F,0.45F,2},{1.6F,0.45F,1.0F},cover);

    // Eight 0.4 m steps approach the roof from the eastern courtyard. Their
    // tops are walkable surfaces; the last sits level with the roof at 3.2 m.
    for (int step = 0; step < 8; ++step) {
        const float top = 0.4F * static_cast<float>(step + 1);
        box(world, "west_roof_step_" + std::to_string(step), GreyboxPrimitiveKind::Cover,
            {-4.8F, top * 0.5F, -7.0F + 1.25F * static_cast<float>(step)},
            {1.7F, top * 0.5F, 0.625F}, {0.18F,0.53F,0.52F,1.0F});
    }
    // Thin non-solid cyan stripe calls out the long exterior wallrun face.
    box(world, "west_wallrun_marker", GreyboxPrimitiveKind::RangeMarker,
        {-21.215F, 2.55F, 2.0F}, {0.012F,0.10F,8.5F},
        {0.08F,0.80F,0.90F,1.0F}, false);

    // East shelter: entry at z=6 and exit at z=22, linked by exterior cover.
    box(world,"east_left",GreyboxPrimitiveKind::Wall,{10,1.5F,14},{0.20F,1.5F,8},wall);
    box(world,"east_right",GreyboxPrimitiveKind::Wall,{24,1.5F,14},{0.20F,1.5F,8},wall);
    box(world,"east_front_left",GreyboxPrimitiveKind::Wall,{12.5F,1.5F,6},{2.5F,1.5F,0.20F},wall);
    box(world,"east_front_right",GreyboxPrimitiveKind::Wall,{21.5F,1.5F,6},{2.5F,1.5F,0.20F},wall);
    box(world,"east_back_left",GreyboxPrimitiveKind::Wall,{12.5F,1.5F,22},{2.5F,1.5F,0.20F},wall);
    box(world,"east_back_right",GreyboxPrimitiveKind::Wall,{21.5F,1.5F,22},{2.5F,1.5F,0.20F},wall);
    box(world,"east_roof",GreyboxPrimitiveKind::Wall,{17,3.10F,14},{7.2F,0.10F,8.2F},roof);
    box(world,"east_console",GreyboxPrimitiveKind::Cover,{21,0.45F,14},{1.4F,0.45F,1},cover);
    box(world,"courtyard_west_cover",GreyboxPrimitiveKind::Cover,{-3,0.65F,10},{1.4F,0.65F,0.75F},cover);
    box(world,"courtyard_east_cover",GreyboxPrimitiveKind::Cover,{5,0.65F,5},{1.4F,0.65F,0.75F},cover);
    box(world,"south_east_cover",GreyboxPrimitiveKind::Cover,{12,0.65F,-13},{1.4F,0.65F,0.75F},cover);
    box(world,"north_west_cover",GreyboxPrimitiveKind::Cover,{-8,0.65F,22},{1.4F,0.65F,0.75F},cover);
    // The batched grove mesh draws the visible trunks; these matching boxes
    // supply collision and are rendered only as a resource-failure fallback.
    for (std::size_t sector = 0; sector < kFacilityGroveCenters.size(); ++sector) {
        for (std::size_t tree = 0; tree < kFacilityTreeOffsets.size(); ++tree) {
            const auto p = kFacilityGroveCenters[sector] + kFacilityTreeOffsets[tree];
            box(world, "grove_trunk_" + std::to_string(sector) + "_" + std::to_string(tree),
                GreyboxPrimitiveKind::Wall, p + Vec3{0,1.1F,0}, {0.18F,1.1F,0.18F},
                {0.30F,0.22F,0.14F,1.0F});
        }
    }
    box(world,"spawn_marker",GreyboxPrimitiveKind::Spawn,world.playerSpawn,{1.2F,0.03F,1.2F},{0.10F,0.55F,0.70F,1},false);
    return world;
}

DevTargetRangeState makeOuterFacilityTargets() {
    DevTargetRangeState range{};
    range.lanes.push_back(target("facility_center", "COURTYARD", {0,1.65F,21}));
    range.lanes.push_back(target("facility_west", "WEST EXIT", {-14,1.65F,18}));
    range.lanes.push_back(target("facility_east", "EAST ENTRY", {17,1.65F,11}));
    range.activeLaneIndex = 0;
    return range;
}
} // namespace nemisis::dev

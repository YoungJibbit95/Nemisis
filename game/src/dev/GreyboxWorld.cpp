#include "nemisis/dev/GreyboxWorld.hpp"

#include <algorithm>
#include <utility>

namespace nemisis::dev {

namespace {

using novacore::math::Vec3;

void addPrimitive(
    GreyboxWorld& world,
    std::string id,
    GreyboxPrimitiveKind kind,
    Vec3 center,
    Vec3 halfExtents,
    std::array<float, 4> color,
    bool blocksMovement,
    GreyboxRampDirection rampDirection = GreyboxRampDirection::None,
    float stepOverrideHeight = 0.0F) {
    world.primitives.push_back(GreyboxPrimitive{
        std::move(id),
        kind,
        center,
        halfExtents,
        color,
        blocksMovement,
        rampDirection,
        stepOverrideHeight,
    });
}

} // namespace

GreyboxWorld createDevRangeGreyboxWorld() {
    GreyboxWorld world{};
    world.id = "dev_range_greybox_01";
    world.boundsHalfExtents = Vec3{24.0F, 5.0F, 20.0F};
    world.playerSpawn = Vec3{0.0F, 0.0F, -12.0F};

    addPrimitive(world, "floor_main", GreyboxPrimitiveKind::Floor, Vec3{0.0F, -0.05F, 0.0F}, Vec3{24.0F, 0.05F, 20.0F}, {0.16F, 0.19F, 0.20F, 1.0F}, true);
    addPrimitive(world, "wall_back", GreyboxPrimitiveKind::Wall, Vec3{0.0F, 1.5F, 20.0F}, Vec3{24.0F, 1.5F, 0.25F}, {0.25F, 0.30F, 0.32F, 1.0F}, true);
    addPrimitive(world, "wall_left", GreyboxPrimitiveKind::Wall, Vec3{-24.0F, 1.5F, 0.0F}, Vec3{0.25F, 1.5F, 20.0F}, {0.22F, 0.27F, 0.29F, 1.0F}, true);
    addPrimitive(world, "wall_right", GreyboxPrimitiveKind::Wall, Vec3{24.0F, 1.5F, 0.0F}, Vec3{0.25F, 1.5F, 20.0F}, {0.22F, 0.27F, 0.29F, 1.0F}, true);

    addPrimitive(world, "cover_left_mid", GreyboxPrimitiveKind::Cover, Vec3{-8.0F, 0.8F, 4.0F}, Vec3{2.0F, 0.8F, 1.0F}, {0.35F, 0.41F, 0.40F, 1.0F}, true);
    addPrimitive(world, "cover_right_mid", GreyboxPrimitiveKind::Cover, Vec3{8.0F, 0.8F, 4.0F}, Vec3{2.0F, 0.8F, 1.0F}, {0.35F, 0.41F, 0.40F, 1.0F}, true);
    addPrimitive(world, "cover_center_low", GreyboxPrimitiveKind::Cover, Vec3{0.0F, 0.45F, 7.5F}, Vec3{2.8F, 0.45F, 0.8F}, {0.38F, 0.43F, 0.42F, 1.0F}, true);
    addPrimitive(world, "step_training_low", GreyboxPrimitiveKind::Cover, Vec3{-3.5F, 0.18F, -7.0F}, Vec3{1.4F, 0.18F, 1.0F}, {0.31F, 0.46F, 0.43F, 1.0F}, true, GreyboxRampDirection::None, 0.36F);
    addPrimitive(world, "ledge_training_mid", GreyboxPrimitiveKind::Ledge, Vec3{3.8F, 0.65F, -6.5F}, Vec3{1.5F, 0.65F, 1.0F}, {0.36F, 0.38F, 0.46F, 1.0F}, true);

    addPrimitive(world, "ramp_left_slide", GreyboxPrimitiveKind::Ramp, Vec3{-13.5F, 0.45F, -1.0F}, Vec3{3.0F, 0.45F, 4.0F}, {0.28F, 0.39F, 0.37F, 1.0F}, true, GreyboxRampDirection::PositiveZ);
    addPrimitive(world, "ramp_right_slide", GreyboxPrimitiveKind::Ramp, Vec3{13.5F, 0.45F, -1.0F}, Vec3{3.0F, 0.45F, 4.0F}, {0.28F, 0.39F, 0.37F, 1.0F}, true, GreyboxRampDirection::PositiveZ);
    addPrimitive(world, "wallrun_left_panel_a", GreyboxPrimitiveKind::WallRunPanel, Vec3{-18.5F, 1.45F, -5.5F}, Vec3{0.18F, 1.45F, 3.4F}, {0.08F, 0.56F, 0.68F, 1.0F}, true);
    addPrimitive(world, "wallrun_right_panel_a", GreyboxPrimitiveKind::WallRunPanel, Vec3{18.5F, 1.45F, -5.5F}, Vec3{0.18F, 1.45F, 3.4F}, {0.08F, 0.56F, 0.68F, 1.0F}, true);
    addPrimitive(world, "wallrun_left_panel_b", GreyboxPrimitiveKind::WallRunPanel, Vec3{-18.5F, 1.45F, 2.5F}, Vec3{0.18F, 1.45F, 3.4F}, {0.12F, 0.62F, 0.76F, 1.0F}, true);
    addPrimitive(world, "wallrun_right_panel_b", GreyboxPrimitiveKind::WallRunPanel, Vec3{18.5F, 1.45F, 2.5F}, Vec3{0.18F, 1.45F, 3.4F}, {0.12F, 0.62F, 0.76F, 1.0F}, true);

    addPrimitive(world, "spawn_player", GreyboxPrimitiveKind::Spawn, world.playerSpawn, Vec3{1.2F, 0.05F, 1.2F}, {0.10F, 0.55F, 0.70F, 1.0F}, false);
    addPrimitive(world, "spawn_enemy_proxy", GreyboxPrimitiveKind::Spawn, Vec3{0.0F, 0.0F, 15.0F}, Vec3{1.2F, 0.05F, 1.2F}, {0.70F, 0.20F, 0.14F, 1.0F}, false);

    addPrimitive(world, "marker_10m", GreyboxPrimitiveKind::RangeMarker, Vec3{0.0F, 0.0F, -2.0F}, Vec3{18.0F, 0.02F, 0.08F}, {0.44F, 0.50F, 0.52F, 1.0F}, false);
    addPrimitive(world, "marker_20m", GreyboxPrimitiveKind::RangeMarker, Vec3{0.0F, 0.0F, 8.0F}, Vec3{18.0F, 0.02F, 0.08F}, {0.44F, 0.50F, 0.52F, 1.0F}, false);
    addPrimitive(world, "marker_30m", GreyboxPrimitiveKind::RangeMarker, Vec3{0.0F, 0.0F, 18.0F}, Vec3{18.0F, 0.02F, 0.08F}, {0.44F, 0.50F, 0.52F, 1.0F}, false);

    addPrimitive(world, "target_debug_dummy", GreyboxPrimitiveKind::Target, Vec3{0.0F, 1.65F, 18.0F}, Vec3{0.85F, 1.65F, 0.85F}, {0.82F, 0.18F, 0.14F, 1.0F}, false);
    return world;
}

const GreyboxPrimitive* findPrimitive(const GreyboxWorld& world, const std::string& id) {
    const auto it = std::find_if(
        world.primitives.begin(),
        world.primitives.end(),
        [&id](const GreyboxPrimitive& primitive) {
            return primitive.id == id;
        });
    return it == world.primitives.end() ? nullptr : &(*it);
}

std::size_t countPrimitivesByKind(const GreyboxWorld& world, GreyboxPrimitiveKind kind) {
    return static_cast<std::size_t>(std::count_if(
        world.primitives.begin(),
        world.primitives.end(),
        [kind](const GreyboxPrimitive& primitive) {
            return primitive.kind == kind;
        }));
}

} // namespace nemisis::dev

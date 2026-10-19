#include "nemisis/dev/DevRangeRigidBodies.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace nemisis::dev {
using namespace novacore;
namespace {
constexpr math::Vec3 kPropHalfExtents{.30F, .30F, .30F};
constexpr float kPlayerRadius = .42F;
constexpr float kPlayerHeight = 1.80F;
constexpr math::Vec3 kSensorCenter{-9.25F, 1.F, -12.F};
constexpr math::Vec3 kSensorHalf{2.5F, 1.5F, 1.5F};
constexpr const char* kSensorId = "physics_fixture_sensor";
constexpr const char* kPlayerId = "local_player_prop_proxy";

// Renderer uses Ry*Rx*Rz. Preserve the physical orientation; never transform
// already-cooked geometry or convert solver coordinates at the rendering seam.
math::Vec3 renderEuler(math::Quat q) {
    constexpr float degrees = 180.F / 3.14159265358979323846F;
    const float m02 = 2 * (q.x * q.z + q.y * q.w);
    const float m12 = 2 * (q.y * q.z - q.x * q.w);
    const float m22 = 1 - 2 * (q.x * q.x + q.y * q.y);
    const float m10 = 2 * (q.x * q.y + q.z * q.w);
    const float m11 = 1 - 2 * (q.x * q.x + q.z * q.z);
    const float pitch = std::asin(std::clamp(-m12, -1.F, 1.F));
    if (std::abs(std::cos(pitch)) > 1.e-5F) {
        return {pitch * degrees, std::atan2(m02, m22) * degrees, std::atan2(m10, m11) * degrees};
    }
    const float m20 = 2 * (q.x * q.z - q.y * q.w);
    const float m00 = 1 - 2 * (q.y * q.y + q.z * q.z);
    return {pitch * degrees, std::atan2(-m20, m00) * degrees, 0};
}

math::Vec3 rotate(math::Quat q, math::Vec3 p) {
    const math::Vec3 u{q.x, q.y, q.z};
    const auto cross = [](math::Vec3 a, math::Vec3 b) {
        return math::Vec3{a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
    };
    const auto t = cross(u, p) * 2.F;
    return p + t * q.w + cross(u, t);
}
}

void DevRangeRigidBodies::setVisual(render::MeshResourceHandle handle, const assets::GltfMeshData& mesh) {
    const float limit = std::numeric_limits<float>::max();
    math::Vec3 lo{limit, limit, limit}, hi{-limit, -limit, -limit};
    for (const auto& primitive : mesh.primitives) {
        for (const auto p : primitive.positions) {
            lo = {std::min(lo.x,p.x), std::min(lo.y,p.y), std::min(lo.z,p.z)};
            hi = {std::max(hi.x,p.x), std::max(hi.y,p.y), std::max(hi.z,p.z)};
        }
    }
    const auto size = hi - lo;
    if (!handle.isValid() || !std::isfinite(size.lengthSquared()) ||
        size.x <= 0 || size.y <= 0 || size.z <= 0) {
        throw std::runtime_error("Dev Range physics requires the actual registered crate mesh");
    }
    visual_ = handle;
    visualCenter_ = (lo + hi) * .5F;
    visualScale_ = {2*kPropHalfExtents.x/size.x, 2*kPropHalfExtents.y/size.y, 2*kPropHalfExtents.z/size.z};
}

void DevRangeRigidBodies::reset(const GreyboxWorld& greybox) {
    tick_ = 0;
    lastStep_ = {};
    lastImpact_.reset();
    playerInsideSensor_ = false;
    sensorEnterCount_ = sensorExitCount_ = 0;
    bodies_.shutdown();
    statics_ = buildGreyboxPhysicsWorld(greybox);
    std::string error;
    physics::RigidBodyWorldConfig config;
    config.maxContactEvents = 256;
    if (!bodies_.initialize(config, error)) {
        throw std::runtime_error("Dev Range rigid bodies: " + error);
    }
    // A small falling stack beside the spawn, clear of existing training lanes.
    // These are labelled physics fixtures, not a claim of finished prop artwork.
    for (int i = 0; i < 6; ++i) {
        physics::RigidBodyDesc prop;
        prop.id = "physics_crate_" + std::to_string(i);
        prop.position = {-10.F + (i / 3) * 1.5F, .5F + (i % 3) * .65F, -12.F};
        prop.halfExtents = kPropHalfExtents;
        prop.massKg = 5.F;
        prop.filter = {1, 0xff};
        prop.materialId = static_cast<std::uint32_t>(DevRangePhysicsMaterial::Metal);
        prop.surfaceId = static_cast<std::uint32_t>(DevRangePhysicsSurface::Prop);
        if (!bodies_.createBody(prop, error).valid()) {
            throw std::runtime_error("Dev Range crate: " + error);
        }
    }
    physics::RigidBodyDesc player;
    player.id = kPlayerId;
    player.shape = physics::RigidBodyShape::Capsule;
    player.motion = physics::RigidBodyMotion::Kinematic;
    player.radius = kPlayerRadius;
    player.capsuleHalfHeight = kPlayerHeight * .5F - kPlayerRadius;
    player.position = greybox.playerSpawn + math::Vec3{0, kPlayerHeight * .5F, 0};
    player.filter = {2, (1 << 1) | (1 << 3)}; // Props and nonblocking sensor; no motor feedback.
    player.surfaceId = static_cast<std::uint32_t>(DevRangePhysicsSurface::Player);
    playerProxy_ = bodies_.createBody(player, error);
    if (!playerProxy_.valid()) { throw std::runtime_error("Dev Range player proxy: " + error); }
    physics::RigidBodyDesc sensor;
    sensor.id = kSensorId;
    sensor.position = kSensorCenter;
    sensor.halfExtents = kSensorHalf;
    sensor.motion = physics::RigidBodyMotion::Kinematic;
    sensor.isSensor = true;
    sensor.filter = {3, (1 << 1) | (1 << 2)};
    sensor.surfaceId = static_cast<std::uint32_t>(DevRangePhysicsSurface::Sensor);
    if (!bodies_.createBody(sensor, error).valid()) { throw std::runtime_error("Dev Range sensor: " + error); }
}

void DevRangeRigidBodies::tick(math::Vec3 feet) {
    if (!bodies_.moveKinematic(playerProxy_, feet + math::Vec3{0, kPlayerHeight * .5F, 0}, {})) {
        throw std::runtime_error("Invalid Dev Range player proxy target");
    }
    auto result = bodies_.step(statics_, tick_ + 1);
    if (!result.stepped) { throw std::runtime_error("Dev Range physics tick: " + result.error); }
    if (tick_ == 0) {
        // Canonical handles exist after synchronization. Tags are game data;
        // they never change KCC tuning or infer physics friction from artwork.
        for (const auto& body : bodies_.bodies()) {
            if (!body.canonicalCollider) { continue; }
            const auto collider = std::ranges::find(statics_.staticColliders(), body.id, &physics::StaticCollider::id);
            const bool floor = collider != statics_.staticColliders().end() && collider->kind == physics::SurfaceKind::Floor;
            if (!bodies_.setBodyTags(body.handle, static_cast<std::uint32_t>(DevRangePhysicsMaterial::Concrete),
                static_cast<std::uint32_t>(floor ? DevRangePhysicsSurface::Floor : DevRangePhysicsSurface::Obstacle))) {
                throw std::runtime_error("Dev Range canonical material tag failed");
            }
        }
    }
    tick_ = result.tick;
    for (const auto& event : result.sensorEvents) {
        if (!((event.first.id == kSensorId && event.second.id == kPlayerId) ||
              (event.second.id == kSensorId && event.first.id == kPlayerId))) { continue; }
        playerInsideSensor_ = event.phase != physics::RigidBodySensorPhase::Exit;
        sensorEnterCount_ += event.phase == physics::RigidBodySensorPhase::Enter;
        sensorExitCount_ += event.phase == physics::RigidBodySensorPhase::Exit;
    }
    lastStep_ = std::move(result);
}

bool DevRangeRigidBodies::impact(math::Vec3 origin, math::Vec3 displacement) {
    lastImpact_.reset();
    // Canonical static colliders are synchronized by the first successful step.
    if (tick_ == 0) { return false; }
    // Include static occluders in closest-hit selection; exclude player proxy.
    const auto hit = bodies_.raycast(origin, displacement, (1 << 0) | (1 << 1));
    lastImpact_ = hit;
    if (!hit || hit->canonicalCollider) { return false; }
    const auto body = bodies_.body(hit->handle);
    const float length2 = displacement.lengthSquared();
    if (!body || body->motion != physics::RigidBodyMotion::Dynamic || !std::isfinite(length2) || length2 <= 0) {
        return false;
    }
    // Developer prop-only impulse. Does not alter damage, ammo, cadence or score.
    return bodies_.applyImpulse(hit->handle, displacement * (2.F / std::sqrt(length2)), hit->position);
}

void DevRangeRigidBodies::append(render::RenderFrameInfo& frame) const {
    if (!visual_.isValid()) { throw std::runtime_error("Dev Range physics crate visual not registered"); }
    for (const auto& body : bodies_.bodies()) {
        if (body.motion != physics::RigidBodyMotion::Dynamic) { continue; }
        render::RenderMesh3D box;
        box.mesh = visual_;
        box.assetId = "map_a2_cover_crate_01";
        const math::Vec3 scaledCenter{visualCenter_.x*visualScale_.x,
            visualCenter_.y*visualScale_.y, visualCenter_.z*visualScale_.z};
        box.position = body.position - rotate(body.rotation, scaledCenter);
        box.scale = visualScale_;
        box.color = {.7F, .65F, .5F, 1.F};
        const auto angles = renderEuler(body.rotation);
        box.pitchDegrees = angles.x;
        box.yawDegrees = angles.y;
        box.rollDegrees = angles.z;
        frame.worldMeshes.push_back(box);
    }
    // Developer wireframe uses the same sensor dimensions as the native body.
    const auto color = playerInsideSensor_ ? std::array<float,4>{.2F,1.F,.5F,1.F}
                                          : std::array<float,4>{.2F,.7F,1.F,1.F};
    for (int axis=0; axis<3; ++axis) for (int a : {-1,1}) for (int b : {-1,1}) {
        math::Vec3 first, second;
        if (axis == 0) { first={-kSensorHalf.x,a*kSensorHalf.y,b*kSensorHalf.z}; second={kSensorHalf.x,first.y,first.z}; }
        if (axis == 1) { first={a*kSensorHalf.x,-kSensorHalf.y,b*kSensorHalf.z}; second={first.x,kSensorHalf.y,first.z}; }
        if (axis == 2) { first={a*kSensorHalf.x,b*kSensorHalf.y,-kSensorHalf.z}; second={first.x,first.y,kSensorHalf.z}; }
        frame.worldLines.push_back({kSensorCenter+first,kSensorCenter+second,color});
    }
    if (lastImpact_) {
        frame.worldLines.push_back({lastImpact_->position,
            lastImpact_->position+lastImpact_->normal*.3F,{1.F,.65F,.1F,1.F}});
    }
}
} // namespace nemisis::dev

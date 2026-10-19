#pragma once

#include "nemisis/dev/GreyboxWorld.hpp"
#include "novacore/physics/PhysicsSystem.hpp"
#include "novacore/physics/RigidBodyWorld.hpp"
#include "novacore/render/Renderer.hpp"

#include <optional>

namespace nemisis::dev {

enum class DevRangePhysicsMaterial : std::uint32_t { Unspecified, Concrete, Metal };
enum class DevRangePhysicsSurface : std::uint32_t { Unspecified, Floor, Prop, Obstacle, Player, Sensor };

// Opt-in local Dev Range fixture. Game owns placements/interaction policy;
// NovaCore owns the CPU solver. No dynamic state is fed into player prediction.
class DevRangeRigidBodies final {
public:
    void reset(const GreyboxWorld& greybox);
    void setVisual(novacore::render::MeshResourceHandle handle, const novacore::assets::GltfMeshData& mesh);
    void tick(novacore::math::Vec3 resolvedPlayerFeet);
    [[nodiscard]] bool impact(novacore::math::Vec3 origin, novacore::math::Vec3 displacement);
    void append(novacore::render::RenderFrameInfo& frame) const;
    [[nodiscard]] const novacore::physics::PhysicsWorld& collisionWorld() const { return statics_; }
    [[nodiscard]] const novacore::physics::RigidBodyWorld& bodies() const { return bodies_; }
    [[nodiscard]] std::uint64_t tickCount() const { return tick_; }
    [[nodiscard]] const novacore::physics::RigidBodyStepResult& lastStep() const { return lastStep_; }
    [[nodiscard]] const std::optional<novacore::physics::RigidBodyRayHit>& lastImpact() const { return lastImpact_; }
    [[nodiscard]] bool playerInsideSensor() const { return playerInsideSensor_; }
    [[nodiscard]] std::uint64_t sensorEnterCount() const { return sensorEnterCount_; }
    [[nodiscard]] std::uint64_t sensorExitCount() const { return sensorExitCount_; }
private:
    novacore::physics::PhysicsWorld statics_;
    novacore::physics::RigidBodyWorld bodies_;
    novacore::physics::RigidBodyHandle playerProxy_;
    std::uint64_t tick_ = 0;
    novacore::physics::RigidBodyStepResult lastStep_;
    std::optional<novacore::physics::RigidBodyRayHit> lastImpact_;
    bool playerInsideSensor_ = false;
    std::uint64_t sensorEnterCount_ = 0, sensorExitCount_ = 0;
    novacore::render::MeshResourceHandle visual_;
    novacore::math::Vec3 visualCenter_{};
    novacore::math::Vec3 visualScale_{1, 1, 1};
};
} // namespace nemisis::dev

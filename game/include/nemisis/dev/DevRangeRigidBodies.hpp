#pragma once

#include "nemisis/dev/GreyboxWorld.hpp"
#include "novacore/physics/PhysicsSystem.hpp"
#include "novacore/physics/RigidBodyWorld.hpp"
#include "novacore/render/Renderer.hpp"

namespace nemisis::dev {

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
private:
    novacore::physics::PhysicsWorld statics_;
    novacore::physics::RigidBodyWorld bodies_;
    novacore::physics::RigidBodyHandle playerProxy_;
    std::uint64_t tick_ = 0;
    novacore::render::MeshResourceHandle visual_;
    novacore::math::Vec3 visualCenter_{};
    novacore::math::Vec3 visualScale_{1, 1, 1};
};
} // namespace nemisis::dev

#pragma once

#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/dev/DebugTarget.hpp"
#include "nemisis/dev/GreyboxCollision.hpp"
#include "nemisis/net/LoopbackCommandBridge.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerInputCommand.hpp"
#include "nemisis/weapons/WeaponSimulation.hpp"
#include "nemisis/weapons/WeaponShot.hpp"
#include "nemisis/weapons/WeaponTypes.hpp"

#include "novacore/math/Types.hpp"
#include "novacore/platform/Input.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace nemisis::dev {

struct DevSandboxSample final {
    std::uint64_t tick = 0;
    player::PlayerInputCommand command{};
    novacore::math::Vec3 position{};
    novacore::math::Vec3 velocity{};
    movement::MovementMode movementMode = movement::MovementMode::Grounded;
    weapons::WeaponRuntimeState weapon{};
    weapons::FireResult fire{};
    weapons::ShotTraceResult shot{};
    bool hasShot = false;
    DebugTargetState target{};
    DebugTargetHitResult targetHit{};
    player::PlayerNetworkComponent network{};
    net::LoopbackBridgeStats netBridge{};
    player::PlayerViewComponent view{};
    GreyboxCollisionResult collision{};
};

class DevSandbox final {
public:
    void setEnabled(bool enabled);
    [[nodiscard]] bool enabled() const;

    void recordTick(DevSandboxSample sample);
    void onFrame(double deltaSeconds);

    [[nodiscard]] const DevSandboxSample& latestSample() const;
    [[nodiscard]] std::string latestSummary() const;
    [[nodiscard]] std::array<float, 4> clearColor() const;

private:
    bool enabled_ = true;
    bool hasSample_ = false;
    bool printedControls_ = false;
    double logAccumulatorSeconds_ = 0.0;
    DevSandboxSample latest_{};
};

} // namespace nemisis::dev

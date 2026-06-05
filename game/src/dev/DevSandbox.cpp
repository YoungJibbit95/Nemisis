#include "nemisis/dev/DevSandbox.hpp"

#include "novacore/core/Log.hpp"

#include <iomanip>
#include <sstream>
#include <string_view>
#include <utility>

namespace nemisis::dev {

namespace {

constexpr double kTelemetryIntervalSeconds = 0.5;

[[nodiscard]] std::string_view movementModeName(movement::MovementMode mode) {
    switch (mode) {
    case movement::MovementMode::Grounded:
        return "grounded";
    case movement::MovementMode::Airborne:
        return "airborne";
    case movement::MovementMode::Sliding:
        return "sliding";
    case movement::MovementMode::Dashing:
        return "dashing";
    case movement::MovementMode::WallRunning:
        return "wallrun";
    case movement::MovementMode::Mantling:
        return "mantle";
    }
    return "unknown";
}

[[nodiscard]] std::string_view deviceName(novacore::platform::InputDeviceKind device) {
    switch (device) {
    case novacore::platform::InputDeviceKind::KeyboardMouse:
        return "mkb";
    case novacore::platform::InputDeviceKind::Controller:
        return "controller";
    }
    return "unknown";
}

void appendVec3(std::ostringstream& stream, novacore::math::Vec3 value) {
    stream << '('
           << std::fixed << std::setprecision(2)
           << value.x << ", "
           << value.y << ", "
           << value.z << ')';
}

} // namespace

void DevSandbox::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool DevSandbox::enabled() const {
    return enabled_;
}

void DevSandbox::recordTick(DevSandboxSample sample) {
    latest_ = std::move(sample);
    hasSample_ = true;
}

void DevSandbox::onFrame(double deltaSeconds) {
    if (!enabled_ || !hasSample_) {
        return;
    }

    if (!printedControls_) {
        novacore::core::logInfo(
            "dev",
            "Playable dev sandbox controls: WASD/LeftStick move, Mouse/RightStick look, Space/A jump, LeftAlt/B dash, C/B slide, Shift/LeftStick sprint, MouseLeft/RT fire, MouseRight/LT ADS, R/X reload; target dummy is straight ahead");
        printedControls_ = true;
    }

    logAccumulatorSeconds_ += deltaSeconds;
    if (logAccumulatorSeconds_ < kTelemetryIntervalSeconds) {
        return;
    }
    logAccumulatorSeconds_ = 0.0;

    novacore::core::logInfo("dev", latestSummary());
}

const DevSandboxSample& DevSandbox::latestSample() const {
    return latest_;
}

std::string DevSandbox::latestSummary() const {
    std::ostringstream stream;
    stream << "tick=" << latest_.tick
           << " mode=" << movementModeName(latest_.movementMode)
           << " input=" << deviceName(latest_.command.device)
           << " move=(" << std::fixed << std::setprecision(2)
           << latest_.command.move.x << ", " << latest_.command.move.y << ')'
           << " pos=";
    appendVec3(stream, latest_.position);
    stream << " vel=";
    appendVec3(stream, latest_.velocity);
    stream << " weapon=" << latest_.weapon.weaponId
           << " ammo=" << latest_.weapon.ammoInMagazine
           << " shot=" << latest_.weapon.shotIndex
           << " reload=" << (latest_.weapon.reloading ? "yes" : "no")
           << " fire=" << (latest_.fire.fired ? "yes" : "no")
           << " dry=" << (latest_.fire.dryFire ? "yes" : "no")
           << " pending=" << latest_.network.pendingCommandCount;
    if (latest_.hasShot) {
        stream << " traceSeed=" << latest_.shot.seed
               << " traceRange=" << latest_.shot.rangeMeters
               << " traceDir=";
        appendVec3(stream, latest_.shot.direction);
    }
    stream << " targetHp=" << std::fixed << std::setprecision(1)
           << latest_.target.health
           << " targetHits=" << latest_.target.hitsTaken
           << " hit=" << (latest_.targetHit.hit ? "yes" : "no")
           << " eliminated=" << (latest_.targetHit.eliminated ? "yes" : "no")
           << " collisionHits=" << latest_.collision.hitCount
           << " blocked=" << (latest_.collision.blocked ? "yes" : "no")
           << " cmdPackets=" << latest_.netBridge.sentCommandPackets
           << " acks=" << latest_.netBridge.receivedAckPackets
           << " ackTick=" << latest_.netBridge.lastAcknowledgedTick;
    return stream.str();
}

std::array<float, 4> DevSandbox::clearColor() const {
    if (!enabled_ || !hasSample_) {
        return {0.025F, 0.035F, 0.055F, 1.0F};
    }
    if (latest_.fire.fired) {
        if (latest_.targetHit.hit) {
            return {0.22F, 0.025F, 0.025F, 1.0F};
        }
        return {0.18F, 0.085F, 0.025F, 1.0F};
    }
    if (latest_.weapon.reloading) {
        return {0.025F, 0.07F, 0.13F, 1.0F};
    }
    if (latest_.movementMode == movement::MovementMode::Dashing) {
        return {0.02F, 0.11F, 0.12F, 1.0F};
    }
    if (latest_.movementMode == movement::MovementMode::Sliding) {
        return {0.04F, 0.105F, 0.055F, 1.0F};
    }
    if (latest_.movementMode == movement::MovementMode::Airborne) {
        return {0.075F, 0.045F, 0.13F, 1.0F};
    }
    return {0.025F, 0.035F, 0.055F, 1.0F};
}

} // namespace nemisis::dev

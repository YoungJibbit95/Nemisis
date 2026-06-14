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

void appendContactRoles(std::ostringstream& stream, const GreyboxCollisionResult& collision) {
    std::size_t ground = 0;
    std::size_t step = 0;
    std::size_t wall = 0;
    std::size_t bounds = 0;
    std::size_t sweep = 0;
    for (const auto& contact : collision.contacts) {
        switch (contact.role) {
        case GreyboxContactRole::Ground:
            ++ground;
            break;
        case GreyboxContactRole::Step:
            ++step;
            break;
        case GreyboxContactRole::Wall:
            ++wall;
            break;
        case GreyboxContactRole::Bounds:
            ++bounds;
            break;
        case GreyboxContactRole::Sweep:
            ++sweep;
            break;
        }
    }

    stream << "G" << ground << "/S" << step << "/W" << wall << "/B" << bounds << "/X" << sweep;
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
            "Playable dev sandbox controls: WASD/LeftStick move, Mouse/RightStick look, Space/A jump, LeftAlt/B dash, C/B slide, Shift/LeftStick sprint, MouseLeft/RT fire, MouseRight/LT ADS, R/X reload, P/Y reset; multi-lane targets are downrange");
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
           << " tech=" << movement::movementTechCueName(movement::dominantMovementTechCue(latest_.movementTech))
           << " coyote=" << latest_.coyoteTimeRemaining
           << " jumpBuffer=" << latest_.jumpBufferRemaining
           << " doubleJumpBuffer=" << latest_.doubleJumpBufferRemaining
           << " mantleTimer=" << latest_.mantleTimeRemaining
           << " wallTimer=" << latest_.wallRunTimeRemaining
           << " ammo=" << latest_.weapon.ammoInMagazine
           << " shot=" << latest_.weapon.shotIndex
           << " burst=" << latest_.weapon.burstShotCount
           << " ads=" << std::fixed << std::setprecision(2) << latest_.weapon.adsAlpha
           << " recoil=(" << latest_.weapon.recoilPitchOffsetDegrees << ", "
           << latest_.weapon.recoilYawOffsetDegrees << ')'
           << " reload=" << (latest_.weapon.reloading ? "yes" : "no")
           << " reloadProgress=" << latest_.weapon.reloadProgress
           << " fire=" << (latest_.fire.fired ? "yes" : "no")
           << " dry=" << (latest_.fire.dryFire ? "yes" : "no")
           << " moveSpread=" << latest_.fire.movementSpreadDegrees
           << " hp=" << latest_.playerHealth.health << "/" << latest_.playerHealth.maxHealth
           << " playerDown=" << (latest_.playerHealth.eliminated ? "yes" : "no")
           << " score=" << latest_.rangeSession.score.targetsEliminated
           << " accuracy=" << devRangeAccuracy(latest_.rangeSession.score)
           << " streak=" << latest_.rangeSession.score.currentStreak
           << " targetsAlive=" << aliveTargetCount(latest_.targetRange)
           << "/" << totalTargetCount(latest_.targetRange)
           << " targetsDown=" << eliminatedTargetCount(latest_.targetRange)
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
           << " contacts=" << latest_.collision.contacts.size()
           << " contactRoles=";
    appendContactRoles(stream, latest_.collision);
    stream
           << " blocked=" << (latest_.collision.blocked ? "yes" : "no")
           << " grounded=" << (latest_.collision.grounded ? "yes" : "no")
           << " ramp=" << (latest_.collision.onRamp ? "yes" : "no")
           << " slideSurface=" << (latest_.collision.nearSlideSurface ? "yes" : "no")
           << " wallrunSurface=" << (latest_.collision.nearWallRunSurface ? "yes" : "no")
           << " swept=" << (latest_.collision.swept ? "yes" : "no")
           << " sweepHit=" << (latest_.collision.sweepHit ? latest_.collision.sweepPrimitiveId : "none")
           << " sweepFraction=" << latest_.collision.sweepFraction
           << " sweepIterations=" << latest_.collision.sweepIterations
           << " mantle=" << (latest_.collision.mantleCandidate ? latest_.collision.mantlePrimitiveId : "none")
           << " mantleHeight=" << latest_.collision.mantleHeight
           << " stepped=" << (latest_.collision.stepped ? "yes" : "no")
           << " ground=" << latest_.collision.groundPrimitiveId
           << " wall=" << latest_.collision.wallPrimitiveId
           << " normal=";
    appendVec3(stream, latest_.collision.groundNormal);
    stream << " wallNormal=";
    appendVec3(stream, latest_.collision.wallNormal);
    stream
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
    if (latest_.movementMode == movement::MovementMode::WallRunning) {
        return {0.02F, 0.09F, 0.12F, 1.0F};
    }
    if (latest_.movementMode == movement::MovementMode::Mantling) {
        return {0.06F, 0.075F, 0.025F, 1.0F};
    }
    if (latest_.movementMode == movement::MovementMode::Airborne) {
        return {0.075F, 0.045F, 0.13F, 1.0F};
    }
    return {0.025F, 0.035F, 0.055F, 1.0F};
}

} // namespace nemisis::dev

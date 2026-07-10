#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/movement/MovementConfig.hpp"

#include "novacore/core/ConfigDocument.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (condition) {
        return;
    }

    ++failures;
    std::cerr << "[fail] " << message << '\n';
}

void expectNear(float actual, float expected, float tolerance, std::string_view message) {
    const float delta = actual > expected ? actual - expected : expected - actual;
    expect(delta <= tolerance, message);
}

void testSprintReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    nemisis::player::PlayerInputCommand command{};
    command.move = novacore::math::Vec2{0.0F, 1.0F};
    command.sprintHeld = true;

    constexpr float dt = 1.0F / 60.0F;
    for (int i = 0; i < 90; ++i) {
        command.tick = static_cast<std::uint64_t>(i);
        state = movement.simulate(state, command, dt);
    }

    expect(state.position.z > movement.tuning().sprintSpeed, "accelerated sprint covers meaningful distance after 1.5s");
    expectNear(state.velocity.z, movement.tuning().sprintSpeed, 0.05F, "sprint replay converges to sprint speed");
    expect(state.mode == nemisis::movement::MovementMode::Grounded, "sprint replay stays grounded");
}

void testGroundAccelerationAndFrictionReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    nemisis::player::PlayerInputCommand command{};
    command.move = novacore::math::Vec2{0.0F, 1.0F};

    constexpr float dt = 1.0F / 60.0F;
    state = movement.simulate(state, command, dt);
    expect(state.velocity.z > 0.0F, "first movement tick accelerates forward");
    expect(state.velocity.z < movement.tuning().walkSpeed, "first movement tick does not snap to full speed");

    for (int i = 0; i < 30; ++i) {
        state = movement.simulate(state, command, dt);
    }
    expectNear(state.velocity.z, movement.tuning().walkSpeed, 0.05F, "ground acceleration reaches walk speed");

    command.move = {};
    const float speedBeforeFriction = state.velocity.z;
    state = movement.simulate(state, command, dt);
    expect(state.velocity.z < speedBeforeFriction, "friction slows player when input is released");
}

void testJumpDoubleJumpReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    nemisis::player::PlayerInputCommand command{};
    constexpr float dt = 1.0F / 60.0F;

    command.jumpPressed = true;
    state = movement.simulate(state, command, dt);
    expect(state.mode == nemisis::movement::MovementMode::Airborne, "jump enters airborne mode");
    expectNear(
        state.velocity.y,
        movement.tuning().jumpVelocity + (movement.tuning().gravity * dt),
        0.01F,
        "ground jump applies tuned 1.75x-height launch velocity before gravity integration");
    expect(!state.groundJumpAvailable, "ground jump is consumed after first jump");
    expect(state.hasDoubleJump, "double jump is available after first jump");
    expect(state.jumpBufferRemaining <= 0.0001F, "ground jump consumes jump buffer");
    expect(state.coyoteTimeRemaining <= 0.0001F, "ground jump consumes coyote time");

    command.jumpPressed = false;
    command.doubleJumpPressed = true;
    state = movement.simulate(state, command, dt);
    expect(!state.hasDoubleJump, "double jump is consumed");
    expect(!state.groundJumpAvailable, "double jump does not refresh ground jump");
    expect(state.velocity.y > 6.5F, "double jump strongly refreshes upward velocity");
    expect(state.tech.doubleJumpPlatformThrown, "double jump throws energy platform cue");
    expect(state.tech.energyPlatformSeconds > 0.0F, "double jump platform cue starts timer");
}

void testSecondSpacePressUsesAirJumpNotCoyoteGroundJump() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    nemisis::player::PlayerInputCommand command{};
    constexpr float dt = 1.0F / 60.0F;

    command.jumpPressed = true;
    command.doubleJumpPressed = true;
    state = movement.simulate(state, command, dt);
    expect(!state.groundJumpAvailable, "first space press consumes ground jump even when double-jump action shares the key");

    command = {};
    for (int i = 0; i < 4; ++i) {
        state = movement.simulate(state, command, dt);
    }

    const float upwardVelocityBeforeSecondPress = state.velocity.y;
    command.jumpPressed = true;
    command.doubleJumpPressed = true;
    state = movement.simulate(state, command, dt);

    expect(!state.hasDoubleJump, "second space press consumes air jump");
    expect(!state.groundJumpAvailable, "second space press does not reopen ground jump while airborne");
    expect(state.velocity.y > upwardVelocityBeforeSecondPress + 0.5F, "second space press strongly refreshes upward velocity");
    expect(state.tech.doubleJumpPlatformThrown, "second space press triggers energy platform cue");
}

void testBufferedDoubleJumpSurvivesCoyoteWindow() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    state.position = {0.0F, 1.2F, 0.0F};
    state.velocity = {0.0F, 1.0F, 0.0F};
    state.mode = nemisis::movement::MovementMode::Airborne;
    state.hasDoubleJump = true;
    state.groundJumpAvailable = false;
    state.coyoteTimeRemaining = 0.04F;

    nemisis::player::PlayerInputCommand command{};
    command.jumpPressed = true;
    command.doubleJumpPressed = true;

    constexpr float dt = 1.0F / 60.0F;
    state = movement.simulate(state, command, dt);
    expect(state.hasDoubleJump, "air jump is not consumed while coyote timer blocks it");
    expect(state.doubleJumpBufferRemaining > 0.0F, "double jump press is buffered through coyote window");

    command = {};
    for (int i = 0; i < 4; ++i) {
        state = movement.simulate(state, command, dt);
    }

    expect(!state.hasDoubleJump, "buffered double jump fires once coyote window expires");
    expect(state.doubleJumpBufferRemaining <= 0.0001F, "buffered double jump is consumed after firing");
    expect(state.velocity.y > 6.0F, "buffered double jump applies tuned upward impulse");
    expect(state.tech.energyPlatformSeconds > 0.0F, "buffered double jump still emits energy platform cue");
}

void testCoyoteJumpReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    state.position = {0.0F, 0.20F, 0.0F};
    state.velocity = {2.0F, -0.5F, 0.0F};
    state.mode = nemisis::movement::MovementMode::Airborne;
    state.coyoteTimeRemaining = 0.06F;

    nemisis::player::PlayerInputCommand command{};
    command.jumpPressed = true;

    const auto jumped = movement.simulate(state, command, 1.0F / 60.0F);

    expect(jumped.mode == nemisis::movement::MovementMode::Airborne, "coyote jump stays airborne after launch");
    expectNear(jumped.velocity.y, movement.tuning().jumpVelocity + (movement.tuning().gravity / 60.0F), 0.01F, "coyote jump applies launch velocity before gravity");
    expect(jumped.jumpBufferRemaining <= 0.0001F, "coyote jump consumes jump buffer");
    expect(jumped.coyoteTimeRemaining <= 0.0001F, "coyote jump consumes coyote timer");
}

void testJumpBufferReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    state.position = {0.0F, 0.08F, 0.0F};
    state.velocity = {0.0F, -2.0F, 0.0F};
    state.mode = nemisis::movement::MovementMode::Airborne;
    state.hasDoubleJump = false;

    nemisis::player::PlayerInputCommand command{};
    command.jumpPressed = true;
    state = movement.simulate(state, command, 1.0F / 60.0F);
    expect(state.jumpBufferRemaining > 0.0F, "airborne jump without double jump starts buffer");

    state.mode = nemisis::movement::MovementMode::Grounded;
    state.position.y = 0.0F;
    state.velocity.y = 0.0F;
    command.jumpPressed = false;
    state = movement.simulate(state, command, 1.0F / 60.0F);
    expect(state.mode == nemisis::movement::MovementMode::Airborne, "landing consumes buffered jump on next tick");
    expect(state.velocity.y > 0.0F, "buffered jump launches upward");
}

void testLandingConsumesBufferedJumpImmediately() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    state.position = {0.0F, 0.02F, 0.0F};
    state.velocity = {0.0F, -2.4F, 0.0F};
    state.mode = nemisis::movement::MovementMode::Airborne;
    state.hasDoubleJump = false;

    nemisis::player::PlayerInputCommand command{};
    command.jumpPressed = true;

    const auto jumped = movement.simulate(state, command, 1.0F / 60.0F);

    expect(jumped.mode == nemisis::movement::MovementMode::Airborne, "same-tick landing buffer launches instead of eating jump");
    expect(jumped.position.y <= 0.001F, "same-tick buffered launch starts from grounded foot position");
    expect(jumped.velocity.y > movement.tuning().jumpVelocity - 0.05F, "same-tick buffered launch applies full jump velocity");
    expect(jumped.jumpBufferRemaining <= 0.0001F, "same-tick buffered launch consumes jump buffer");
    expect(jumped.hasDoubleJump, "same-tick buffered launch refreshes air jump");
}

void testDashCooldownReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    nemisis::player::PlayerInputCommand command{};
    command.move = novacore::math::Vec2{1.0F, 0.0F};
    command.dashPressed = true;

    constexpr float dt = 1.0F / 60.0F;
    state = movement.simulate(state, command, dt);

    expect(state.dashCooldownRemaining > 0.0F, "dash starts cooldown");
    expect(state.velocity.x > movement.tuning().walkSpeed, "dash adds horizontal impulse");

    const float velocityAfterDash = state.velocity.x;
    state = movement.simulate(state, command, dt);
    expect(state.velocity.x <= velocityAfterDash + 0.001F, "dash cannot be repeated during cooldown");
}

void testSlideDurationAndSlideJumpReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    nemisis::player::PlayerInputCommand command{};
    command.move = novacore::math::Vec2{0.0F, 1.0F};

    constexpr float dt = 1.0F / 60.0F;
    for (int i = 0; i < 20; ++i) {
        state = movement.simulate(state, command, dt);
    }

    command.slidePressed = true;
    state = movement.simulate(state, command, dt);
    expect(state.mode == nemisis::movement::MovementMode::Sliding, "slide enters sliding mode");
    expect(state.slideTimeRemaining > 0.0F, "slide starts duration timer");
    const float slideSpeed = state.lastHorizontalSpeed;
    expect(slideSpeed > movement.tuning().sprintSpeed, "slide preserves burst speed");

    command.slidePressed = false;
    command.jumpPressed = true;
    state = movement.simulate(state, command, dt);
    expect(state.mode == nemisis::movement::MovementMode::Airborne, "jump from slide enters airborne mode");
    expect(state.velocity.y > 0.0F, "slide jump has upward velocity");
    expect(state.lastHorizontalSpeed > movement.tuning().sprintSpeed, "slide jump keeps momentum boost");
}

void testSlideBufferMakesSprintSlideReliable() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    state.position = {0.0F, 0.08F, 0.0F};
    state.velocity = {0.0F, -1.0F, movement.tuning().sprintSpeed};
    state.mode = nemisis::movement::MovementMode::Airborne;

    nemisis::player::PlayerInputCommand command{};
    command.move = novacore::math::Vec2{0.0F, 1.0F};
    command.sprintHeld = true;
    command.slidePressed = true;
    command.slideHeld = true;

    constexpr float dt = 1.0F / 60.0F;
    state = movement.simulate(state, command, dt);
    expect(state.slideBufferRemaining > 0.0F, "airborne slide input starts slide buffer");
    expect(state.mode == nemisis::movement::MovementMode::Airborne, "airborne buffered slide waits for ground");

    command.slidePressed = false;
    state.position.y = 0.0F;
    state.velocity.y = 0.0F;
    state.mode = nemisis::movement::MovementMode::Grounded;
    state = movement.simulate(state, command, dt);

    expect(state.mode == nemisis::movement::MovementMode::Sliding, "landing with buffered sprint slide enters sliding");
    expect(state.slideBufferRemaining <= 0.0001F, "slide consumes buffer");
    expect(state.slideHeldConsumed, "held slide is latched after starting slide");
}

void testWallRunContactAndWallJumpReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    state.position = {0.0F, 1.1F, 0.0F};
    state.velocity = {0.0F, -1.0F, 4.0F};
    state.mode = nemisis::movement::MovementMode::Airborne;
    state.airborneTimeSeconds = 0.25F;

    nemisis::player::PlayerInputCommand command{};
    command.move = novacore::math::Vec2{0.0F, 1.0F};

    constexpr float dt = 1.0F / 60.0F;
    state = movement.applyWallRunContact(
        state,
        command,
        nemisis::movement::WallRunContact{
            true,
            {1.0F, 0.0F, 0.0F},
            {0.0F, 0.0F, 1.0F},
        },
        dt);

    expect(state.mode == nemisis::movement::MovementMode::WallRunning, "wallrun contact enters wallrunning mode");
    expect(state.hasWallRunContact, "wallrun contact marks telemetry flag");
    expect(state.wallRunTimeRemaining > 1.0F, "wallrun contact starts timer");
    expect(state.velocity.z > 4.0F, "wallrun contact accelerates along wall tangent");
    expect(state.velocity.z < movement.tuning().wallRunSpeed, "wallrun entry avoids an instant speed snap");
    expect(state.tech.wallRunArmTriggerPressed, "wallrun contact presses arm gravity trigger cue");
    expect(state.tech.gravityInvertersActive, "wallrun contact activates gravity boot cue");

    command.jumpPressed = true;
    state = movement.simulate(state, command, dt);
    expect(state.mode == nemisis::movement::MovementMode::Airborne, "jumping from wallrun returns airborne");
    expect(state.velocity.x > movement.tuning().wallJumpImpulse - 0.1F, "wall jump pushes away from wall normal");
    expect(state.velocity.y > 4.0F, "wall jump gives upward impulse");
    expect(state.tech.wallJumpDetachTriggered, "wall jump triggers detach animation cue");
}

void testWallRunContactGraceReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    state.position = {0.0F, 1.1F, 0.0F};
    state.velocity = {0.0F, -0.8F, 6.0F};
    state.mode = nemisis::movement::MovementMode::Airborne;

    nemisis::player::PlayerInputCommand command{};
    command.move = novacore::math::Vec2{0.0F, 1.0F};

    constexpr float dt = 1.0F / 60.0F;
    state = movement.applyWallRunContact(
        state,
        command,
        nemisis::movement::WallRunContact{
            true,
            {1.0F, 0.0F, 0.0F},
            {0.0F, 0.0F, 1.0F},
        },
        dt);
    expect(state.wallRunContactGraceRemaining > 0.05F, "wallrun contact starts grace timer");

    state = movement.applyWallRunContact(
        state,
        command,
        nemisis::movement::WallRunContact{},
        dt);
    expect(state.mode == nemisis::movement::MovementMode::WallRunning, "wallrun grace survives a one-frame probe miss");
    expect(state.hasWallRunContact, "wallrun grace keeps contact telemetry active");

    command.jumpPressed = true;
    state = movement.simulate(state, command, dt);
    expect(state.wallRunDetachCooldownRemaining > 0.05F, "wall jump starts detach cooldown to prevent instant reattach jitter");
}

void testMantleCandidateReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    state.position = {3.8F, 0.72F, -7.9F};
    state.velocity = {0.0F, -1.5F, 3.0F};
    state.mode = nemisis::movement::MovementMode::Airborne;

    nemisis::player::PlayerInputCommand command{};
    command.mantlePressed = true;

    state = movement.applyMantleCandidate(
        state,
        command,
        nemisis::movement::MantleCandidate{
            true,
            {3.8F, 1.30F, -6.95F},
            {0.0F, 0.0F, -1.0F},
        },
        1.0F / 60.0F);

    expect(state.mode == nemisis::movement::MovementMode::Mantling, "mantle candidate enters mantling mode");
    expect(state.mantleTargetPosition.y > 1.25F && state.mantleTargetPosition.y < 1.35F, "mantle candidate stores target top height");
    expect(state.position.y > 0.70F && state.position.y < 0.74F, "mantle candidate keeps current position until fixed tick interpolation");
    expect(state.velocity.lengthSquared() <= 0.0001F, "mantle candidate clears movement velocity");
    expect(state.mantleTimeRemaining > 0.0F, "mantle candidate starts exit timer");
    expect(state.mantleProgressSeconds <= 0.0001F, "mantle candidate starts interpolation progress at zero");
    expect(state.hasDoubleJump, "mantle candidate refreshes double jump on ledge");
    expect(state.tech.mantleClimbTriggered, "mantle candidate triggers climb cue");
    expect(nemisis::movement::dominantMovementTechCue(state.tech) == nemisis::movement::MovementTechCue::MantleClimb,
           "mantle climb becomes dominant tech cue");

    command.mantlePressed = false;
    for (int i = 0; i < 20; ++i) {
        state = movement.simulate(state, command, 1.0F / 60.0F);
    }
    expect(state.mode == nemisis::movement::MovementMode::Grounded, "mantle exits to grounded after timer");
    expect(state.position.y > 1.25F && state.position.y < 1.35F, "mantle exit lands exactly on target top height");
    expect(state.velocity.lengthSquared() <= 0.0001F, "mantle exit clears interpolation velocity");
}

void testCrouchTransitionReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    nemisis::player::PlayerInputCommand command{};
    command.move = {0.0F, 1.0F};
    command.crouchHeld = true;
    constexpr float dt = 1.0F / 60.0F;

    state = movement.simulate(state, command, dt);
    expect(state.crouched, "crouch press starts crouch state immediately");
    expect(state.crouchFraction > 0.0F && state.crouchFraction < 1.0F, "crouch height blends over fixed ticks");
    for (int tick = 0; tick < 12; ++tick) {
        state = movement.simulate(state, command, dt);
    }
    expectNear(state.crouchFraction, 1.0F, 0.001F, "crouch transition reaches deterministic full crouch");
    expect(state.lastHorizontalSpeed <= movement.tuning().crouchSpeed + 0.05F, "crouch transition uses crouched speed cap");

    command.crouchHeld = false;
    state = movement.simulate(state, command, dt);
    expect(state.crouchFraction > 0.0F && state.crouchFraction < 1.0F, "crouch release does not pop height in one tick");
    for (int tick = 0; tick < 12; ++tick) {
        state = movement.simulate(state, command, dt);
    }
    expectNear(state.crouchFraction, 0.0F, 0.001F, "crouch exit converges on fixed-tick boundary");
    expect(!state.crouched, "crouch state clears after exit blend");
}

void testAirControlPreservesMomentumReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    state.position = {0.0F, 2.0F, 0.0F};
    state.velocity = {6.0F, 0.0F, 0.0F};
    state.mode = nemisis::movement::MovementMode::Airborne;
    state.groundJumpAvailable = false;

    nemisis::player::PlayerInputCommand command{};
    command.move = {0.0F, 1.0F};
    constexpr float dt = 1.0F / 60.0F;
    for (int tick = 0; tick < 20; ++tick) {
        state = movement.simulate(state, command, dt);
    }

    expect(state.velocity.z > 3.0F, "air acceleration adds deliberate forward authority");
    expect(state.velocity.x > 4.0F, "air control preserves meaningful inherited lateral momentum");
    expect(state.lastHorizontalSpeed <= movement.tuning().maxValidatedHorizontalSpeed + 0.001F,
           "air control remains inside validation speed envelope");
}

void testLandingAndBufferedSlideTransitionReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState hardLanding{};
    hardLanding.position = {0.0F, 0.05F, 0.0F};
    hardLanding.velocity = {0.0F, -10.0F, 0.0F};
    hardLanding.mode = nemisis::movement::MovementMode::Airborne;

    hardLanding = movement.simulate(hardLanding, {}, 1.0F / 60.0F);
    expect(hardLanding.landedThisTick, "floor crossing emits one deterministic landing tick");
    expect(hardLanding.lastLandingSpeed >= 10.0F, "landing records pre-resolution impact speed");
    expect(hardLanding.landingRecoveryRemaining > 0.0F, "hard landing starts bounded control recovery");
    hardLanding = movement.simulate(hardLanding, {}, 1.0F / 60.0F);
    expect(!hardLanding.landedThisTick, "landing pulse does not repeat while grounded");

    nemisis::movement::PlayerMovementState slideLanding{};
    slideLanding.position = {0.0F, 0.03F, 0.0F};
    slideLanding.velocity = {0.0F, -2.5F, movement.tuning().sprintSpeed};
    slideLanding.mode = nemisis::movement::MovementMode::Airborne;
    nemisis::player::PlayerInputCommand slide{};
    slide.move = {0.0F, 1.0F};
    slide.sprintHeld = true;
    slide.slidePressed = true;
    slide.slideHeld = true;
    slideLanding = movement.simulate(slideLanding, slide, 1.0F / 60.0F);
    expect(slideLanding.landedThisTick, "buffered slide landing records landing transition");
    expect(slideLanding.mode == nemisis::movement::MovementMode::Sliding,
           "buffered slide is consumed on the landing tick without grounded jitter");
    expect(slideLanding.lastHorizontalSpeed <= movement.tuning().slideMaxSpeed + 0.001F,
           "slide entry uses deterministic maximum speed");
}

void testWallRunTransitionDoesNotRetriggerReplay() {
    nemisis::movement::MovementSystem movement;
    nemisis::movement::PlayerMovementState state{};
    state.position = {0.0F, 1.2F, 0.0F};
    state.velocity = {0.0F, -0.5F, 5.0F};
    state.mode = nemisis::movement::MovementMode::Airborne;
    nemisis::player::PlayerInputCommand command{};
    command.move = {0.0F, 1.0F};
    const nemisis::movement::WallRunContact contact{true, {1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}};
    constexpr float dt = 1.0F / 60.0F;

    state = movement.applyWallRunContact(state, command, contact, dt);
    expect(state.tech.wallRunArmTriggerPressed, "first wallrun contact emits entry cue");
    state = movement.simulate(state, command, dt);
    expect(!state.tech.wallRunArmTriggerPressed, "wallrun entry cue clears on following simulation tick");
    state = movement.applyWallRunContact(state, command, contact, dt);
    expect(!state.tech.wallRunArmTriggerPressed, "persistent wall contact does not retrigger entry cue");
    expect(state.mode == nemisis::movement::MovementMode::WallRunning, "persistent contact keeps stable wallrun mode");
}

void testIdenticalCommandReplayIsStable() {
    nemisis::movement::MovementSystem movement;
    std::vector<nemisis::player::PlayerInputCommand> commands(180);
    for (std::size_t tick = 0; tick < commands.size(); ++tick) {
        auto& command = commands[tick];
        command.tick = static_cast<std::uint64_t>(tick);
        command.move = tick < 120 ? novacore::math::Vec2{0.35F, 0.94F} : novacore::math::Vec2{-0.8F, 0.2F};
        command.sprintHeld = tick < 55;
        command.jumpPressed = tick == 32;
        command.doubleJumpPressed = tick == 58;
        command.slidePressed = tick == 110;
        command.slideHeld = tick >= 110 && tick < 126;
        command.crouchHeld = tick >= 125 && tick < 145;
        command.dashPressed = tick == 150;
    }

    const auto replay = [&](nemisis::movement::PlayerMovementState state) {
        for (const auto& command : commands) {
            state = movement.simulate(state, command, 1.0F / 60.0F);
        }
        return state;
    };
    const auto first = replay({});
    const auto second = replay({});
    expectNear(first.position.x, second.position.x, 0.000001F, "replayed x position is bit-stable within float epsilon");
    expectNear(first.position.y, second.position.y, 0.000001F, "replayed y position is bit-stable within float epsilon");
    expectNear(first.position.z, second.position.z, 0.000001F, "replayed z position is bit-stable within float epsilon");
    expectNear(first.velocity.x, second.velocity.x, 0.000001F, "replayed x velocity is stable");
    expectNear(first.velocity.z, second.velocity.z, 0.000001F, "replayed z velocity is stable");
    expect(first.mode == second.mode, "replayed movement mode is stable");
    expectNear(first.crouchFraction, second.crouchFraction, 0.000001F, "replayed crouch transition is stable");
}

void testMovementTuningConfigReplay() {
    constexpr std::string_view json = R"({
        "sprint_speed": 10.0,
        "ground": { "acceleration": 55.0, "turn_acceleration": 71.0, "friction": 12.0 },
        "slide": { "max_duration": 1.1, "min_entry_speed": 4.6, "max_speed": 12.0, "steering_acceleration": 9.0, "jump_boost": 3.0, "buffer_time": 0.18 },
        "dash": { "impulse": 12.0, "cooldown": 1.25 },
        "air": { "max_speed": 8.75, "drag": 0.2, "counter_acceleration": 29.0, "control": 0.5 },
        "double_jump": { "min_airborne_time": 0.04, "buffer_time": 0.19 },
        "jump": { "coyote_time": 0.12, "buffer_time": 0.14 },
        "wall_run": {
            "speed": 9.0,
            "max_speed": 11.0,
            "acceleration": 27.0,
            "max_duration": 1.5,
            "wall_jump_impulse": 7.0,
            "min_height": 0.7,
            "probe_distance": 0.6,
            "contact_grace": 0.11,
            "detach_cooldown": 0.18
        },
        "crouch": { "enter_time": 0.07, "exit_time": 0.13 },
        "landing": { "hard_speed": 9.0, "recovery_time": 0.2, "control_scale": 0.6 }
    })";

    novacore::core::ConfigDocument document;
    const auto result = novacore::core::parseJsonConfig(json, document);
    expect(result.ok(), "movement tuning json parses");

    const auto tuning = nemisis::movement::movementTuningFromConfig(document);
    expectNear(tuning.sprintSpeed, 10.0F, 0.001F, "sprint speed loads from config");
    expectNear(tuning.groundAcceleration, 55.0F, 0.001F, "ground acceleration loads from config");
    expectNear(tuning.groundTurnAcceleration, 71.0F, 0.001F, "ground turn acceleration loads from config");
    expectNear(tuning.slideMaxDurationSeconds, 1.1F, 0.001F, "slide duration loads from config");
    expectNear(tuning.slideJumpBoost, 3.0F, 0.001F, "slide jump boost loads from config");
    expectNear(tuning.slideBufferSeconds, 0.18F, 0.001F, "slide buffer loads from config");
    expectNear(tuning.slideMinEntrySpeed, 4.6F, 0.001F, "slide minimum entry speed loads from config");
    expectNear(tuning.slideMaxSpeed, 12.0F, 0.001F, "slide maximum speed loads from config");
    expectNear(tuning.dashImpulse, 12.0F, 0.001F, "dash impulse loads from config");
    expectNear(tuning.airMaxSpeed, 8.75F, 0.001F, "air max speed loads from config");
    expectNear(tuning.airCounterAcceleration, 29.0F, 0.001F, "air counter acceleration loads from config");
    expectNear(tuning.airControl, 0.5F, 0.001F, "air control loads from config");
    expectNear(tuning.wallRunSpeed, 9.0F, 0.001F, "wall run speed loads from config");
    expectNear(tuning.wallRunMaxSpeed, 11.0F, 0.001F, "wall run max speed loads from config");
    expectNear(tuning.wallRunAcceleration, 27.0F, 0.001F, "wall run acceleration loads from config");
    expectNear(tuning.coyoteTimeSeconds, 0.12F, 0.001F, "coyote time loads from config");
    expectNear(tuning.jumpBufferSeconds, 0.14F, 0.001F, "jump buffer loads from config");
    expectNear(tuning.doubleJumpMinAirborneSeconds, 0.04F, 0.001F, "double jump min airborne time loads from config");
    expectNear(tuning.doubleJumpBufferSeconds, 0.19F, 0.001F, "double jump buffer loads from config");
    expectNear(tuning.wallRunMinHeight, 0.7F, 0.001F, "wall run min height loads from config");
    expectNear(tuning.wallRunProbeDistance, 0.6F, 0.001F, "wall run probe distance loads from config");
    expectNear(tuning.wallRunContactGraceSeconds, 0.11F, 0.001F, "wall run contact grace loads from config");
    expectNear(tuning.wallRunDetachCooldownSeconds, 0.18F, 0.001F, "wall run detach cooldown loads from config");
    expectNear(tuning.crouchEnterSeconds, 0.07F, 0.001F, "crouch enter time loads from config");
    expectNear(tuning.crouchExitSeconds, 0.13F, 0.001F, "crouch exit time loads from config");
    expectNear(tuning.hardLandingRecoverySeconds, 0.2F, 0.001F, "landing recovery loads from config");
}

} // namespace

int main() {
    testSprintReplay();
    testGroundAccelerationAndFrictionReplay();
    testJumpDoubleJumpReplay();
    testSecondSpacePressUsesAirJumpNotCoyoteGroundJump();
    testBufferedDoubleJumpSurvivesCoyoteWindow();
    testCoyoteJumpReplay();
    testJumpBufferReplay();
    testLandingConsumesBufferedJumpImmediately();
    testDashCooldownReplay();
    testSlideDurationAndSlideJumpReplay();
    testSlideBufferMakesSprintSlideReliable();
    testWallRunContactAndWallJumpReplay();
    testWallRunContactGraceReplay();
    testMantleCandidateReplay();
    testCrouchTransitionReplay();
    testAirControlPreservesMomentumReplay();
    testLandingAndBufferedSlideTransitionReplay();
    testWallRunTransitionDoesNotRetriggerReplay();
    testIdenticalCommandReplayIsStable();
    testMovementTuningConfigReplay();

    if (failures > 0) {
        std::cerr << failures << " movement replay test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis movement replay tests passed\n";
    return EXIT_SUCCESS;
}

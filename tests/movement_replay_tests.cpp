#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/movement/MovementConfig.hpp"

#include "novacore/core/ConfigDocument.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

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
    expect(state.hasDoubleJump, "double jump is available after first jump");

    command.jumpPressed = false;
    command.doubleJumpPressed = true;
    state = movement.simulate(state, command, dt);
    expect(!state.hasDoubleJump, "double jump is consumed");
    expect(state.velocity.y > 0.0F, "double jump refreshes upward velocity");
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
    expect(state.velocity.z >= movement.tuning().wallRunSpeed, "wallrun contact aligns speed to wall tangent");

    command.jumpPressed = true;
    state = movement.simulate(state, command, dt);
    expect(state.mode == nemisis::movement::MovementMode::Airborne, "jumping from wallrun returns airborne");
    expect(state.velocity.x > movement.tuning().wallJumpImpulse - 0.1F, "wall jump pushes away from wall normal");
    expect(state.velocity.y > 4.0F, "wall jump gives upward impulse");
}

void testMovementTuningConfigReplay() {
    constexpr std::string_view json = R"({
        "sprint_speed": 10.0,
        "ground": { "acceleration": 55.0, "friction": 12.0 },
        "slide": { "max_duration": 1.1, "steering_acceleration": 9.0, "jump_boost": 3.0 },
        "dash": { "impulse": 12.0, "cooldown": 1.25 },
        "air": { "max_speed": 8.75, "drag": 0.2 },
        "wall_run": { "speed": 9.0, "max_duration": 1.5, "wall_jump_impulse": 7.0 }
    })";

    novacore::core::ConfigDocument document;
    const auto result = novacore::core::parseJsonConfig(json, document);
    expect(result.ok(), "movement tuning json parses");

    const auto tuning = nemisis::movement::movementTuningFromConfig(document);
    expectNear(tuning.sprintSpeed, 10.0F, 0.001F, "sprint speed loads from config");
    expectNear(tuning.groundAcceleration, 55.0F, 0.001F, "ground acceleration loads from config");
    expectNear(tuning.slideMaxDurationSeconds, 1.1F, 0.001F, "slide duration loads from config");
    expectNear(tuning.slideJumpBoost, 3.0F, 0.001F, "slide jump boost loads from config");
    expectNear(tuning.dashImpulse, 12.0F, 0.001F, "dash impulse loads from config");
    expectNear(tuning.airMaxSpeed, 8.75F, 0.001F, "air max speed loads from config");
    expectNear(tuning.wallRunSpeed, 9.0F, 0.001F, "wall run speed loads from config");
}

} // namespace

int main() {
    testSprintReplay();
    testGroundAccelerationAndFrictionReplay();
    testJumpDoubleJumpReplay();
    testDashCooldownReplay();
    testSlideDurationAndSlideJumpReplay();
    testWallRunContactAndWallJumpReplay();
    testMovementTuningConfigReplay();

    if (failures > 0) {
        std::cerr << failures << " movement replay test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis movement replay tests passed\n";
    return EXIT_SUCCESS;
}

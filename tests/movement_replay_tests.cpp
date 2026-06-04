#include "nemisis/movement/MovementSystem.hpp"

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
    for (int i = 0; i < 60; ++i) {
        command.tick = static_cast<std::uint64_t>(i);
        state = movement.simulate(state, command, dt);
    }

    expectNear(state.position.z, movement.tuning().sprintSpeed, 0.05F, "one second sprint replay reaches expected distance");
    expect(state.mode == nemisis::movement::MovementMode::Grounded, "sprint replay stays grounded");
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

} // namespace

int main() {
    testSprintReplay();
    testJumpDoubleJumpReplay();
    testDashCooldownReplay();

    if (failures > 0) {
        std::cerr << failures << " movement replay test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis movement replay tests passed\n";
    return EXIT_SUCCESS;
}

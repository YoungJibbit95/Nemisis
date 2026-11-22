#pragma once
#include "nemisis/dev/GreyboxCollision.hpp"
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/player/PlayerView.hpp"

namespace nemisis::movement {
// Shared fixed-step traversal and contacts for the game, prediction and authority.
dev::GreyboxCollisionResult stepFacilityMotor(PlayerMovementState& state, const MovementSystem& motor,
    const dev::GreyboxWorld& world, const novacore::physics::PhysicsWorld& collision,
    player::PlayerInputCommand movementCommand, novacore::math::Vec3 viewForward, float dt);
}

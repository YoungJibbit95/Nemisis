#include "nemisis/player/PlayerView.hpp"

#include <algorithm>
#include <cmath>

namespace nemisis::player {

namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kPitchClampDegrees = 89.0F;

[[nodiscard]] float degreesToRadians(float degrees) {
    return degrees * (kPi / 180.0F);
}

[[nodiscard]] float wrapDegrees(float value) {
    while (value >= 180.0F) {
        value -= 360.0F;
    }
    while (value < -180.0F) {
        value += 360.0F;
    }
    return value;
}

[[nodiscard]] novacore::math::Vec3 normalized(novacore::math::Vec3 value) {
    const float lengthSquared = value.lengthSquared();
    if (lengthSquared <= 0.000001F) {
        return novacore::math::Vec3{0.0F, 0.0F, 1.0F};
    }

    return value * (1.0F / std::sqrt(lengthSquared));
}

} // namespace

void applyLook(PlayerViewComponent& view, novacore::math::Vec2 lookDegrees) {
    view.yawDegrees = wrapDegrees(view.yawDegrees + lookDegrees.x);
    view.pitchDegrees = std::clamp(view.pitchDegrees + lookDegrees.y, -kPitchClampDegrees, kPitchClampDegrees);
}

PlayerViewVectors viewVectors(const PlayerViewComponent& view) {
    const float yaw = degreesToRadians(view.yawDegrees);
    const float pitch = degreesToRadians(view.pitchDegrees);
    const float cosPitch = std::cos(pitch);

    PlayerViewVectors vectors{};
    vectors.forward = normalized(novacore::math::Vec3{
        std::sin(yaw) * cosPitch,
        std::sin(pitch),
        std::cos(yaw) * cosPitch,
    });
    vectors.horizontalForward = normalized(novacore::math::Vec3{
        std::sin(yaw),
        0.0F,
        std::cos(yaw),
    });
    vectors.horizontalRight = normalized(novacore::math::Vec3{
        std::cos(yaw),
        0.0F,
        -std::sin(yaw),
    });
    return vectors;
}

PlayerInputCommand commandRelativeToView(PlayerInputCommand command, const PlayerViewComponent& view) {
    const auto vectors = viewVectors(view);
    const auto worldMove =
        (vectors.horizontalRight * command.move.x) +
        (vectors.horizontalForward * command.move.y);

    command.move.x = std::clamp(worldMove.x, -1.0F, 1.0F);
    command.move.y = std::clamp(worldMove.z, -1.0F, 1.0F);
    return command;
}

} // namespace nemisis::player

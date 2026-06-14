#include "nemisis/net/SnapshotInterpolation.hpp"
#include "nemisis/net/SnapshotMessages.hpp"

#include <cmath>
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
    expect(std::abs(actual - expected) <= tolerance, message);
}

nemisis::movement::PlayerMovementState movementAt(float x, float speed) {
    nemisis::movement::PlayerMovementState movement{};
    movement.position = {x, 1.0F, 3.0F};
    movement.velocity = {speed, 0.0F, 0.5F};
    movement.wallRunNormal = {1.0F, 0.0F, 0.0F};
    movement.wallRunTangent = {0.0F, 0.0F, 1.0F};
    movement.mode = nemisis::movement::MovementMode::WallRunning;
    movement.hasDoubleJump = false;
    movement.hasWallRunContact = true;
    movement.lastHorizontalSpeed = speed;
    movement.inputMagnitude = 0.8F;
    movement.coyoteTimeRemaining = 0.03F;
    movement.jumpBufferRemaining = 0.01F;
    movement.wallRunTimeRemaining = 1.2F;
    movement.mantleTimeRemaining = 0.0F;
    movement.mantleProgressSeconds = 0.0F;
    return movement;
}

nemisis::weapons::WeaponRuntimeState weaponAt(std::uint16_t ammo, std::uint32_t shotIndex) {
    nemisis::weapons::WeaponRuntimeState weapon{};
    weapon.weaponId = "ar_01";
    weapon.ammoInMagazine = ammo;
    weapon.shotIndex = shotIndex;
    weapon.burstShotCount = 2;
    weapon.fireCooldownRemaining = 0.04F;
    weapon.reloadTimeRemaining = 0.0F;
    weapon.adsAlpha = 0.75F;
    weapon.recoilPitchOffsetDegrees = 0.8F;
    weapon.recoilYawOffsetDegrees = -0.2F;
    weapon.timeSinceLastShotSeconds = 0.02F;
    weapon.reloadProgress = 0.0F;
    weapon.reloading = false;
    return weapon;
}

nemisis::net::PlayerSnapshotState snapshotAt(
    std::uint64_t serverTick,
    std::uint64_t commandTick,
    float x,
    float yaw,
    std::uint16_t ammo) {
    auto movement = movementAt(x, 4.0F + x);
    nemisis::player::PlayerViewComponent view{};
    view.yawDegrees = yaw;
    view.pitchDegrees = -4.0F;
    auto weapon = weaponAt(ammo, static_cast<std::uint32_t>(commandTick));
    return nemisis::net::makePlayerSnapshotState(7, serverTick, commandTick, &movement, &view, &weapon);
}

void testSnapshotPacketRoundTrip() {
    nemisis::net::SnapshotPacket packet{};
    packet.serverTick = 120;
    packet.players.push_back(snapshotAt(120, 118, 2.0F, 42.0F, 27));
    packet.players.push_back(snapshotAt(120, 119, 3.0F, 46.0F, 26));

    const auto payload = nemisis::net::serializeSnapshotPacket(packet);
    const auto decoded = nemisis::net::deserializeSnapshotPacket(payload);

    expect(decoded.has_value(), "snapshot packet decodes");
    expect(decoded.has_value() && decoded->serverTick == 120, "snapshot packet keeps server tick");
    expect(decoded.has_value() && decoded->players.size() == 2, "snapshot packet keeps player count");

    const auto& player = decoded->players[1];
    expect(player.playerId == 7, "snapshot packet keeps player id");
    expect(player.acknowledgedCommandTick == 119, "snapshot packet keeps acknowledged command tick");
    expect(player.state.hasMovement && player.state.hasView && player.state.hasWeapon, "snapshot packet keeps component masks");
    expectNear(player.state.movement.position.x, 3.0F, 0.001F, "snapshot packet keeps movement position");
    expectNear(player.state.view.yawDegrees, 46.0F, 0.001F, "snapshot packet keeps view yaw");
    expect(player.state.weapon.weaponId == "ar_01", "snapshot packet keeps weapon id");
    expect(player.state.weapon.ammoInMagazine == 26, "snapshot packet keeps ammo");
}

void testSnapshotPacketRejectsInvalidPayload() {
    const std::vector<std::uint8_t> invalid{1, 2, 3, 4};
    expect(!nemisis::net::deserializeSnapshotPacket(invalid).has_value(), "snapshot packet rejects invalid payload");
}

void testPlayerSnapshotInterpolatorSamplesRemoteState() {
    nemisis::net::PlayerSnapshotInterpolator interpolator;
    expect(interpolator.store(snapshotAt(10, 8, 0.0F, 350.0F, 30)), "interpolator stores first snapshot");
    expect(interpolator.store(snapshotAt(14, 12, 8.0F, 10.0F, 28)), "interpolator stores second snapshot");

    const auto sample = interpolator.sample(12);
    expect(sample.valid, "interpolator returns valid middle sample");
    expect(sample.fromTick == 10 && sample.toTick == 14, "interpolator uses adjacent server ticks");
    expectNear(sample.alpha, 0.5F, 0.001F, "interpolator computes middle alpha");
    expectNear(sample.state.movement.position.x, 4.0F, 0.001F, "interpolator blends movement position");
    expectNear(sample.state.view.yawDegrees, 360.0F, 0.001F, "interpolator uses shortest yaw path");
    expect(sample.state.weapon.ammoInMagazine == 28, "interpolator chooses newer discrete weapon state at midpoint");

    const auto delayed = interpolator.sampleDelayed(14, 2);
    expect(delayed.valid && delayed.renderTick == 12, "interpolator samples delayed render tick");

    const auto stats = interpolator.stats();
    expect(stats.storedSnapshots == 2, "interpolator stats count snapshots");
    expect(stats.hasNewestServerTick && stats.newestServerTick == 14, "interpolator stats track newest server tick");
    expect(stats.hasOldestServerTick && stats.oldestServerTick == 10, "interpolator stats track oldest server tick");

    expect(interpolator.pruneThrough(10) == 1, "interpolator prunes old snapshots");
    expect(!interpolator.find(10).has_value(), "interpolator removes pruned snapshot");
    expect(interpolator.rollbackAfter(10) == 1, "interpolator rolls back future snapshots");
    expect(interpolator.stats().storedSnapshots == 0, "interpolator clears after prune and rollback");
}

void testSnapshotConvertsToAuthoritativePredictionState() {
    const auto snapshot = snapshotAt(33, 31, 6.0F, 90.0F, 24);
    const auto authoritative = nemisis::net::authoritativeStateFromSnapshot(snapshot);

    expect(authoritative.tick == 31, "snapshot authoritative state uses acknowledged command tick");
    expect(authoritative.hasMovement && authoritative.hasView && authoritative.hasWeapon, "snapshot authoritative state keeps masks");
    expectNear(authoritative.movement.position.x, 6.0F, 0.001F, "snapshot authoritative state keeps position");
    expect(authoritative.weapon.ammoInMagazine == 24, "snapshot authoritative state keeps weapon ammo");
}

} // namespace

int main() {
    testSnapshotPacketRoundTrip();
    testSnapshotPacketRejectsInvalidPayload();
    testPlayerSnapshotInterpolatorSamplesRemoteState();
    testSnapshotConvertsToAuthoritativePredictionState();

    if (failures > 0) {
        std::cerr << failures << " snapshot message test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis snapshot message tests passed\n";
    return EXIT_SUCCESS;
}

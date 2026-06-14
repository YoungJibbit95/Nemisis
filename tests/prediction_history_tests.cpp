#include "nemisis/net/PredictionHistory.hpp"

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

nemisis::net::PredictionSnapshot snapshotAt(std::uint64_t tick) {
    nemisis::net::PredictionSnapshot snapshot{};
    snapshot.tick = tick;
    snapshot.command.tick = tick;
    snapshot.command.move.y = 1.0F;
    snapshot.movement.position = {static_cast<float>(tick), 0.0F, 2.0F};
    snapshot.movement.velocity = {3.0F, 0.0F, 4.0F};
    snapshot.view.yawDegrees = 30.0F;
    snapshot.view.pitchDegrees = -4.0F;
    snapshot.weapon.weaponId = "ar_01";
    snapshot.weapon.ammoInMagazine = static_cast<std::uint16_t>(30U - (tick % 5U));
    snapshot.weapon.shotIndex = static_cast<std::uint32_t>(tick);
    snapshot.hasCamera = true;
    snapshot.hasAnimation = true;
    return snapshot;
}

void testPredictionHistoryRecordsAndFindsSamples() {
    nemisis::net::PlayerPredictionHistory history;

    expect(history.record(snapshotAt(10)), "prediction history records sample");
    expect(history.record(snapshotAt(11)), "prediction history records second sample");

    const auto* sample = history.find(11);
    expect(sample != nullptr, "prediction history finds sample by tick");
    expect(sample != nullptr && sample->command.tick == 11, "prediction sample keeps command tick");
    expect(sample != nullptr && sample->weapon.weaponId == "ar_01", "prediction sample keeps weapon telemetry");

    const auto stats = history.stats();
    expect(stats.storedSamples == 2, "prediction history stats count stored samples");
    expect(stats.hasRecordedTick && stats.lastRecordedTick == 11, "prediction history stats track last recorded tick");
    expect(stats.unacknowledgedTickSpan == 0, "prediction history has no ack span before acknowledgements");
}

void testPredictionHistoryPrunesAcknowledgedTicks() {
    nemisis::net::PlayerPredictionHistory history;
    for (std::uint64_t tick = 20; tick < 28; ++tick) {
        expect(history.record(snapshotAt(tick)), "prediction history records range");
    }

    const auto result = history.acknowledgeThrough(24);
    expect(result.prunedSamples == 5, "prediction history prunes acknowledged samples");
    expect(history.find(24) == nullptr, "acknowledged sample is removed");
    expect(history.find(25) != nullptr, "newer sample remains");

    const auto stats = history.stats();
    expect(stats.hasAcknowledgedTick && stats.lastAcknowledgedTick == 24, "prediction history records last ack tick");
    expect(stats.unacknowledgedTickSpan == 3, "prediction history reports unacknowledged tick span");
}

void testPredictionHistoryRejectsStaleWrappedTicks() {
    nemisis::net::PlayerPredictionHistory history;
    for (std::uint64_t tick = 1000; tick < 1000 + nemisis::net::kPredictionHistoryCapacity; ++tick) {
        expect(history.record(snapshotAt(tick)), "prediction history fills sequence window");
    }
    expect(history.record(snapshotAt(1000 + nemisis::net::kPredictionHistoryCapacity)), "prediction history wraps newest sample");
    expect(history.find(1000) == nullptr, "prediction history expires oldest sample after wrap");
    expect(!history.record(snapshotAt(999)), "prediction history rejects stale tick outside window");
}

void testPredictionHistoryComparesAuthoritativeState() {
    nemisis::net::PlayerPredictionHistory history{0.15F};
    auto predicted = snapshotAt(50);
    expect(history.record(predicted), "prediction history records predicted state");

    nemisis::net::AuthoritativePlayerState authoritative{};
    authoritative.tick = 50;
    authoritative.movement = predicted.movement;
    authoritative.movement.position.x += 0.25F;
    authoritative.view = predicted.view;
    authoritative.view.yawDegrees += 3.5F;
    authoritative.weapon = predicted.weapon;
    authoritative.weapon.ammoInMagazine -= 1;
    authoritative.hasMovement = true;
    authoritative.hasView = true;
    authoritative.hasWeapon = true;

    const auto result = history.reconcile(authoritative);
    expect(result.error.hasPrediction, "prediction reconcile finds matching prediction");
    expect(result.error.hasAuthoritativeState, "prediction reconcile sees authoritative state");
    expect(result.error.positionErrorMeters > 0.24F, "prediction reconcile measures position error");
    expect(result.error.yawErrorDegrees > 3.0F, "prediction reconcile measures view error");
    expect(result.error.ammoDelta == -1, "prediction reconcile measures weapon ammo delta");
    expect(result.error.exceedsCorrectionThreshold, "prediction reconcile flags correction threshold");
    expect(history.find(50) == nullptr, "prediction reconcile prunes acknowledged tick");

    const auto stats = history.stats();
    expect(stats.hasLatestError, "prediction stats exposes latest reconciliation error");
    expect(stats.latestError.tick == 50, "prediction stats keeps latest error tick");
}

} // namespace

int main() {
    testPredictionHistoryRecordsAndFindsSamples();
    testPredictionHistoryPrunesAcknowledgedTicks();
    testPredictionHistoryRejectsStaleWrappedTicks();
    testPredictionHistoryComparesAuthoritativeState();

    if (failures > 0) {
        std::cerr << failures << " prediction history test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis prediction history tests passed\n";
    return EXIT_SUCCESS;
}

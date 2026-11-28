#pragma once
#include "nemisis/movement/MovementSystem.hpp"
#include "nemisis/net/MatchRules.hpp"
#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/weapons/WeaponTypes.hpp"
#include "novacore/net/LocalDatagram.hpp"
#include <array>
#include <memory>
#include <optional>
#include <span>

namespace nemisis::net {
enum class MatchPhase : std::uint8_t { Waiting, Playing, Finished };
struct Duelist {
    movement::PlayerMovementState movement{};
    player::PlayerViewComponent view{};
    weapons::WeaponRuntimeState weapon{};
    weapons::WeaponRuntimeState holstered{};
    player::PlayerHealthComponent health{};
    std::uint64_t acknowledged = 0;
    std::uint32_t score = 0;
    std::uint32_t respawn = 0;
    std::uint32_t hits = 0;
    std::uint32_t spawnTick = 0;
};
struct MatchFrame {
    MatchRules rules{};
    std::array<Duelist, maxMatchPlayers> players{};
    std::array<std::uint32_t, 2> teamScores{};
    std::uint32_t tick = 0;
    std::uint32_t remaining = 3 * 60 * 60;
    MatchPhase phase = MatchPhase::Waiting;
    bool interrupted = false;
};
class PrivateMatch;
class FacilityMatchClient final {
public:
    explicit FacilityMatchClient(const MatchRules& localRules);
    explicit FacilityMatchClient(std::uint16_t port);
    explicit FacilityMatchClient(novacore::net::DatagramEndpoint remote);
    ~FacilityMatchClient();
    // An empty input polls snapshots and resends pending commands without predicting a new tick.
    void tick(std::optional<player::PlayerInputCommand> command);
    void confirmMapReady();
    [[nodiscard]] bool isPrivateMatch() const { return static_cast<bool>(local_); }
    [[nodiscard]] const dev::GreyboxWorld* negotiatedWorld() const;
    [[nodiscard]] unsigned slot() const;
    [[nodiscard]] const MatchFrame& authoritative() const;
    [[nodiscard]] const Duelist& predicted() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::unique_ptr<PrivateMatch> local_;
};
int runFacilityMatchServer(novacore::net::DatagramEndpoint bind,
    std::span<const MatchRules> rotation, bool singleRound);
int runFacilityMatchProbe(novacore::net::DatagramEndpoint remote);
std::optional<int> runMatchCommandLine(int argc, char** argv, bool dedicated,
    std::string& joinHost, std::uint16_t& joinPort);
}

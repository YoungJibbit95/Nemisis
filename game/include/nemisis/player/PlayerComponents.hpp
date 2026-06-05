#pragma once

#include <cstdint>
#include <string>

namespace nemisis::player {

using PlayerId = std::uint32_t;

struct PlayerIdentityComponent final {
    PlayerId playerId = 0;
    std::uint8_t teamIndex = 0;
    std::string displayName;
};

struct LocalPlayerComponent final {
    bool ownsInput = true;
};

struct PlayerLoadoutComponent final {
    std::string activeWeaponId;
};

struct PlayerViewComponent final {
    float yawDegrees = 0.0F;
    float pitchDegrees = 0.0F;
};

struct PlayerNetworkComponent final {
    std::uint32_t connectionId = 0;
    std::uint64_t lastProcessedCommandTick = 0;
    std::uint64_t lastServerAcknowledgedTick = 0;
    std::uint16_t pendingCommandCount = 0;
    bool authoritativeProxy = false;
};

} // namespace nemisis::player

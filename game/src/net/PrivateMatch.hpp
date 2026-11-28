#pragma once
#include "nemisis/net/FacilityMatch.hpp"

namespace nemisis::net {
class PrivateMatch final {
  public:
    explicit PrivateMatch(const MatchRules& rules);
    ~PrivateMatch();
    const MatchFrame& frame() const {
        return frame_;
    }
    const dev::GreyboxWorld& world() const {
        return arena_.world();
    }
    void ready();
    void tick(player::PlayerInputCommand input, std::uint32_t viewedTick);

  private:
    struct Navigation;
    struct Bot {
        int target = -1;
        unsigned goal = 0;
        std::uint32_t seen = 0, acquired = 0, life = 0;
        novacore::math::Vec3 lastSeen{};
    };
    player::PlayerInputCommand botInput(unsigned slot);
    MatchArena arena_;
    MatchFrame frame_;
    std::unique_ptr<Navigation> nav_;
    std::array<Bot, maxMatchPlayers> bots_{};
};
} // namespace nemisis::net

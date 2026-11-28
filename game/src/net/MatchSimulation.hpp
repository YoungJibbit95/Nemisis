#pragma once
#include "FacilityMatchCodec.hpp"

namespace nemisis::net {
void advanceMatch(MatchArena& arena, MatchFrame& frame, std::span<const MatchInput> inputs);
}

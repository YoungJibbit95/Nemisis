#include "PrivateMatch.hpp"
#include "MatchSimulation.hpp"
#include "nemisis/dev/DevTargetRange.hpp"
#include "nemisis/player/PlayerCameraRig.hpp"
#include "nemisis/player/PlayerView.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace nemisis::net {
namespace {
using Vec3 = novacore::math::Vec3;
constexpr float degrees = 180.0F / std::numbers::pi_v<float>;
float horizontal(Vec3 v) {
    return std::hypot(v.x, v.z);
}
float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3 eye(const Duelist& actor) {
    return player::gameplayEyePosition(actor.movement.position, actor.movement.mode,
                                       actor.movement.crouchFraction);
}
} // namespace

// Ground routes share four distance fields across all seats. Build them once
// from the same static collision boxes used by the character motor.
struct PrivateMatch::Navigation {
    static constexpr float spacing = 1.25F;
    int originX, originZ, width, height;
    std::vector<unsigned char> open;
    std::array<std::vector<int>, 4> distance;
    std::array<int, 4> destinations{};

    int index(int x, int z) const {
        return z * width + x;
    }
    Vec3 position(int i) const {
        return {(i % width - originX) * spacing, 0, (i / width - originZ) * spacing};
    }
    int nearest(Vec3 position) const {
        const int x = std::clamp(static_cast<int>(std::lround(position.x / spacing)) + originX, 1, width - 2);
        const int z =
            std::clamp(static_cast<int>(std::lround(position.z / spacing)) + originZ, 1, height - 2);
        if (open[index(x, z)])
            return index(x, z);
        int best = -1;
        float bestDistance = 1e9F;
        for (int dz = -5; dz <= 5; ++dz)
            for (int dx = -5; dx <= 5; ++dx) {
                if (x + dx <= 0 || x + dx >= width - 1 || z + dz <= 0 || z + dz >= height - 1)
                    continue;
                const int cell = index(x + dx, z + dz);
                const float range = (this->position(cell) - position).lengthSquared();
                if (open[cell] && range < bestDistance) {
                    best = cell;
                    bestDistance = range;
                }
            }
        return best;
    }
    Navigation(const dev::GreyboxWorld& world, const MatchFrame& frame)
        : originX(static_cast<int>(std::ceil(world.boundsHalfExtents.x / spacing))),
          originZ(static_cast<int>(std::ceil(world.boundsHalfExtents.z / spacing))), width(2 * originX + 1),
          height(2 * originZ + 1), open(width * height, 1) {
        for (int z = 0; z < height; ++z)
            for (int x = 0; x < width; ++x)
                if (x == 0 || z == 0 || x == width - 1 || z == height - 1)
                    open[index(x, z)] = 0;
        for (const auto& box : world.primitives) {
            if (!box.blocksMovement || box.center.y + box.halfExtents.y < .35F ||
                box.center.y - box.halfExtents.y > 1.7F)
                continue;
            const auto lower = [&](float v, float ext, int offset) {
                return static_cast<int>(std::ceil((v - ext - .65F) / spacing)) + offset;
            };
            const auto upper = [&](float v, float ext, int offset) {
                return static_cast<int>(std::floor((v + ext + .65F) / spacing)) + offset;
            };
            for (int z = std::max(1, lower(box.center.z, box.halfExtents.z, originZ));
                 z <= std::min(height - 2, upper(box.center.z, box.halfExtents.z, originZ)); ++z)
                for (int x = std::max(1, lower(box.center.x, box.halfExtents.x, originX));
                     x <= std::min(width - 2, upper(box.center.x, box.halfExtents.x, originX)); ++x)
                    open[index(x, z)] = 0;
        }
        const auto home = frame.players[0].movement.position, away = frame.players[1].movement.position;
        const Vec3 middle = (home + away) * .5F;
        const std::array goals{home, away, middle + Vec3{0, 0, 9}, middle - Vec3{0, 0, 9}};
        std::vector<int> queue;
        queue.reserve(open.size());
        for (unsigned g = 0; g < goals.size(); ++g) {
            auto& field = distance[g];
            field.assign(open.size(), -1);
            destinations[g] = nearest(goals[g]);
            if (destinations[g] < 0)
                continue;
            queue.clear();
            queue.push_back(destinations[g]);
            field[destinations[g]] = 0;
            for (std::size_t i = 0; i < queue.size(); ++i) {
                const int cell = queue[i];
                for (int next : {cell - 1, cell + 1, cell - width, cell + width}) {
                    if (!open[next] || field[next] >= 0)
                        continue;
                    field[next] = field[cell] + 1;
                    queue.push_back(next);
                }
            }
        }
    }
    Vec3 step(Vec3 feet, unsigned& goal, unsigned slot) const {
        const int cell = nearest(feet);
        if (cell < 0)
            return {};
        for (unsigned attempt = 0; attempt < distance.size(); ++attempt) {
            const auto& field = distance[goal];
            if (field[cell] > 1) {
                int chosen = cell;
                for (int next : {cell - width, cell + 1, cell + width, cell - 1})
                    if (field[next] >= 0 && field[next] < field[chosen])
                        chosen = next;
                // Look through a second straight grid edge, keeping corners on their centreline.
                const int ahead = chosen + (chosen - cell);
                if (ahead >= 0 && ahead < static_cast<int>(open.size()) && field[ahead] >= 0 &&
                    field[ahead] < field[chosen])
                    chosen = ahead;
                return position(chosen) - feet;
            }
            goal = goal < 2 ? 2 + (slot / 2) % 2 : goal == 2 ? 3 : 1 - slot % 2;
        }
        return {};
    }
};

PrivateMatch::PrivateMatch(const MatchRules& rules) : arena_(rules.map) {
    if (!validMatchRules(rules))
        throw std::invalid_argument("Invalid private match roster or map");
    frame_.rules = rules;
    frame_.remaining = rules.durationTicks;
    for (unsigned i = 0; i < 2 * rules.teamSize; ++i) {
        arena_.spawn(frame_.players[i], i);
        bots_[i].goal = 1 - i % 2;
    }
    nav_ = std::make_unique<Navigation>(arena_.world(), frame_);
}
PrivateMatch::~PrivateMatch() = default;
void PrivateMatch::ready() {
    if (frame_.phase == MatchPhase::Waiting)
        frame_.phase = MatchPhase::Playing;
}
player::PlayerInputCommand PrivateMatch::botInput(unsigned slot) {
    player::PlayerInputCommand input{};
    auto& bot = bots_[slot];
    const auto& self = frame_.players[slot];
    const auto tick = frame_.tick;
    if (self.health.eliminated)
        return input;
    if (bot.life != self.spawnTick) {
        bot = {};
        bot.life = self.spawnTick;
        bot.goal = 1 - slot % 2;
    }
    const auto feet = self.movement.position, origin = eye(self),
               forward = player::viewVectors(self.view).forward;
    if (tick % 6 == slot % 6) {
        int target = -1;
        float closest = 64.F;
        for (unsigned i = 1 - slot % 2; i < 2 * frame_.rules.teamSize; i += 2) {
            const auto& opponent = frame_.players[i];
            if (opponent.health.eliminated)
                continue;
            const Vec3 torso = eye(opponent) - Vec3{0, .36F, 0};
            const Vec3 offset = torso - origin;
            const float range = novacore::math::length(offset);
            if (range < .01F || range >= closest)
                continue;
            const auto direction = offset * (1.F / range);
            if (range > 5.F && dot(direction, forward) < .25F)
                continue;
            weapons::ShotTraceResult ray{};
            ray.origin = origin;
            ray.direction = direction;
            ray.rangeMeters = range;
            if (dev::traceDevRangeShot({}, ray, arena_.world().primitives).distanceMeters + .02F < range)
                continue;
            target = static_cast<int>(i);
            closest = range;
            bot.lastSeen = torso;
        }
        if (target >= 0) {
            if (target != bot.target)
                bot.acquired = tick;
            bot.seen = tick;
        }
        bot.target = target;
    }
    const bool engaged = bot.target >= 0 && !frame_.players[bot.target].health.eliminated;
    const bool searching = bot.seen && tick - bot.seen < 45;
    Vec3 travel = nav_->step(feet, bot.goal, slot);
    if (engaged) {
        const auto toward = bot.lastSeen - origin;
        const float range = horizontal(toward);
        const float side = ((tick / 54 + slot) % 2 ? 1.F : -1.F);
        travel = range > .1F ? Vec3{toward.z * side, 0, -toward.x * side} * (.52F / range) : Vec3{};
        if (range > 19.F)
            travel = travel + toward * (.45F / range);
    }
    const Vec3 aim = engaged || searching ? bot.lastSeen - origin : travel;
    const float desiredYaw = std::atan2(aim.x, aim.z) * degrees;
    const float yawError = std::remainder(self.view.yawDegrees - desiredYaw, 360.F);
    const float pitchError =
        (engaged || searching ? std::atan2(aim.y, horizontal(aim)) * degrees : 0.F) - self.view.pitchDegrees;
    input.look = {std::clamp(yawError * .32F, -4.3F, 4.3F), std::clamp(pitchError * .32F, -2.5F, 2.5F)};
    auto view = self.view;
    player::applyLook(view, input.look);
    const auto basis = player::viewVectors(view);
    const float travelScale = 1.F / std::max(1.F, horizontal(travel));
    travel.y = 0;
    input.move = {dot(travel, basis.horizontalRight) * travelScale,
                  dot(travel, basis.horizontalForward) * travelScale};
    input.adsHeld = engaged;
    input.sprintHeld = !engaged && !searching && std::abs(yawError) < 35.F;
    input.fireHeld = engaged && tick - bot.acquired >= 12 + slot % 5 && std::abs(yawError) < 3.F &&
                     std::abs(pitchError) < 3.F && (tick + 13 * slot) % 42 < 26;
    input.reloadPressed = !self.weapon.reloading &&
                          (self.weapon.ammoInMagazine == 0 || (!engaged && self.weapon.ammoInMagazine < 8));
    return input;
}
void PrivateMatch::tick(player::PlayerInputCommand input, std::uint32_t viewedTick) {
    if (frame_.phase != MatchPhase::Playing)
        return;
    std::array<MatchInput, maxMatchPlayers> inputs{};
    const unsigned count = 2 * frame_.rules.teamSize;
    for (unsigned slot = 0; slot < count; ++slot) {
        auto command = slot == 0 ? input : botInput(slot);
        sanitizeMatchCommand(command);
        command.tick = frame_.tick + 1;
        inputs[slot] = {command, slot == 0 ? viewedTick : frame_.tick};
    }
    advanceMatch(arena_, frame_, std::span(inputs).first(count));
}
} // namespace nemisis::net

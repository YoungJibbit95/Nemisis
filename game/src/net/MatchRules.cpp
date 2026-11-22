#include "nemisis/net/MatchRules.hpp"

#include "nemisis/dev/OuterFacility.hpp"
#include "nemisis/movement/FacilityMotor.hpp"
#include "nemisis/net/FacilityMatch.hpp"
#include "nemisis/player/PlayerCameraRig.hpp"
#include "nemisis/player/PlayerHealth.hpp"
#include "nemisis/weapons/WeaponMetrics.hpp"
#include "nemisis/weapons/WeaponSimulation.hpp"
#include "nemisis/weapons/WeaponSystem.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace nemisis::net {
namespace {
constexpr float stepSeconds = 1.0F / 60.0F;
}

bool validMatchRules(const MatchRules& rules) {
    if (rules.map != MatchMap::OuterFacility && rules.map != MatchMap::DevRange) return false;
    if (!rules.scoreLimit || rules.scoreLimit > 10'000 || !rules.durationTicks || rules.durationTicks > 60 * 60 * 60)
        return false;
    switch (rules.mode) {
    case MatchMode::Duel: return rules.teamSize == 1;
    case MatchMode::TeamDeathmatch: return rules.teamSize >= 1 && rules.teamSize <= maxMatchPlayers / 2;
    }
    return false;
}

MatchArena::MatchArena(MatchMap map)
    : map_(map), world_(map == MatchMap::DevRange ? dev::createDevRangeGreyboxWorld() : dev::createOuterFacilityWorld()),
      collision_(dev::buildGreyboxPhysicsWorld(world_)) {
    weapons::WeaponSystem registry;
    registry.registerPrototypeLoadout();
    rifle_ = *registry.findWeapon("ar_01");
    sidearm_ = *registry.findWeapon("sidearm_01");
}

void MatchArena::spawn(Duelist& actor, unsigned slot) const {
    constexpr std::array<float, maxMatchPlayers / 2> lateralOffsets{0, -2, 2, -4, 4, 6};
    const unsigned team = slot % 2;
    actor.movement = {};
    actor.movement.position = world_.playerSpawn;
    actor.movement.position.x += lateralOffsets.at(slot / 2);
    if (team == 1) actor.movement.position.z += map_ == MatchMap::DevRange ? 27.0F : 10.0F;
    actor.view = {team == 1 ? 180.0F : 0.0F, -1.0F};
    actor.weapon = {};
    actor.weapon.weaponId = rifle_.id;
    actor.weapon.ammoInMagazine = rifle_.magazineSize;
    actor.holstered = {};
    actor.holstered.weaponId = sidearm_.id;
    actor.holstered.ammoInMagazine = sidearm_.magazineSize;
    actor.respawn = 0;
    player::resetHealth(actor.health);
}

std::optional<weapons::ShotTraceResult> MatchArena::simulate(Duelist& actor, const player::PlayerInputCommand& input) const {
    if (actor.health.eliminated) return {};
    player::applyLook(actor.view, input.look);
    const auto forward = player::viewVectors(actor.view).forward;
    movement::stepFacilityMotor(actor.movement, motor_, world_, collision_,
        player::commandRelativeToView(input, actor.view), forward, stepSeconds);
    const auto& motion = actor.movement;
    const auto speed = std::hypot(motion.velocity.x, motion.velocity.z);
    const bool airborne = motion.mode == movement::MovementMode::Airborne;
    const bool sprint = input.sprintHeld || input.tacticalSprintHeld;
    const bool switchToPrimary = input.switchWeaponPrimaryPressed && actor.weapon.weaponId != rifle_.id;
    const bool switchToSidearm = input.switchWeaponSidearmPressed && actor.weapon.weaponId != sidearm_.id;
    if (switchToPrimary || switchToSidearm) {
        std::swap(actor.weapon, actor.holstered);
        actor.weapon.adsAlpha = 0.0F;
    }
    const auto& weapon = actor.weapon.weaponId == rifle_.id ? rifle_ : sidearm_;
    const auto fire = weapons::simulateWeaponTick(weapon, actor.weapon,
        {input.fireHeld, input.reloadPressed || input.reloadHeld, input.adsHeld, airborne, sprint, speed, stepSeconds});
    if (!fire.fired) return {};
    return weapons::buildShotTrace(weapon, {
        player::gameplayEyePosition(motion.position, motion.mode, motion.crouchFraction), forward,
        static_cast<std::uint32_t>(input.tick), fire.shotIndex, speed, fire.adsAlpha,
        fire.recoilPitchOffsetDegrees, fire.recoilYawOffsetDegrees, input.adsHeld, airborne, sprint});
}

void MatchArena::record(const MatchFrame& frame) {
    auto& sample = history_[frame.tick % history_.size()];
    sample.tick = frame.tick;
    sample.recorded = true;
    for (unsigned i = 0; i < 2 * frame.rules.teamSize; ++i) {
        const auto& actor = frame.players[i];
        sample.poses[i] = {actor.movement.position, actor.view.yawDegrees,
            1.0F - 0.28F * std::clamp(actor.movement.crouchFraction, 0.0F, 1.0F),
            actor.spawnTick, !actor.health.eliminated};
    }
}

void MatchArena::resolveShot(MatchFrame& frame, unsigned shooter, const weapons::ShotTraceResult& shot,
                             std::uint32_t viewedTick) const {
    const auto playerCount = frame.rules.teamSize * 2;
    if (shooter >= playerCount) return;
    // An old or future report cannot extend the server's rewind window. A shot
    // from a newly spawned shooter also cannot interact with its previous life.
    if (viewedTick > frame.tick || frame.tick - viewedTick > maxRewindTicks ||
        viewedTick < frame.players[shooter].spawnTick) viewedTick = frame.tick;
    const auto& requested = history_[viewedTick % history_.size()];
    const auto& sample = requested.recorded && requested.tick == viewedTick
        ? requested : history_[frame.tick % history_.size()];
    if (!sample.recorded || (sample.tick != viewedTick && sample.tick != frame.tick)) return;

    // Trace the static map once. Transform the ray into each historical model's
    // yaw and crouch scale; retain the ray parameter in world metres.
    auto nearestDistance = dev::traceDevRangeShot({}, shot, world_.primitives).distanceMeters;
    std::optional<unsigned> nearestPlayer;
    player::DamageZone nearestZone{};
    dev::DebugTargetState humanoid;
    humanoid.position = {0, 1.65F, 0};
    humanoid.humanoidHitZones = true;
    for (unsigned i = 0; i < playerCount; ++i) {
        const auto& pose = sample.poses[i];
        if (i == shooter || !pose.alive) continue;
        const float yaw = pose.yawDegrees * std::numbers::pi_v<float> / 180.0F;
        const float cosine = std::cos(yaw);
        const float sine = std::sin(yaw);
        const auto local = [&](novacore::math::Vec3 v) -> novacore::math::Vec3 {
            return {cosine * v.x - sine * v.z, v.y / pose.heightScale, sine * v.x + cosine * v.z};
        };
        auto localShot = shot;
        localShot.origin = local(shot.origin - pose.feet);
        localShot.direction = local(shot.direction);
        localShot.rangeMeters = nearestDistance;
        const auto hit = dev::traceShotToDebugTarget(humanoid, localShot);
        if (!hit.hit || hit.distanceMeters >= nearestDistance) continue;
        nearestDistance = hit.distanceMeters;
        nearestPlayer = i;
        nearestZone = hit.zone;
    }
    if (!nearestPlayer || *nearestPlayer % 2 == shooter % 2) return;
    auto& target = frame.players[*nearestPlayer];
    if (target.health.eliminated || target.spawnTick != sample.poses[*nearestPlayer].spawnTick) return;
    auto& attacker = frame.players[shooter];
    const auto& weapon = attacker.weapon.weaponId == rifle_.id ? rifle_ : sidearm_;
    const auto damage = player::applyDamage(target.health, {
        weapons::damageAtDistance(weapon, nearestDistance, false), nearestZone,
        weapon.damage.headMultiplier, 0.85F, shooter + 1, frame.tick});
    if (damage.damageApplied > 0) ++attacker.hits;
    if (damage.eliminated) {
        ++attacker.score;
        ++frame.teamScores[shooter % 2];
        target.respawn = 90;
    }
}

} // namespace nemisis::net

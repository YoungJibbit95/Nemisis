#include "nemisis/GameApp.hpp"
#include "nemisis/player/PlayerView.hpp"
#include "nemisis/ui/UiCanvas.hpp"
#include "novacore/ecs/Components.hpp"
#include "novacore/core/Log.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include "novacore/net/LocalDatagram.hpp"

namespace nemisis::game {
namespace {
const std::string remoteMeshId = "match_remote_operator";
}

void GameApp::leaveFacilityMatch() {
    facilityMatch_.reset();
    if (const auto entry = devMeshResources_.find(remoteMeshId); entry != devMeshResources_.end()) {
        renderer_.releaseMeshResource(entry->second);
        devMeshResources_.erase(entry);
    }
    remoteMatchAnimator_.reset();
    remoteAnimationState_ = {};
    remotePoseElapsed_ = 0;
    remoteVelocity_ = {};
    confirmedHitUntil_ = 0;
    options_.localMatchPort = 0;
    gameplayInputBuffer_.clear();
}

void GameApp::tickFacilityMatch(player::PlayerInputCommand command) {
    if (!facilityMatch_) {
        // Load the two supported maps' shared asset superset before admission.
        // A slower disk must not stall network heartbeats after a map is announced.
        loadPlayableResources(ui::GameScreen::DevRange);
        const auto endpoint = novacore::net::DatagramEndpoint::parse(options_.matchHost, options_.localMatchPort);
        if (!endpoint) throw std::runtime_error("Invalid match host");
        facilityMatch_ = std::make_unique<net::FacilityMatchClient>(*endpoint);
    }
    const auto oldState = facilityMatch_->authoritative().players[facilityMatch_->slot()];
    const auto& before = facilityMatch_->authoritative();
    const auto oldTick = before.tick;
    const auto oldPhase = before.phase;
    const auto& priorOpponent = before.players[1 - facilityMatch_->slot() % 2];
    const auto oldPosition = priorOpponent.movement.position;
    const auto oldSpawn = priorOpponent.spawnTick;
    facilityMatch_->tick(command);
    const auto& snapshot = facilityMatch_->authoritative();
    syncMatchMap(snapshot.rules.map);
    if (snapshot.phase == net::MatchPhase::Waiting) return;
    const auto& opponent = snapshot.players[1 - facilityMatch_->slot() % 2];
    if (oldPhase != net::MatchPhase::Playing || opponent.spawnTick != oldSpawn || opponent.health.eliminated) {
        remoteVelocity_ = {};
    } else if (snapshot.tick > oldTick) {
        // Compact remote snapshots carry feet, facing, stance and magazine.
        // Derive presentation from consecutive confirmed server ticks.
        remoteVelocity_ = (opponent.movement.position-oldPosition) * (60.0F/static_cast<float>(snapshot.tick-oldTick));
    }
    const auto& actor = facilityMatch_->predicted();
    const auto& motion = actor.movement;
    *world_.getComponent<movement::PlayerMovementState>(localPlayerEntity_) = motion;
    *world_.getComponent<player::PlayerViewComponent>(localPlayerEntity_) = actor.view;
    *world_.getComponent<weapons::WeaponRuntimeState>(localPlayerEntity_) = actor.weapon;
    *world_.getComponent<player::PlayerHealthComponent>(localPlayerEntity_) = actor.health;
    world_.getComponent<novacore::ecs::TransformComponent>(localPlayerEntity_)->position = motion.position;
    if (oldState.health.eliminated && !actor.health.eliminated) player::resetCameraRig(cameraRig_);
    const auto camera = player::updateCameraRig(cameraRig_,{motion.position,motion.velocity,motion.mode,
        actor.view,actor.weapon,command.adsHeld,command.sprintHeld,command.tacticalSprintHeld,motion.crouched,
        1.0F/60.0F});
    const auto facing = player::viewVectors(actor.view);
    player::CharacterAnimationInput animation{motion.velocity,motion.mode,motion.tech,actor.weapon,
        camera.speed01,camera.rollDegrees,0.0F,1.0F/60.0F,motion.hasWallRunContact,command.adsHeld,
        command.sprintHeld,command.tacticalSprintHeld,motion.crouched,facing.horizontalForward,facing.horizontalRight};
    animation.aimPitchDegrees = actor.view.pitchDegrees;
    latestCharacterAnimationFrame_ = player::updateCharacterAnimation(characterAnimation_,animation);
    hasCharacterAnimationFrame_ = true;
    pendingAnimationSeconds_ += 1.0F/60.0F;
    const auto& confirmed = snapshot.players[facilityMatch_->slot()];
    if (confirmed.hits > oldState.hits) confirmedHitUntil_ = snapshot.tick + 14;
}

void GameApp::updateRemoteMatchPose(float deltaSeconds) {
    const auto& snapshot = facilityMatch_->authoritative();
    const auto& other = snapshot.players[1 - facilityMatch_->slot() % 2];
    if (snapshot.phase == net::MatchPhase::Waiting || other.health.eliminated) return;
    const auto& local = facilityMatch_->predicted();
    const auto offset = other.movement.position-local.movement.position;
    const auto look = player::viewVectors(local.view).forward;
    const float facing = offset.x*look.x + offset.y*look.y + offset.z*look.z;
    if (facing < -1.0F) return;
    remotePoseElapsed_ += std::clamp(deltaSeconds,0.0F,0.1F);
    const float cadence = offset.lengthSquared() > 30*30 ? 1.0F/15 : 1.0F/30;
    if (remotePoseElapsed_ < cadence) return;
    const float elapsed = std::min(remotePoseElapsed_,0.1F);
    remotePoseElapsed_ = 0;
    if (!remoteMatchAnimator_) {
        const auto* model = devAssetBindings_.meshCatalog().findByAssetId(dev::kLocalPlayerBodyAssetId);
        if (!model || !model->meshData) return;
        remoteMatchAnimator_ = std::make_unique<player::PlayerSkeletalAnimator>();
        if (!remoteMatchAnimator_->initialize(*model->meshData)) {
            novacore::core::logWarning("match","Remote operator clip initialization failed");
            return;
        }
        const auto handle = renderer_.registerMeshResource(remoteMeshId,*remoteMatchAnimator_->skinnedMesh(),
            novacore::render::MeshResourceUsage::DynamicVertices);
        if (handle.isValid()) devMeshResources_.emplace(remoteMeshId,handle);
    }
    const auto mesh = devMeshResources_.find(remoteMeshId);
    if (mesh == devMeshResources_.end()) return;
    const auto& motion = other.movement;
    const auto forward = player::viewVectors(other.view);
    player::CharacterAnimationInput input{};
    input.velocity = remoteVelocity_;
    input.movementMode = std::abs(remoteVelocity_.y) > 0.8F ? movement::MovementMode::Airborne : movement::MovementMode::Grounded;
    input.movementTech = motion.tech;
    input.weapon = other.weapon;
    input.speed01 = std::clamp(player::cameraRigHorizontalSpeed(remoteVelocity_)/movement_.tuning().tacticalSprintSpeed,0.0F,1.0F);
    input.fixedDeltaSeconds = elapsed;
    input.hasWallRunContact = motion.hasWallRunContact;
    input.adsHeld = other.weapon.adsAlpha > 0.5F;
    input.sprintHeld = input.speed01 > 0.7F;
    input.crouchHeld = motion.crouchFraction > 0.5F;
    if (input.crouchHeld && input.speed01 > 0.5F && input.movementMode == movement::MovementMode::Grounded)
        input.movementMode = movement::MovementMode::Sliding;
    input.facingForward = forward.horizontalForward;
    input.facingRight = forward.horizontalRight;
    input.aimPitchDegrees = other.view.pitchDegrees;
    const auto pose = player::updateCharacterAnimation(remoteAnimationState_,input);
    const bool initial = remoteMatchAnimator_->stats().evaluatedFrameCount == 0;
    if (remoteMatchAnimator_->update(pose,elapsed) &&
        renderer_.updateMeshResourceVertices(mesh->second,*remoteMatchAnimator_->skinnedMesh()) && initial) {
        const auto& stats = remoteMatchAnimator_->stats();
        novacore::core::logInfo("match","Remote skeletal upload: clip=" + stats.currentClip + " joints=" +
            std::to_string(stats.jointCount) + " vertices=" + std::to_string(stats.skinnedVertexCount));
    }
}

void GameApp::drawFacilityMatch(novacore::render::RenderFrameInfo& frame) const {
    const auto& snapshot = facilityMatch_->authoritative();
    const auto index = facilityMatch_->slot();
    const auto& me = snapshot.players[index];
    const auto team = index % 2;
    const auto ownScore = snapshot.teamScores[team];
    const auto otherScore = snapshot.teamScores[1-team];
    const auto& predicted = facilityMatch_->predicted();
    ui::UiCanvas hud;
    hud.beginFrame();
    constexpr ui::UiColor white{0.95F,0.96F,0.96F,1};
    constexpr ui::UiColor amber{1,0.69F,0.34F,1};
    hud.roundedRect({445,18,390,55},6,{0.04F,0.07F,0.09F,0.89F});
    if (snapshot.phase == net::MatchPhase::Waiting) {
        hud.text(467,34,2,white,"WAITING FOR OPERATORS");
        hud.appendToRenderFrame(frame);
        return;
    }
    for (unsigned i = 0; i < 2 * snapshot.rules.teamSize; ++i) {
        if (i == index) continue;
        const auto& other = snapshot.players[i];
        if (other.health.eliminated) continue;
        const auto position = other.movement.position;
        const float heightScale = 1.0F - 0.28F * other.movement.crouchFraction;
        const auto color = i % 2 == team ? std::array{0.35F,0.65F,0.90F,1.0F} :
            std::array{0.85F,0.52F,0.36F,1.0F};
        auto mesh = i == 1 - index % 2 ? devMeshResources_.find(remoteMeshId) : devMeshResources_.end();
        if (mesh == devMeshResources_.end()) mesh = devMeshResources_.find(std::string(dev::kLocalPlayerBodyAssetId));
        if (mesh != devMeshResources_.end()) {
            novacore::render::RenderMesh3D model{};
            model.mesh = mesh->second;
            model.assetId = std::string(dev::kLocalPlayerBodyAssetId);
            model.position = position;
            model.scale = {0.92F,0.92F*heightScale,0.92F};
            model.yawDegrees = other.view.yawDegrees;
            model.color = color;
            frame.worldMeshes.push_back(std::move(model));
        } else {
            frame.worldBoxes.push_back({position + novacore::math::Vec3{0,0.90F*heightScale,0},
                {0.30F,0.90F*heightScale,0.21F},color});
        }
    }
    const auto seconds = (snapshot.remaining + 59) / 60;
    const auto time = std::to_string(seconds/60) + (seconds%60 < 10 ? ":0" : ":") + std::to_string(seconds%60);
    hud.text(470,34,2.0F,white,(snapshot.rules.mode == net::MatchMode::Duel ? "DUEL   " : "TEAM   ") + std::to_string(ownScore) + "  /  " +
        std::to_string(otherScore) + "       " + time);
    hud.roundedRect({47,637,248,50},5,{0.04F,0.07F,0.09F,0.89F});
    hud.text(62,651,2.2F,white,"HP  " + std::to_string(static_cast<unsigned>(me.health.health)));
    hud.roundedRect({982,637,251,50},5,{0.04F,0.07F,0.09F,0.89F});
    hud.text(999,651,2.2F,white,predicted.weapon.reloading ? "RELOADING" :
        (predicted.weapon.weaponId == "ar_01" ? "AR   " : "PISTOL   ") + std::to_string(predicted.weapon.ammoInMagazine) +
        (predicted.weapon.weaponId == "ar_01" ? " / 30" : " / 15"));
    if (snapshot.phase == net::MatchPhase::Finished) {
        const auto result = snapshot.interrupted ? "MATCH INTERRUPTED" :
            ownScore == otherScore ? "DRAW" : ownScore > otherScore ? "VICTORY" : "DEFEAT";
        hud.outlinedText(555,290,4,amber,result);
    } else if (me.health.eliminated) {
        hud.shadowedText(510,290,2.5F,amber,"REDEPLOY  " + std::to_string((me.respawn+59)/60));
    } else {
        hud.crosshair(640,360,4,7,white);
        if (snapshot.tick < confirmedHitUntil_) {
            for (const float sx : {-1.0F,1.0F})
                for (const float sy : {-1.0F,1.0F})
                    hud.line(640+sx*6,360+sy*6,640+sx*11,360+sy*11,amber);
            if (snapshot.rules.mode == net::MatchMode::Duel) {
                hud.shadowedText(603,389,1.3F,amber,"ENEMY  " +
                    std::to_string(static_cast<unsigned>(std::ceil(snapshot.players[1-team].health.health))) + " HP");
            } else {
                hud.shadowedText(590,389,1.3F,amber,"HIT CONFIRMED");
            }
        }
    }
    hud.appendToRenderFrame(frame);
}
}

#include "nemisis/player/PlayerSpawn.hpp"

#include "nemisis/movement/MovementSystem.hpp"

#include "novacore/ecs/Components.hpp"

#include <string>

namespace nemisis::player {

novacore::ecs::EntityId spawnLocalPlayer(
    novacore::ecs::World& world,
    const LocalPlayerSpawnDesc& desc,
    const weapons::WeaponDefinition* activeWeapon) {
    const auto entity = world.createEntity();
    const std::string activeWeaponId = activeWeapon != nullptr ? activeWeapon->id : desc.activeWeaponId;

    novacore::ecs::TransformComponent transform{};
    transform.position = desc.position;

    world.addComponent(entity, novacore::ecs::NameComponent{"local_player"});
    world.addComponent(entity, transform);
    world.addComponent(entity, PlayerIdentityComponent{desc.playerId, desc.teamIndex, desc.displayName});
    world.addComponent(entity, LocalPlayerComponent{true});
    world.addComponent(entity, PlayerLoadoutComponent{activeWeaponId});
    world.addComponent(entity, PlayerViewComponent{});
    world.addComponent(entity, PlayerNetworkComponent{});
    world.addComponent(entity, movement::PlayerMovementState{});

    weapons::WeaponRuntimeState weaponState{};
    weaponState.weaponId = activeWeaponId;
    if (activeWeapon != nullptr) {
        weaponState.weaponId = activeWeapon->id;
        weaponState.ammoInMagazine = activeWeapon->magazineSize;
    }
    world.addComponent(entity, weaponState);

    return entity;
}

} // namespace nemisis::player

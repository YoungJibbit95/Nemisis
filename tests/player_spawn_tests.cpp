#include "nemisis/player/PlayerComponents.hpp"
#include "nemisis/player/PlayerSpawn.hpp"

#include "nemisis/movement/MovementSystem.hpp"

#include "novacore/ecs/Components.hpp"

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

nemisis::weapons::WeaponDefinition testWeapon() {
    nemisis::weapons::WeaponDefinition weapon{};
    weapon.id = "spawn_ar";
    weapon.displayName = "Spawn AR";
    weapon.magazineSize = 30;
    weapon.fireRateRpm = 720;
    weapon.reloadTimeSeconds = 1.9F;
    return weapon;
}

void testSpawnCreatesExpectedPlayerComponents() {
    novacore::ecs::World world;
    const auto weapon = testWeapon();

    nemisis::player::LocalPlayerSpawnDesc desc{};
    desc.playerId = 7;
    desc.teamIndex = 1;
    desc.displayName = "Test Local";
    desc.activeWeaponId = weapon.id;
    desc.position = novacore::math::Vec3{1.0F, 2.0F, 3.0F};

    const auto entity = nemisis::player::spawnLocalPlayer(world, desc, &weapon);

    expect(world.isAlive(entity), "spawned local player entity is alive");
    expect(world.getComponent<nemisis::player::LocalPlayerComponent>(entity) != nullptr, "local player component exists");
    expect(world.getComponent<nemisis::player::PlayerViewComponent>(entity) != nullptr, "view component exists");
    expect(world.getComponent<nemisis::movement::PlayerMovementState>(entity) != nullptr, "movement state component exists");

    const auto* identity = world.getComponent<nemisis::player::PlayerIdentityComponent>(entity);
    expect(identity != nullptr, "identity component exists");
    expect(identity != nullptr && identity->playerId == 7, "identity stores player id");
    expect(identity != nullptr && identity->teamIndex == 1, "identity stores team index");

    const auto* transform = world.getComponent<novacore::ecs::TransformComponent>(entity);
    expect(transform != nullptr, "transform component exists");
    expect(transform != nullptr && transform->position.x == 1.0F, "transform stores spawn x");
    expect(transform != nullptr && transform->position.y == 2.0F, "transform stores spawn y");
    expect(transform != nullptr && transform->position.z == 3.0F, "transform stores spawn z");

    const auto* weaponState = world.getComponent<nemisis::weapons::WeaponRuntimeState>(entity);
    expect(weaponState != nullptr, "weapon runtime component exists");
    expect(weaponState != nullptr && weaponState->weaponId == weapon.id, "weapon state stores active weapon id");
    expect(weaponState != nullptr && weaponState->ammoInMagazine == weapon.magazineSize, "weapon state starts with full magazine");

    const auto* loadout = world.getComponent<nemisis::player::PlayerLoadoutComponent>(entity);
    expect(loadout != nullptr, "loadout component exists");
    expect(loadout != nullptr && loadout->activeWeaponId == weapon.id, "loadout stores active weapon id");
}

} // namespace

int main() {
    testSpawnCreatesExpectedPlayerComponents();

    if (failures > 0) {
        std::cerr << failures << " player spawn test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "Nemisis player spawn tests passed\n";
    return EXIT_SUCCESS;
}

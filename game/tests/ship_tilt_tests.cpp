#include <catch2/catch_test_macros.hpp>

#include "components/NetworkEntity.h"
#include "components/Player.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "library/InputManager.h"
#include "network/GameProtocol.h"
#include "systems/ShipTiltSystem.h"

using Game::ShipTiltSystem;

namespace {
struct Ships {
    RType::Registry registry;
    InputManager input;
    ShipTiltSystem tilt;

    Ships()
    {
        registry.registerComponent<Game::Player>();
        registry.registerComponent<RType::Sprite>();
        registry.registerComponent<RType::Transform2D>();
        registry.registerComponent<Game::NetworkEntity>();
        input.defineAction("MoveUp", Game::Protocol::Action::MoveUp);
        input.defineAction("MoveDown", Game::Protocol::Action::MoveDown);
        input.bind(Game::Protocol::Action::MoveUp, 1);
        input.bind(Game::Protocol::Action::MoveDown, 2);
    }

    RType::Entity ship(bool local, float y)
    {
        const auto entity = registry.spawnEntity();
        registry.addComponent(entity, Game::Player(3, false));
        registry.addComponent(entity, RType::Sprite(1));
        registry.addComponent(entity, RType::Transform2D({100.f, y}));
        registry.addComponent(entity, Game::NetworkEntity(1, local));
        return entity;
    }

    int frameLeft(RType::Entity entity)
    {
        return registry.getComponents<RType::Sprite>()[entity]->rectLeft;
    }
};

int left(std::size_t frame)
{
    return ShipTiltSystem::FRAMES.at(frame).left;
}
} // namespace

TEST_CASE("Ship tilt maps to the five frames", "[ship-tilt]")
{
    CHECK(ShipTiltSystem::frameIndex(0.f) == ShipTiltSystem::LEVEL_FRAME);
    CHECK(ShipTiltSystem::frameIndex(1.f) == 4);
    CHECK(ShipTiltSystem::frameIndex(0.5f) == 3);
    CHECK(ShipTiltSystem::frameIndex(-1.f) == 0);
    CHECK(ShipTiltSystem::frameIndex(-5.f) == 0);
}

TEST_CASE("Ship tilt moves towards its target in BANK_TIME", "[ship-tilt]")
{
    const float half = ShipTiltSystem::BANK_TIME / 2.f;

    CHECK(ShipTiltSystem::approach(0.f, 1.f, half) == 0.5f);
    CHECK(
        ShipTiltSystem::approach(0.5f, 1.f, ShipTiltSystem::BANK_TIME) == 1.f);
    CHECK(ShipTiltSystem::approach(1.f, 0.f, half) == 0.5f);
    CHECK(ShipTiltSystem::approach(0.f, 0.f, half) == 0.f);
}

TEST_CASE("The local ship banks with its inputs and levels off at rest",
    "[ship-tilt]")
{
    Ships ships;
    const auto local = ships.ship(true, 500.f);

    ships.tilt.update(ships.registry, ships.input, 0.1f);
    CHECK(ships.frameLeft(local) == left(ShipTiltSystem::LEVEL_FRAME));

    ships.input.setKeyState(1, true);
    ships.tilt.update(ships.registry, ships.input, 1.f);
    CHECK(ships.frameLeft(local) == left(4));

    ships.input.setKeyState(1, false);
    ships.tilt.update(ships.registry, ships.input, 1.f);
    CHECK(ships.frameLeft(local) == left(ShipTiltSystem::LEVEL_FRAME));

    ships.input.setKeyState(2, true);
    ships.tilt.update(ships.registry, ships.input, 1.f);
    CHECK(ships.frameLeft(local) == left(0));
}

TEST_CASE("A remote ship banks with its movement", "[ship-tilt]")
{
    Ships ships;
    const auto remote = ships.ship(false, 500.f);
    auto &transforms = ships.registry.getComponents<RType::Transform2D>();

    ships.tilt.update(ships.registry, ships.input, 0.1f);
    transforms[remote]->position.y = 400.f;
    ships.tilt.update(ships.registry, ships.input, 1.f);
    CHECK(ships.frameLeft(remote) == left(4));

    transforms[remote]->position.y = 600.f;
    ships.tilt.update(ships.registry, ships.input, 1.f);
    CHECK(ships.frameLeft(remote) == left(0));
}

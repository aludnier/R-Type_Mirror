#include <catch2/catch_test_macros.hpp>

#include <set>
#include <vector>

#include "components/Animator.h"
#include "components/Controllable.h"
#include "components/Enemy.h"
#include "components/Hitbox.h"
#include "components/NetworkEntity.h"
#include "components/Player.h"
#include "components/Projectile.h"
#include "components/Rigidbody2D.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "systems/NetworkSyncSystem.h"

namespace {
void registerClientComponents(RType::Registry &registry)
{
    registry.registerComponent<Game::NetworkEntity>();
    registry.registerComponent<Game::Player>();
    registry.registerComponent<Game::Enemy>();
    registry.registerComponent<Game::Projectile>();
    registry.registerComponent<RType::Hitbox>();
    registry.registerComponent<RType::Transform2D>();
    registry.registerComponent<RType::Controllable>();
    registry.registerComponent<RType::Sprite>();
    registry.registerComponent<RType::Physics2D::Rigidbody2D>();
    registry.registerComponent<RType::Animation::Animator>();
}
} // namespace

TEST_CASE("Players get distinct tints", "[network][players]")
{
    std::set<std::array<std::uint8_t, 3>> tints;

    for (RType::Network::ClientId id = 1; id <= 4; ++id)
        tints.insert(Game::NetworkSyncSystem::playerTint(id));
    CHECK(tints.size() == 4);
    CHECK(Game::NetworkSyncSystem::playerTint(1) ==
        std::array<std::uint8_t, 3> {255, 255, 255});
}

TEST_CASE(
    "NetworkSyncSystem tints each player by client id", "[network][players]")
{
    Game::NetworkSyncSystem sync;
    RType::Registry registry;
    AssetManager assets;
    assets.addAsset("laser_bullet", "laser_bullet.png", "Texture");
    registerClientComponents(registry);
    sync.init(registry, assets);
    std::vector<Game::NetworkClient::ServerMessage> messages {
        RType::Network::AcceptConnectPacket {.clientId = 2, .linkedEntity = 10},
        RType::Network::PlayerConnectedPacket {
            .clientId = 1, .linkedEntity = 11, .playerName = "other"}};

    sync.processServerMessages(messages, 10);

    const auto local = sync.findEntity(10);
    const auto other = sync.findEntity(11);
    REQUIRE(local);
    REQUIRE(other);
    auto &sprites = registry.getComponents<RType::Sprite>();
    REQUIRE(sprites[*local]);
    REQUIRE(sprites[*other]);
    const auto localTint = Game::NetworkSyncSystem::playerTint(2);
    CHECK(sprites[*local]->r == localTint[0]);
    CHECK(sprites[*local]->g == localTint[1]);
    CHECK(sprites[*local]->b == localTint[2]);
    CHECK(sprites[*other]->r == 255);
    CHECK(sprites[*other]->b == 255);
}

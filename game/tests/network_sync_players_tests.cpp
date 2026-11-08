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
#include "network/ByteWriter.h"
#include "network/GameProtocol.h"
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

TEST_CASE("NetworkSyncSystem applies the lives sent by the server",
    "[network][players]")
{
    Game::NetworkSyncSystem sync;
    RType::Registry registry;
    AssetManager assets;
    registerClientComponents(registry);
    sync.init(registry, assets);
    RType::Network::ByteWriter lives;
    lives.write(std::uint8_t {1});

    sync.processServerMessages(
        {RType::Network::AcceptConnectPacket {
             .clientId = 1, .linkedEntity = 10},
            RType::Network::ComponentUpdatePacket {.tick = 1,
                .entities = {{10,
                    {{Game::Protocol::PLAYER_LIVES, lives.release()}}}}}},
        10);

    const auto local = sync.findEntity(10);
    REQUIRE(local);
    auto &players = registry.getComponents<Game::Player>();
    REQUIRE(players[*local]);
    CHECK(players[*local]->lives == 1);
    CHECK_FALSE(sync.isGameOver());
}

TEST_CASE("NetworkSyncSystem reports game over when the local ship despawns",
    "[network][players]")
{
    Game::NetworkSyncSystem sync;
    RType::Registry registry;
    AssetManager assets;
    registerClientComponents(registry);
    sync.init(registry, assets);

    sync.processServerMessages({RType::Network::AcceptConnectPacket {
                                    .clientId = 1, .linkedEntity = 10},
                                   RType::Network::EntityDespawnPacket {11}},
        10);
    CHECK_FALSE(sync.isGameOver());

    sync.processServerMessages({RType::Network::EntityDespawnPacket {10}}, 10);
    CHECK(sync.isGameOver());

    sync.reset();
    CHECK_FALSE(sync.isGameOver());
}

TEST_CASE(
    "NetworkSyncSystem announces the levels and the boss", "[network][players]")
{
    Game::NetworkSyncSystem sync;
    RType::Registry registry;
    AssetManager assets;
    registerClientComponents(registry);
    sync.init(registry, assets);
    RType::Network::ByteWriter kind;
    kind.write(static_cast<std::uint8_t>(Game::Protocol::EntityKind::Boss));

    sync.processServerMessages({RType::Network::AcceptConnectPacket {
                                   .clientId = 1, .linkedEntity = 10}},
        10);
    CHECK(sync.takeBanner() == "LEVEL 1");
    CHECK_FALSE(sync.takeBanner().has_value());

    sync.processServerMessages(
        {RType::Network::EntitySpawnPacket {.networkId = 20,
            .components = {{Game::Protocol::ENTITY_KIND, kind.release()}}}},
        10);
    CHECK(sync.takeBanner() == "BOSS");

    sync.processServerMessages({RType::Network::EntityDespawnPacket {20}}, 10);
    CHECK(sync.takeBanner() == "LEVEL 2");
}

TEST_CASE("A PowerUp is not taken for a player", "[network][players]")
{
    Game::NetworkSyncSystem sync;
    RType::Registry registry;
    AssetManager assets;
    registerClientComponents(registry);
    sync.init(registry, assets);
    RType::Network::ByteWriter kind;
    kind.write(static_cast<std::uint8_t>(Game::Protocol::EntityKind::PowerUp));

    sync.processServerMessages(
        {RType::Network::EntitySpawnPacket {.networkId = 30,
            .components = {{Game::Protocol::ENTITY_KIND, kind.release()}}}},
        10);

    const auto powerUp = sync.findEntity(30);
    REQUIRE(powerUp);
    auto &players = registry.getComponents<Game::Player>();
    CHECK((*powerUp >= players.size() || !players[*powerUp]));
}

TEST_CASE("A walker is an enemy, not a player", "[network][players]")
{
    Game::NetworkSyncSystem sync;
    RType::Registry registry;
    AssetManager assets;
    registerClientComponents(registry);
    sync.init(registry, assets);
    RType::Network::ByteWriter kind;
    kind.write(static_cast<std::uint8_t>(Game::Protocol::EntityKind::Walker));

    sync.processServerMessages(
        {RType::Network::EntitySpawnPacket {.networkId = 40,
            .components = {{Game::Protocol::ENTITY_KIND, kind.release()}}}},
        10);

    const auto walker = sync.findEntity(40);
    REQUIRE(walker);
    CHECK(registry.getComponents<Game::Enemy>()[*walker].has_value());
    auto &players = registry.getComponents<Game::Player>();
    CHECK((*walker >= players.size() || !players[*walker]));
}

TEST_CASE(
    "Players are kept in the play area, above the ground", "[network][players]")
{
    using Game::Protocol::clampToPlayArea;

    CHECK(clampToPlayArea({500.f, 400.f}) == RType::Vector2 {500.f, 400.f});
    CHECK(clampToPlayArea({-50.f, -50.f}) == RType::Vector2 {28.f, 12.f});
    CHECK(clampToPlayArea({5000.f, 5000.f}) ==
        RType::Vector2 {1920.f - 28.f, Game::Protocol::GROUND_TOP - 12.f});
}

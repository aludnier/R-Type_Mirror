/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** NetworkSyncSystem implementation
*/

#include "systems/NetworkSyncSystem.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include "components/Controllable.h"
#include "components/Enemy.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Player.h"
#include "components/Projectile.h"
#include "components/Rigidbody2D.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "network/ByteReader.h"
#include "network/GameProtocol.h"

#include "components/NetworkEntity.h"

#include "components/AnimationClip.h"
#include "components/Animator.h"
#include "systems/AnimationSystem.h"
#include "systems/ShipTiltSystem.h"

namespace Game {
namespace {
    constexpr float EXPLOSION_SCALE = 2.f;
    constexpr float EXPLOSION_FRAME_TIME = 0.07f;
    constexpr auto EXPLOSION_DURATION = std::chrono::milliseconds(420);
} // namespace

// Animation frame data extracted from sprite sheets
namespace AnimationFrames {

    // Boss: the walking cannon of the player sheet (6 frames)
    const std::vector<RType::Animation::AnimationFrame> boss = {{1, 2, 31, 25},
        {34, 3, 32, 24},
        {67, 4, 32, 23},
        {100, 2, 31, 25},
        {133, 3, 31, 24},
        {166, 4, 32, 23}};

    // Walker: the walking cannon of the player sheet, one walk cycle (the
    // next 6 frames play it backwards)
    const std::vector<RType::Animation::AnimationFrame> walker = {
        {1, 2, 31, 25},
        {34, 3, 32, 24},
        {67, 4, 32, 23},
        {100, 2, 31, 25},
        {133, 3, 31, 24},
        {166, 4, 32, 23}};

    // Explosion of a shot down enemy (laser bullet sheet, y = 296)
    const std::vector<RType::Animation::AnimationFrame> explosion = {
        {72, 296, 25, 30},
        {109, 296, 25, 30},
        {146, 296, 32, 29},
        {182, 296, 32, 27},
        {217, 300, 27, 20},
        {247, 304, 18, 15}};

    // Force pod animation (12 frames)
    const std::vector<RType::Animation::AnimationFrame> forcePod = {
        {1, 2, 16, 12},
        {18, 2, 16, 12},
        {35, 2, 16, 12},
        {52, 2, 16, 14},
        {69, 4, 16, 12},
        {86, 4, 16, 12},
        {103, 4, 16, 12},
        {120, 4, 16, 12},
        {137, 4, 16, 12},
        {154, 2, 16, 14},
        {171, 2, 16, 12},
        {188, 2, 16, 12}};

    // Bydo scout animation (16 frames)
    // Bydo scout: its first 8 frames, nose to the left. The next 8 face
    // right, so the scout seemed to turn around.
    const std::vector<RType::Animation::AnimationFrame> bydoScout = {
        {5, 6, 21, 24},
        {38, 12, 21, 14},
        {71, 16, 21, 7},
        {104, 10, 21, 14},
        {137, 6, 21, 24},
        {170, 12, 21, 14},
        {203, 13, 21, 7},
        {236, 10, 21, 14}};

    // Laser bullet animation (small blue shot of the laser bullet sheet,
    // y = 103: 4 frames)
    const std::vector<RType::Animation::AnimationFrame> laserBullet = {
        {232, 103, 16, 12},
        {249, 105, 16, 8},
        {267, 105, 16, 8},
        {284, 103, 16, 12}};

    // Wave cannon: the long beam of the laser bullet sheet (y = 170), so a
    // fully charged shot looks different from the small blue laser
    const std::vector<RType::Animation::AnimationFrame> waveCannon = {
        {106, 171, 78, 14},
        {185, 170, 80, 16},
        {267, 170, 80, 16},
        {348, 171, 78, 14}};

    // Enemy bullet: the blue orb of the enemy bullet sheet. The rest of
    // its first row is an explosion, not a bullet.
    const std::vector<RType::Animation::AnimationFrame> enemyBullet = {
        {184, 9, 16, 16}};

} // namespace AnimationFrames

void NetworkSyncSystem::init(RType::Registry &registry, AssetManager &resources)
{
    _registry = registry;
    _resources = resources;
}

void NetworkSyncSystem::syncWithServer(const InputManager &input,
    NetworkClient &client, RType::IAudioPlayer *audio)
{
    client.update(0.0f);

    if (!client.isConnected()) {
        if (_wasConnected) {
            reset();
            _wasConnected = false;
        }
        return;
    }

    _wasConnected = true;

    removeOldExplosions();
    processServerMessages(
        client.pollServerMessages(), client.getLinkedEntity(), audio);
    sendLocalPlayerInput(input, client);
}

struct NetworkSyncSystem::ServerMessageVisitor {
    NetworkSyncSystem &system;
    RType::Network::NetworkId localLinkedEntity;
    RType::IAudioPlayer *audio;

    void operator()(const RType::Network::AcceptConnectPacket &packet) const
    {
        system.handleAcceptConnect(packet);
    }

    void operator()(const RType::Network::PlayerConnectedPacket &packet) const
    {
        system.handlePlayerConnected(packet, localLinkedEntity);
    }

    void operator()(
        const RType::Network::PlayerDisconnectedPacket &packet) const
    {
        system.handlePlayerDisconnected(packet);
    }

    void operator()(const RType::Network::EntitySpawnPacket &packet) const
    {
        system.handleEntitySpawn(packet, localLinkedEntity);
    }

    void operator()(const RType::Network::EntityDespawnPacket &packet) const
    {
        system.handleEntityDespawn(packet);
    }

    void operator()(const RType::Network::ComponentUpdatePacket &packet) const
    {
        system.handleComponentUpdate(packet, localLinkedEntity);
    }

    void operator()(const RType::Network::PlaySoundMsgPacket &packet) const
    {
        if (audio)
            audio->playSound(packet.assetName, packet.volume, packet.pitch);
    }

    void operator()(const auto & /* packet */) const
    {
        // Ignore unhandled packets
    }
};

void NetworkSyncSystem::processServerMessages(
    const std::vector<NetworkClient::ServerMessage> &messages,
    RType::Network::NetworkId localLinkedEntity, RType::IAudioPlayer *audio)
{
    if (messages.empty()) {
        return;
    }

    ServerMessageVisitor visitor {*this, localLinkedEntity, audio};

    for (const auto &msg : messages) {
        std::visit(visitor, msg);
    }
    applyPlayerColors();
}

std::array<std::uint8_t, 3> NetworkSyncSystem::playerTint(
    RType::Network::ClientId clientId)
{
    // Okabe-Ito colours, safe for colour-blind players.
    static constexpr std::array<std::array<std::uint8_t, 3>, 4> TINTS = {{
        {255, 255, 255},
        {230, 159, 0},
        {240, 228, 66},
        {204, 121, 167},
    }};

    return TINTS[(clientId + TINTS.size() - 1) % TINTS.size()];
}

void NetworkSyncSystem::applyPlayerColors()
{
    if (_clientToNetwork.empty() ||
        !_registry->get().isRegistered<RType::Sprite>())
        return;
    auto &sprites = _registry->get().getComponents<RType::Sprite>();

    for (const auto &[clientId, networkId] : _clientToNetwork) {
        const auto entity = _networkToEntity.find(networkId);
        if (entity == _networkToEntity.end() ||
            entity->second >= sprites.size() || !sprites[entity->second])
            continue;
        const auto [r, g, b] = playerTint(clientId);
        auto &sprite = *sprites[entity->second];
        sprite.r = r;
        sprite.g = g;
        sprite.b = b;
    }
}

void NetworkSyncSystem::sendLocalPlayerInput(
    const InputManager &input, NetworkClient &client)
{
    client.sendControls(input.actionMask());
}

void NetworkSyncSystem::reset()
{
    for (auto entityId : _networkToEntity | std::views::values) {
        try {
            _registry->get().killEntity(
                RType::Registry::entityFromIndex(entityId));
        } catch (...) {
        }
    }
    _networkToEntity.clear();
    _entityToNetwork.clear();
    _clientToNetwork.clear();
    _localNetworkId = RType::Network::NO_ENTITY;
    _gameOver = false;
    _bosses.clear();
    _banner.reset();
    _score = 0;
    for (const auto &explosion : _explosions)
        _registry->get().killEntity(
            RType::Registry::entityFromIndex(explosion.first));
    _explosions.clear();
}

bool NetworkSyncSystem::isGameOver() const
{
    return _gameOver;
}

std::uint32_t NetworkSyncSystem::score() const
{
    return _score;
}

std::optional<RType::Entity> NetworkSyncSystem::bossEntity() const
{
    for (const auto networkId : _bosses)
        if (const auto entity = findEntity(networkId))
            return entity;
    return std::nullopt;
}

std::optional<std::string> NetworkSyncSystem::takeBanner()
{
    return std::exchange(_banner, std::nullopt);
}

std::optional<RType::Entity> NetworkSyncSystem::findEntity(
    RType::Network::NetworkId id) const
{
    auto it = _networkToEntity.find(id);
    if (it != _networkToEntity.end()) {
        return RType::Registry::entityFromIndex(it->second);
    }
    return std::nullopt;
}

void NetworkSyncSystem::addShipSprite(RType::Entity entity)
{
    if (!_resources->get().hasAsset("laser_bullet"))
        return;
    const auto &asset = _resources->get().getAsset("laser_bullet");
    const auto &level = ShipTiltSystem::FRAMES[ShipTiltSystem::LEVEL_FRAME];

    // Not animated: ShipTiltSystem picks the frame from the movement.
    _registry->get().addComponent<RType::Sprite>(entity,
        RType::Sprite(
            asset.id, level.left, level.top, level.width, level.height, 10));
}

void NetworkSyncSystem::addAnimatedSprite(RType::Entity entity,
    const std::string &assetName,
    const std::vector<RType::Animation::AnimationFrame> &frames,
    float frameTime, int zIndex)
{
    if (frames.empty() || !_resources->get().hasAsset(assetName))
        return;
    const auto &asset = _resources->get().getAsset(assetName);
    const auto &first = frames.front();

    _registry->get().addComponent<RType::Sprite>(entity,
        RType::Sprite(asset.id,
            first.left,
            first.top,
            first.width,
            first.height,
            zIndex));
    auto &animator = _registry->get().addComponent<RType::Animation::Animator>(
        entity, RType::Animation::Animator {});
    if (animator) {
        RType::Animation::AnimationSystem::addClip(*animator,
            "idle",
            RType::Animation::AnimationSystem::createClip(frames, frameTime));
        RType::Animation::AnimationSystem::play(*animator, "idle", true);
    }
}

void NetworkSyncSystem::handleAcceptConnect(
    const RType::Network::AcceptConnectPacket &packet)
{
    _clientToNetwork[packet.clientId] = packet.linkedEntity;
    _localNetworkId = packet.linkedEntity;
    _gameOver = false;
    _level = 1;
    _banner = "LEVEL 1";
    if (packet.linkedEntity == RType::Network::NO_ENTITY) {
        return;
    }

    auto it = _networkToEntity.find(packet.linkedEntity);
    if (it == _networkToEntity.end()) {
        auto entity = _registry->get().spawnEntity();
        auto entityId = static_cast<std::size_t>(entity);
        _networkToEntity.insert_or_assign(packet.linkedEntity, entityId);
        _entityToNetwork.insert_or_assign(entityId, packet.linkedEntity);

        _registry->get().addComponent<NetworkEntity>(
            entity, NetworkEntity(packet.linkedEntity, true));
        _registry->get().addComponent<RType::Transform2D>(
            entity, RType::Transform2D({150.0f, 540.0f}, {1.75f, 1.75f}));
        _registry->get().addComponent<RType::Controllable>(
            entity, RType::Controllable(0));
        _registry->get().addComponent<Player>(entity, Player(3, true));
        _registry->get().addComponent<RType::Physics2D::Rigidbody2D>(
            entity, RType::Physics2D::Rigidbody2D {});
        _registry->get().addComponent<Hitbox>(
            entity, Hitbox(RType::Vector2 {40.0f, 20.0f}));

        addShipSprite(entity);
    }
}

void NetworkSyncSystem::handlePlayerConnected(
    const RType::Network::PlayerConnectedPacket &packet,
    RType::Network::NetworkId localLinkedEntity)
{
    _clientToNetwork[packet.clientId] = packet.linkedEntity;

    if (packet.linkedEntity == RType::Network::NO_ENTITY ||
        packet.linkedEntity == localLinkedEntity) {
        return;
    }

    auto it = _networkToEntity.find(packet.linkedEntity);
    if (it == _networkToEntity.end()) {
        auto entity = _registry->get().spawnEntity();
        auto entityId = static_cast<std::size_t>(entity);
        _networkToEntity.insert_or_assign(packet.linkedEntity, entityId);
        _entityToNetwork.insert_or_assign(entityId, packet.linkedEntity);

        _registry->get().addComponent<NetworkEntity>(
            entity, NetworkEntity(packet.linkedEntity, false));
        _registry->get().addComponent<RType::Transform2D>(
            entity, RType::Transform2D({150.0f, 540.0f}, {1.75f, 1.75f}));
        _registry->get().addComponent<Player>(entity, Player(3, false));
        _registry->get().addComponent<RType::Physics2D::Rigidbody2D>(
            entity, RType::Physics2D::Rigidbody2D {});
        _registry->get().addComponent<Hitbox>(
            entity, Hitbox(RType::Vector2 {40.0f, 20.0f}));

        addShipSprite(entity);
    }
}

void NetworkSyncSystem::handlePlayerDisconnected(
    const RType::Network::PlayerDisconnectedPacket &packet)
{
    auto clientIt = _clientToNetwork.find(packet.clientId);
    if (clientIt == _clientToNetwork.end()) {
        return;
    }

    RType::Network::NetworkId netId = clientIt->second;
    _clientToNetwork.erase(clientIt);

    if (netId == RType::Network::NO_ENTITY) {
        return;
    }

    auto it = _networkToEntity.find(netId);
    if (it != _networkToEntity.end()) {
        std::size_t entityId = it->second;
        _entityToNetwork.erase(entityId);
        _networkToEntity.erase(it);
        try {
            _registry->get().killEntity(
                RType::Registry::entityFromIndex(entityId));
        } catch (...) {
        }
    }
}

void NetworkSyncSystem::handleEntitySpawn(
    const RType::Network::EntitySpawnPacket &packet,
    RType::Network::NetworkId localLinkedEntity)
{
    auto it = _networkToEntity.find(packet.networkId);
    RType::Entity entity = it != _networkToEntity.end()
        ? RType::Registry::entityFromIndex(it->second)
        : _registry->get().spawnEntity();
    auto entityId = static_cast<std::size_t>(entity);

    const bool isLocal = packet.networkId == localLinkedEntity;
    if (it == _networkToEntity.end()) {
        _networkToEntity.insert_or_assign(packet.networkId, entityId);
        _entityToNetwork.insert_or_assign(entityId, packet.networkId);
        _registry->get().addComponent<NetworkEntity>(
            entity, NetworkEntity(packet.networkId, isLocal));
    }

    auto &controllables = _registry->get().getComponents<RType::Controllable>();
    auto &players = _registry->get().getComponents<Player>();

    if (isLocal) {
        if (entityId >= controllables.size() ||
            !controllables[entity].has_value()) {
            _registry->get().addComponent<RType::Controllable>(
                entity, RType::Controllable(0));
        }
        if (entityId >= players.size() || !players[entity].has_value()) {
            _registry->get().addComponent<Player>(entity, Player(3, true));
        }
    }

    for (const auto &comp : packet.components) {
        applyComponentData(entity, comp, isLocal);
    }

    // Enemies and projectiles carry ENTITY_KIND and are drawn from their
    // game component. The other entities are the players.
    auto &enemies = _registry->get().getComponents<Enemy>();
    auto &projectiles = _registry->get().getComponents<Projectile>();
    const bool hasKind =
        (entityId < enemies.size() && enemies[entity].has_value()) ||
        (entityId < projectiles.size() && projectiles[entity].has_value()) ||
        std::ranges::any_of(packet.components, [](const auto &component) {
            return component.typeId == Protocol::ENTITY_KIND;
        });

    if (hasKind) {
        return;
    }
    if (!isLocal &&
        (entityId >= players.size() || !players[entity].has_value())) {
        _registry->get().addComponent<Player>(entity, Player(3, false));
    }

    auto &sprites = _registry->get().getComponents<RType::Sprite>();
    if (entityId >= sprites.size() || !sprites[entity].has_value()) {
        addShipSprite(entity);
    }
    auto &transforms = _registry->get().getComponents<RType::Transform2D>();
    if (entityId < transforms.size() && transforms[entity].has_value()) {
        transforms[entity]->scale = {1.75f, 1.75f};
    }
}

void NetworkSyncSystem::spawnExplosion(RType::Vector2 position)
{
    if (!_resources->get().hasAsset("laser_bullet"))
        return;
    auto &registry = _registry->get();
    const auto entity = registry.spawnEntity();
    const auto &asset = _resources->get().getAsset("laser_bullet");
    const auto &first = AnimationFrames::explosion.front();

    registry.addComponent(entity,
        RType::Transform2D(position, {EXPLOSION_SCALE, EXPLOSION_SCALE}));
    registry.addComponent(entity,
        RType::Sprite(
            asset.id, first.left, first.top, first.width, first.height, 20));
    auto &animator = registry.addComponent<RType::Animation::Animator>(
        entity, RType::Animation::Animator {});
    if (animator) {
        RType::Animation::AnimationSystem::addClip(*animator,
            "explode",
            RType::Animation::AnimationSystem::createClip(
                AnimationFrames::explosion, EXPLOSION_FRAME_TIME));
        RType::Animation::AnimationSystem::play(*animator, "explode", false);
    }
    _explosions.emplace_back(static_cast<std::size_t>(entity),
        std::chrono::steady_clock::now() + EXPLOSION_DURATION);
}

void NetworkSyncSystem::removeOldExplosions()
{
    const auto now = std::chrono::steady_clock::now();

    std::erase_if(_explosions, [&](const auto &explosion) {
        if (explosion.second > now)
            return false;
        _registry->get().killEntity(
            RType::Registry::entityFromIndex(explosion.first));
        return true;
    });
}

void NetworkSyncSystem::handleEntityDespawn(
    const RType::Network::EntityDespawnPacket &packet)
{
    // The server despawns a player's ship when it has no lives left.
    if (packet.networkId == _localNetworkId &&
        _localNetworkId != RType::Network::NO_ENTITY)
        _gameOver = true;
    // The boss only goes away when it is shot down: next level.
    if (_bosses.erase(packet.networkId) > 0) {
        ++_level;
        _banner = "LEVEL " + std::to_string(_level);
    }
    auto it = _networkToEntity.find(packet.networkId);
    if (it != _networkToEntity.end()) {
        std::size_t entityId = it->second;
        _entityToNetwork.erase(entityId);
        _networkToEntity.erase(it);
        // An enemy that goes away explodes (off screen, nobody sees it).
        auto &enemies = _registry->get().getComponents<Enemy>();
        auto &transforms = _registry->get().getComponents<RType::Transform2D>();
        if (entityId < enemies.size() && enemies[entityId] &&
            entityId < transforms.size() && transforms[entityId])
            spawnExplosion(transforms[entityId]->position);
        try {
            _registry->get().killEntity(
                RType::Registry::entityFromIndex(entityId));
        } catch (...) {
        }
    }
}

void NetworkSyncSystem::handleComponentUpdate(
    const RType::Network::ComponentUpdatePacket &packet,
    RType::Network::NetworkId localLinkedEntity)
{
    for (const auto &[networkId, components] : packet.entities) {
        auto it = _networkToEntity.find(networkId);
        RType::Entity entity = it != _networkToEntity.end()
            ? RType::Registry::entityFromIndex(it->second)
            : _registry->get().spawnEntity();
        auto entityId = static_cast<std::size_t>(entity);

        const bool isLocal = networkId == localLinkedEntity;
        if (it == _networkToEntity.end()) {
            _networkToEntity.insert_or_assign(networkId, entityId);
            _entityToNetwork.insert_or_assign(entityId, networkId);
            _registry->get().addComponent<NetworkEntity>(
                entity, NetworkEntity(networkId, isLocal, packet.tick));
        }

        for (const auto &comp : components) {
            applyComponentData(entity, comp, isLocal);
        }
    }
}

void NetworkSyncSystem::applyComponentData(RType::Entity entity,
    const RType::Network::ComponentData &compData, bool isLocalPlayer)
{
    RType::Network::ByteReader reader(compData.data);
    auto entityId = static_cast<std::size_t>(entity);

    if (compData.typeId == Protocol::ENTITY_KIND) {
        applyEntityKind(entity, reader);
        return;
    }
    if (compData.typeId == Protocol::ENEMY_HEALTH) {
        const auto current = reader.read<std::uint16_t>();
        const auto max = reader.read<std::uint16_t>();
        if (reader.ok())
            _registry->get().addComponent<Health>(entity, Health(current, max));
        return;
    }
    if (compData.typeId == Protocol::TEAM_SCORE) {
        const auto score = reader.read<std::uint32_t>();
        if (reader.ok())
            _score = score;
        return;
    }
    if (compData.typeId == Protocol::PLAYER_LIVES) {
        const auto lives = reader.read<std::uint8_t>();
        auto &players = _registry->get().getComponents<Player>();
        if (reader.ok() && entityId < players.size() && players[entity])
            players[entity]->lives = lives;
        return;
    }

    switch (static_cast<RType::Network::EngineComponentType>(compData.typeId)) {
        case RType::Network::EngineComponentType::Position: {
            float posX = reader.readFloat();
            float posY = reader.readFloat();
            if (!reader.ok()) {
                return;
            }

            auto &transforms =
                _registry->get().getComponents<RType::Transform2D>();
            if (entityId < transforms.size() &&
                transforms[entity].has_value()) {
                auto &transform = transforms[entity].value();
                if (isLocalPlayer) {
                    // Server reconciliation for local player: snap if
                    // desynchronized beyond threshold
                    float dx = transform.position.x - posX;
                    float dy = transform.position.y - posY;
                    if (std::sqrt(dx * dx + dy * dy) > 40.0f) {
                        transform.position.x = posX;
                        transform.position.y = posY;
                    }
                } else {
                    // Remote entity authoritative update
                    transform.position.x = posX;
                    transform.position.y = posY;
                }
            } else {
                _registry->get().addComponent<RType::Transform2D>(
                    entity, RType::Transform2D({posX, posY}));
            }
            break;
        }
        case RType::Network::EngineComponentType::Physics: {
            float vx = reader.readFloat();
            float vy = reader.readFloat();
            float hw = reader.readFloat();
            float hh = reader.readFloat();
            if (!reader.ok()) {
                return;
            }

            auto &velocities =
                _registry->get().getComponents<RType::Physics2D::Rigidbody2D>();
            if (entityId < velocities.size() &&
                velocities[entity].has_value()) {
                auto &vel = velocities[entity].value();
                vel.velocity.x = vx;
                vel.velocity.y = vy;
            } else {
                RType::Physics2D::Rigidbody2D body;
                body.velocity = {vx, vy};
                _registry->get()
                    .emplaceComponent<RType::Physics2D::Rigidbody2D>(
                        entity, body);
            }

            if (hw > 0.0f && hh > 0.0f) {
                auto &hitboxes = _registry->get().getComponents<Hitbox>();
                if (entityId < hitboxes.size() &&
                    hitboxes[entity].has_value()) {
                    auto &hitbox = hitboxes[entity].value();
                    hitbox.size.x = hw;
                    hitbox.size.y = hh;
                } else {
                    _registry->get().addComponent<Hitbox>(
                        entity, Hitbox(RType::Vector2 {hw, hh}));
                }
            }
            break;
        }
        case RType::Network::EngineComponentType::Animation:
        default:
            break;
    }
}

void NetworkSyncSystem::applyEntityKind(
    RType::Entity entity, RType::Network::ByteReader &reader)
{
    auto kind = reader.read<std::uint8_t>();

    if (!reader.ok() || !Protocol::isValidKind(kind)) {
        return;
    }
    auto &sprites = _registry->get().getComponents<RType::Sprite>();
    auto entityId = static_cast<std::size_t>(entity);
    switch (static_cast<Protocol::EntityKind>(kind)) {
        case Protocol::EntityKind::Enemy: {
            _registry->get().addComponent<Enemy>(entity, Enemy());
            if (entityId >= sprites.size() || !sprites[entity].has_value()) {
                try {
                    auto &asset =
                        _resources->get().getAsset("enemy_bydo_scout");
                    const auto &firstFrame = AnimationFrames::bydoScout[0];
                    _registry->get().addComponent<RType::Sprite>(entity,
                        RType::Sprite(asset.id,
                            firstFrame.left,
                            firstFrame.top,
                            firstFrame.width,
                            firstFrame.height,
                            5));
                    auto &animatorOpt =
                        _registry->get()
                            .addComponent<RType::Animation::Animator>(
                                entity, RType::Animation::Animator {});
                    if (animatorOpt) {
                        RType::Animation::AnimationSystem::addClip(*animatorOpt,
                            "idle",
                            RType::Animation::AnimationSystem::createClip(
                                AnimationFrames::bydoScout, 0.1f));
                        RType::Animation::AnimationSystem::play(
                            *animatorOpt, "idle", true);
                    }
                } catch (...) {
                }
            }
            auto &transforms =
                _registry->get().getComponents<RType::Transform2D>();
            if (entityId < transforms.size() &&
                transforms[entity].has_value()) {
                transforms[entity]->scale = {1.75f, 1.75f};
            }
            break;
        }
        case Protocol::EntityKind::Gunner: {
            _registry->get().addComponent<Enemy>(entity, Enemy());
            // A flying enemy: a blue Bydo, bigger than a scout.
            if (entityId >= sprites.size() || !sprites[entity].has_value()) {
                addAnimatedSprite(entity,
                    "enemy_bydo_scout",
                    AnimationFrames::bydoScout,
                    0.1f,
                    5);
                if (sprites[entity]) {
                    sprites[entity]->r = 110;
                    sprites[entity]->g = 170;
                    sprites[entity]->b = 255;
                }
            }
            auto &transforms =
                _registry->get().getComponents<RType::Transform2D>();
            if (entityId < transforms.size() &&
                transforms[entity].has_value()) {
                transforms[entity]->scale = {2.2f, 2.2f};
            }
            break;
        }
        case Protocol::EntityKind::Boss: {
            _registry->get().addComponent<Enemy>(entity, Enemy());
            if (entityId >= sprites.size() || !sprites[entity].has_value()) {
                addAnimatedSprite(
                    entity, "player_ship", AnimationFrames::boss, 0.15f, 6);
                if (sprites[entity]) {
                    sprites[entity]->g = 120;
                    sprites[entity]->b = 120;
                }
            }
            auto &transforms =
                _registry->get().getComponents<RType::Transform2D>();
            if (entityId < transforms.size() &&
                transforms[entity].has_value()) {
                transforms[entity]->scale = {5.f, 5.f};
            }
            if (const auto network = _entityToNetwork.find(entityId);
                network != _entityToNetwork.end())
                _bosses.insert(network->second);
            _banner = "BOSS";
            break;
        }
        case Protocol::EntityKind::Walker: {
            _registry->get().addComponent<Enemy>(entity, Enemy());
            if (entityId >= sprites.size() || !sprites[entity].has_value())
                addAnimatedSprite(
                    entity, "player_ship", AnimationFrames::walker, 0.08f, 5);
            auto &transforms =
                _registry->get().getComponents<RType::Transform2D>();
            if (entityId < transforms.size() &&
                transforms[entity].has_value()) {
                transforms[entity]->scale = {1.75f, 1.75f};
            }
            break;
        }
        case Protocol::EntityKind::PowerUp: {
            if (entityId >= sprites.size() || !sprites[entity].has_value())
                addAnimatedSprite(
                    entity, "force_pod", AnimationFrames::forcePod, 0.08f, 8);
            auto &transforms =
                _registry->get().getComponents<RType::Transform2D>();
            if (entityId < transforms.size() &&
                transforms[entity].has_value()) {
                transforms[entity]->scale = {2.25f, 2.25f};
            }
            break;
        }
        case Protocol::EntityKind::PlayerLaser: {
            _registry->get().addComponent<Projectile>(
                entity, Projectile(true, 1, false, 700.0f));
            if (entityId >= sprites.size() || !sprites[entity].has_value()) {
                try {
                    auto &asset = _resources->get().getAsset("laser_bullet");
                    const auto &firstFrame = AnimationFrames::laserBullet[0];
                    _registry->get().addComponent<RType::Sprite>(entity,
                        RType::Sprite(asset.id,
                            firstFrame.left,
                            firstFrame.top,
                            firstFrame.width,
                            firstFrame.height,
                            15));
                    auto &animatorOpt =
                        _registry->get()
                            .addComponent<RType::Animation::Animator>(
                                entity, RType::Animation::Animator {});
                    if (animatorOpt) {
                        RType::Animation::AnimationSystem::addClip(*animatorOpt,
                            "shoot",
                            RType::Animation::AnimationSystem::createClip(
                                AnimationFrames::laserBullet, 0.05f));
                        RType::Animation::AnimationSystem::play(
                            *animatorOpt, "shoot", true);
                    }
                } catch (...) {
                }
            }
            auto &transforms =
                _registry->get().getComponents<RType::Transform2D>();
            if (entityId < transforms.size() &&
                transforms[entity].has_value()) {
                transforms[entity]->scale = {1.75f, 1.75f};
            }
            break;
        }
        case Protocol::EntityKind::ChargedBeam: {
            _registry->get().addComponent<Projectile>(
                entity, Projectile(true, 5, true, 900.0f));
            if (entityId >= sprites.size() || !sprites[entity].has_value()) {
                try {
                    auto &asset = _resources->get().getAsset("laser_bullet");
                    const auto &firstFrame = AnimationFrames::waveCannon[0];
                    _registry->get().addComponent<RType::Sprite>(entity,
                        RType::Sprite(asset.id,
                            firstFrame.left,
                            firstFrame.top,
                            firstFrame.width,
                            firstFrame.height,
                            15));
                    auto &animatorOpt =
                        _registry->get()
                            .addComponent<RType::Animation::Animator>(
                                entity, RType::Animation::Animator {});
                    if (animatorOpt) {
                        RType::Animation::AnimationSystem::addClip(*animatorOpt,
                            "idle",
                            RType::Animation::AnimationSystem::createClip(
                                AnimationFrames::waveCannon, 0.1f));
                        RType::Animation::AnimationSystem::play(
                            *animatorOpt, "idle", true);
                    }
                } catch (...) {
                }
            }
            auto &transforms =
                _registry->get().getComponents<RType::Transform2D>();
            if (entityId < transforms.size() &&
                transforms[entity].has_value()) {
                transforms[entity]->scale = {1.3f, 1.3f};
            }
            break;
        }
        case Protocol::EntityKind::EnemyBullet: {
            _registry->get().addComponent<Projectile>(
                entity, Projectile(false, 1, false, 450.0f));
            if (entityId >= sprites.size() || !sprites[entity].has_value()) {
                try {
                    auto &asset = _resources->get().getAsset("enemy_bullet");
                    const auto &firstFrame = AnimationFrames::enemyBullet[0];
                    _registry->get().addComponent<RType::Sprite>(entity,
                        RType::Sprite(asset.id,
                            firstFrame.left,
                            firstFrame.top,
                            firstFrame.width,
                            firstFrame.height,
                            15));
                    auto &animatorOpt =
                        _registry->get()
                            .addComponent<RType::Animation::Animator>(
                                entity, RType::Animation::Animator {});
                    if (animatorOpt) {
                        RType::Animation::AnimationSystem::addClip(*animatorOpt,
                            "shoot",
                            RType::Animation::AnimationSystem::createClip(
                                AnimationFrames::enemyBullet, 0.08f));
                        RType::Animation::AnimationSystem::play(
                            *animatorOpt, "shoot", true);
                    }
                } catch (...) {
                }
            }
            auto &transforms =
                _registry->get().getComponents<RType::Transform2D>();
            if (entityId < transforms.size() &&
                transforms[entity].has_value()) {
                transforms[entity]->scale = {1.3125f, 1.3125f};
            }
            break;
        }
        case Protocol::EntityKind::Player:
            break;
    }
}

} // namespace Game

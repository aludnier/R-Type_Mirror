/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** NetworkSyncSystem implementation
*/

#include "systems/NetworkSyncSystem.h"
#include <cmath>
#include <iostream>
#include "components/Controllable.h"
#include "components/Enemy.h"
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

namespace Game {

void NetworkSyncSystem::syncWithServer(RType::Registry &registry,
    const InputManager &input, ResourceManager &resources,
    NetworkClient &client)
{
    client.update(0.0f);

    if (!client.isConnected()) {
        if (_wasConnected) {
            reset(registry);
            _wasConnected = false;
        }
        return;
    }

    _wasConnected = true;

    processServerMessages(client.pollServerMessages(),
        registry,
        resources,
        client.getLinkedEntity());
    sendLocalPlayerInput(input, client);
}

struct NetworkSyncSystem::ServerMessageVisitor {
    NetworkSyncSystem &system;
    RType::Registry &registry;
    ResourceManager &resources;
    RType::Network::NetworkId localLinkedEntity;

    void operator()(const RType::Network::AcceptConnectPacket &packet) const
    {
        system.handleAcceptConnect(packet, registry, resources);
    }

    void operator()(const RType::Network::PlayerConnectedPacket &packet) const
    {
        system.handlePlayerConnected(
            packet, registry, resources, localLinkedEntity);
    }

    void operator()(
        const RType::Network::PlayerDisconnectedPacket &packet) const
    {
        system.handlePlayerDisconnected(packet, registry);
    }

    void operator()(const RType::Network::EntitySpawnPacket &packet) const
    {
        system.handleEntitySpawn(
            packet, registry, resources, localLinkedEntity);
    }

    void operator()(const RType::Network::EntityDespawnPacket &packet) const
    {
        system.handleEntityDespawn(packet, registry);
    }

    void operator()(const RType::Network::ComponentUpdatePacket &packet) const
    {
        system.handleComponentUpdate(packet, registry, localLinkedEntity);
    }

    void operator()(const auto & /* packet */) const
    {
        // Ignore unhandled packets
    }
};

void NetworkSyncSystem::processServerMessages(
    const std::vector<NetworkClient::ServerMessage> &messages,
    RType::Registry &registry, ResourceManager &resources,
    RType::Network::NetworkId localLinkedEntity)
{
    if (messages.empty()) {
        return;
    }

    ServerMessageVisitor visitor {
        *this, registry, resources, localLinkedEntity};

    for (const auto &msg : messages) {
        std::visit(visitor, msg);
    }
}

void NetworkSyncSystem::sendLocalPlayerInput(
    const InputManager &input, NetworkClient &client)
{
    client.sendControls(input.actionMask());
}

void NetworkSyncSystem::reset(RType::Registry &registry)
{
    for (auto entityId : _networkToEntity | std::views::values) {
        try {
            registry.killEntity(RType::Registry::entityFromIndex(entityId));
        } catch (...) {
        }
    }
    _networkToEntity.clear();
    _entityToNetwork.clear();
    _clientToNetwork.clear();
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

void NetworkSyncSystem::handleAcceptConnect(
    const RType::Network::AcceptConnectPacket &packet,
    RType::Registry &registry, const ResourceManager &resources)
{
    if (packet.linkedEntity == RType::Network::NO_ENTITY) {
        return;
    }

    auto it = _networkToEntity.find(packet.linkedEntity);
    if (it == _networkToEntity.end()) {
        auto entity = registry.spawnEntity();
        auto entityId = static_cast<std::size_t>(entity);
        _networkToEntity.insert_or_assign(packet.linkedEntity, entityId);
        _entityToNetwork.insert_or_assign(entityId, packet.linkedEntity);

        registry.addComponent<NetworkEntity>(
            entity, NetworkEntity(packet.linkedEntity, true));
        registry.addComponent<RType::Transform2D>(
            entity, RType::Transform2D({150.0f, 540.0f}));
        registry.addComponent<RType::Controllable>(
            entity, RType::Controllable(0));
        registry.addComponent<Player>(entity, Player(3, true));
        registry.addComponent<RType::Physics2D::Rigidbody2D>(
            entity, RType::Physics2D::Rigidbody2D {});
        registry.addComponent<Hitbox>(
            entity, Hitbox(RType::Vector2 {40.0f, 20.0f}));

        try {
            auto &asset = resources.getAsset("player_ship");
            registry.addComponent<RType::Sprite>(
                entity, RType::Sprite(asset.id, 10));
        } catch (...) {
        }
    }
}

void NetworkSyncSystem::handlePlayerConnected(
    const RType::Network::PlayerConnectedPacket &packet,
    RType::Registry &registry, const ResourceManager &resources,
    RType::Network::NetworkId localLinkedEntity)
{
    _clientToNetwork[packet.clientId] = packet.linkedEntity;

    if (packet.linkedEntity == RType::Network::NO_ENTITY ||
        packet.linkedEntity == localLinkedEntity) {
        return;
    }

    auto it = _networkToEntity.find(packet.linkedEntity);
    if (it == _networkToEntity.end()) {
        auto entity = registry.spawnEntity();
        auto entityId = static_cast<std::size_t>(entity);
        _networkToEntity.insert_or_assign(packet.linkedEntity, entityId);
        _entityToNetwork.insert_or_assign(entityId, packet.linkedEntity);

        registry.addComponent<NetworkEntity>(
            entity, NetworkEntity(packet.linkedEntity, false));
        registry.addComponent<RType::Transform2D>(
            entity, RType::Transform2D({150.0f, 540.0f}));
        registry.addComponent<Player>(entity, Player(3, false));
        registry.addComponent<RType::Physics2D::Rigidbody2D>(
            entity, RType::Physics2D::Rigidbody2D {});
        registry.addComponent<Hitbox>(
            entity, Hitbox(RType::Vector2 {40.0f, 20.0f}));

        try {
            auto &asset = resources.getAsset("player_ship");
            registry.addComponent<RType::Sprite>(
                entity, RType::Sprite(asset.id, 10));
        } catch (...) {
        }
    }
}

void NetworkSyncSystem::handlePlayerDisconnected(
    const RType::Network::PlayerDisconnectedPacket &packet,
    RType::Registry &registry)
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
            registry.killEntity(RType::Registry::entityFromIndex(entityId));
        } catch (...) {
        }
    }
}

void NetworkSyncSystem::handleEntitySpawn(
    const RType::Network::EntitySpawnPacket &packet, RType::Registry &registry,
    const ResourceManager &resources,
    RType::Network::NetworkId localLinkedEntity)
{
    auto it = _networkToEntity.find(packet.networkId);
    RType::Entity entity = it != _networkToEntity.end()
        ? RType::Registry::entityFromIndex(it->second)
        : registry.spawnEntity();
    auto entityId = static_cast<std::size_t>(entity);

    const bool isLocal = packet.networkId == localLinkedEntity;
    if (it == _networkToEntity.end()) {
        _networkToEntity.insert_or_assign(packet.networkId, entityId);
        _entityToNetwork.insert_or_assign(entityId, packet.networkId);
        registry.addComponent<NetworkEntity>(
            entity, NetworkEntity(packet.networkId, isLocal));
    }

    auto &controllables = registry.getComponents<RType::Controllable>();
    auto &players = registry.getComponents<Player>();

    if (isLocal) {
        if (entityId >= controllables.size() ||
            !controllables[entity].has_value()) {
            registry.addComponent<RType::Controllable>(
                entity, RType::Controllable(0));
        }
        if (entityId >= players.size() || !players[entity].has_value()) {
            registry.addComponent<Player>(entity, Player(3, true));
        }
    }

    for (const auto &comp : packet.components) {
        applyComponentData(entity, comp, registry, isLocal);
    }

    // Enemies and projectiles carry ENTITY_KIND and are drawn from their
    // game component. The other entities are the players.
    auto &enemies = registry.getComponents<Enemy>();
    auto &projectiles = registry.getComponents<Projectile>();
    const bool hasKind =
        (entityId < enemies.size() && enemies[entity].has_value()) ||
        (entityId < projectiles.size() && projectiles[entity].has_value());

    if (hasKind) {
        return;
    }
    if (!isLocal &&
        (entityId >= players.size() || !players[entity].has_value())) {
        registry.addComponent<Player>(entity, Player(3, false));
    }

    auto &sprites = registry.getComponents<RType::Sprite>();
    if (entityId >= sprites.size() || !sprites[entity].has_value()) {
        try {
            auto &asset = resources.getAsset("player_ship");
            registry.addComponent<RType::Sprite>(
                entity, RType::Sprite(asset.id, 10));
        } catch (...) {
        }
    }
}

void NetworkSyncSystem::handleEntityDespawn(
    const RType::Network::EntityDespawnPacket &packet,
    RType::Registry &registry)
{
    auto it = _networkToEntity.find(packet.networkId);
    if (it != _networkToEntity.end()) {
        std::size_t entityId = it->second;
        _entityToNetwork.erase(entityId);
        _networkToEntity.erase(it);
        try {
            registry.killEntity(RType::Registry::entityFromIndex(entityId));
        } catch (...) {
        }
    }
}

void NetworkSyncSystem::handleComponentUpdate(
    const RType::Network::ComponentUpdatePacket &packet,
    RType::Registry &registry, RType::Network::NetworkId localLinkedEntity)
{
    for (const auto &[networkId, components] : packet.entities) {
        auto it = _networkToEntity.find(networkId);
        RType::Entity entity = it != _networkToEntity.end()
            ? RType::Registry::entityFromIndex(it->second)
            : registry.spawnEntity();
        auto entityId = static_cast<std::size_t>(entity);

        const bool isLocal = networkId == localLinkedEntity;
        if (it == _networkToEntity.end()) {
            _networkToEntity.insert_or_assign(networkId, entityId);
            _entityToNetwork.insert_or_assign(entityId, networkId);
            registry.addComponent<NetworkEntity>(
                entity, NetworkEntity(networkId, isLocal, packet.tick));
        }

        for (const auto &comp : components) {
            applyComponentData(entity, comp, registry, isLocal);
        }
    }
}

void NetworkSyncSystem::applyComponentData(RType::Entity entity,
    const RType::Network::ComponentData &compData, RType::Registry &registry,
    bool isLocalPlayer)
{
    RType::Network::ByteReader reader(compData.data);
    auto entityId = static_cast<std::size_t>(entity);

    if (compData.typeId == Protocol::ENTITY_KIND) {
        applyEntityKind(entity, reader, registry);
        return;
    }

    switch (static_cast<RType::Network::EngineComponentType>(compData.typeId)) {
        case RType::Network::EngineComponentType::Position: {
            float posX = reader.readFloat();
            float posY = reader.readFloat();
            if (!reader.ok()) {
                return;
            }

            auto &transforms = registry.getComponents<RType::Transform2D>();
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
                registry.addComponent<RType::Transform2D>(
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
                registry.getComponents<RType::Physics2D::Rigidbody2D>();
            if (entityId < velocities.size() &&
                velocities[entity].has_value()) {
                auto &vel = velocities[entity].value();
                vel.velocity.x = vx;
                vel.velocity.y = vy;
            } else {
                RType::Physics2D::Rigidbody2D body;
                body.velocity = {vx, vy};
                registry.emplaceComponent<RType::Physics2D::Rigidbody2D>(
                    entity, body);
            }

            if (hw > 0.0f && hh > 0.0f) {
                auto &hitboxes = registry.getComponents<Hitbox>();
                if (entityId < hitboxes.size() &&
                    hitboxes[entity].has_value()) {
                    auto &hitbox = hitboxes[entity].value();
                    hitbox.size.x = hw;
                    hitbox.size.y = hh;
                } else {
                    registry.addComponent<Hitbox>(
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

void NetworkSyncSystem::applyEntityKind(RType::Entity entity,
    RType::Network::ByteReader &reader, RType::Registry &registry)
{
    auto kind = reader.read<std::uint8_t>();

    if (!reader.ok() || !Protocol::isValidKind(kind)) {
        return;
    }
    switch (static_cast<Protocol::EntityKind>(kind)) {
        case Protocol::EntityKind::Enemy:
            registry.addComponent<Enemy>(entity, Enemy());
            break;
        case Protocol::EntityKind::PlayerLaser:
            registry.addComponent<Projectile>(
                entity, Projectile(true, 1, false, 700.0f));
            break;
        case Protocol::EntityKind::ChargedBeam:
            registry.addComponent<Projectile>(
                entity, Projectile(true, 5, true, 900.0f));
            break;
        case Protocol::EntityKind::EnemyBullet:
            registry.addComponent<Projectile>(
                entity, Projectile(false, 1, false, 450.0f));
            break;
        case Protocol::EntityKind::Player:
            break;
    }
}

} // namespace Game

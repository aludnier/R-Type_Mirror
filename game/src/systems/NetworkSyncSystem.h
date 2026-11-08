/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** NetworkSyncSystem header
*/

#ifndef GAME_NETWORKSYNCSYSTEM_H
#define GAME_NETWORKSYNCSYSTEM_H

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include "audio/IAudioPlayer.h"
#include "ecs/Registry.h"
#include "library/AssetManager.h"
#include "library/InputManager.h"
#include "network/ByteReader.h"
#include "network/NetworkClient.h"

namespace Game {

/**
 * @brief System responsible for synchronizing client inputs, processing server
 * messages, and maintaining an authoritative replica of the ECS on the client
 * with prediction.
 */
class NetworkSyncSystem {
public:
    NetworkSyncSystem() = default;
    ~NetworkSyncSystem() = default;

    /**
     * @brief Initializes the system with references to shared resources.
     * @param registry The client ECS registry.
     * @param resources The resource manager holding game assets.
     */
    void init(RType::Registry &registry, AssetManager &resources);

    /**
     * @brief Updates network client, sends local predicted inputs, and
     * replicates server state.
     * @param input The input manager capturing player inputs.
     * @param client The network client managing the connection.
     * @param audio Optional audio player for sound effects.
     */
    void syncWithServer(const InputManager &input, NetworkClient &client,
        RType::IAudioPlayer *audio = nullptr);

    /**
     * @brief Clears all replicated entities and resets network mappings on
     * disconnect.
     */
    void reset();

    /**
     * @brief Finds the local ECS entity associated with a given NetworkId.
     */
    [[nodiscard]] std::optional<RType::Entity> findEntity(
        RType::Network::NetworkId id) const;

    void processServerMessages(
        const std::vector<NetworkClient::ServerMessage> &messages,
        RType::Network::NetworkId localLinkedEntity,
        RType::IAudioPlayer *audio = nullptr);

    /**
     * @brief Tint of a player's ship, from its client id: white (no tint),
     *        orange, yellow and purple, which stay distinct for the common
     *        types of colour blindness.
     */
    [[nodiscard]] static std::array<std::uint8_t, 3> playerTint(
        RType::Network::ClientId clientId);

    /** @return Whether the server despawned the local player's ship. */
    [[nodiscard]] bool isGameOver() const;

private:
    struct ServerMessageVisitor;

    /** @brief Adds the level ship sprite to a player's entity. */
    void addShipSprite(RType::Entity entity);

    /** @brief Gives each player's sprite the tint of its client id. */
    void applyPlayerColors();

    void handleAcceptConnect(const RType::Network::AcceptConnectPacket &packet);

    void handlePlayerConnected(
        const RType::Network::PlayerConnectedPacket &packet,
        RType::Network::NetworkId localLinkedEntity);

    void handlePlayerDisconnected(
        const RType::Network::PlayerDisconnectedPacket &packet);

    void handleEntitySpawn(const RType::Network::EntitySpawnPacket &packet,
        RType::Network::NetworkId localLinkedEntity);

    void handleEntityDespawn(const RType::Network::EntityDespawnPacket &packet);

    void handleComponentUpdate(
        const RType::Network::ComponentUpdatePacket &packet,
        RType::Network::NetworkId localLinkedEntity);

    void applyComponentData(RType::Entity entity,
        const RType::Network::ComponentData &compData, bool isLocalPlayer);

    /** @brief Adds the game component matching an ENTITY_KIND component. */
    void applyEntityKind(
        RType::Entity entity, RType::Network::ByteReader &reader);

    static void sendLocalPlayerInput(
        const InputManager &input, NetworkClient &client);

    std::optional<std::reference_wrapper<RType::Registry>> _registry =
        std::nullopt;
    std::optional<std::reference_wrapper<AssetManager>> _resources =
        std::nullopt;

    std::unordered_map<RType::Network::NetworkId, std::size_t>
        _networkToEntity {};
    std::unordered_map<std::size_t, RType::Network::NetworkId>
        _entityToNetwork {};
    std::unordered_map<RType::Network::ClientId, RType::Network::NetworkId>
        _clientToNetwork {};
    bool _wasConnected = false;
    RType::Network::NetworkId _localNetworkId = RType::Network::NO_ENTITY;
    bool _gameOver = false;
};

} // namespace Game

#endif // GAME_NETWORKSYNCSYSTEM_H

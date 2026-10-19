/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** NetworkSyncSystem header
*/

#ifndef GAME_NETWORKSYNCSYSTEM_H
#define GAME_NETWORKSYNCSYSTEM_H

#include "ecs/Registry.h"
#include "library/InputManager.h"
#include "library/RessourceManager.h"
#include "network/NetworkClient.h"
#include "components/NetworkEntity.h"
#include "network/ByteReader.h"
#include <unordered_map>

namespace Game {

    /**
     * @brief System responsible for synchronizing client inputs, processing server messages,
     *        and maintaining an authoritative replica of the ECS on the client with prediction.
     */
    class NetworkSyncSystem {
    public:
        NetworkSyncSystem() = default;
        ~NetworkSyncSystem() = default;

        /**
         * @brief Updates network client, sends local predicted inputs, and replicates server state.
         * @param registry The client ECS registry.
         * @param input The input manager capturing player inputs.
         * @param resources The resource manager holding game assets.
         * @param client The network client managing the connection.
         */
        void syncWithServer(
            RType::Registry &registry,
            InputManager &input,
            ResourceManager &resources,
            NetworkClient &client
        );

        /**
         * @brief Clears all replicated entities and resets network mappings on disconnect.
         */
        void reset(RType::Registry &registry);

        /**
         * @brief Finds the local ECS entity associated with a given NetworkId.
         */
        [[nodiscard]] std::optional<RType::Entity> findEntity(RType::Network::NetworkId id) const;

    private:
        void handleAcceptConnect(
            const RType::Network::AcceptConnectPacket &packet,
            RType::Registry &registry,
            ResourceManager &resources
        );

        void handlePlayerConnected(
            const RType::Network::PlayerConnectedPacket &packet,
            RType::Registry &registry,
            ResourceManager &resources,
            RType::Network::NetworkId localLinkedEntity
        );

        void handlePlayerDisconnected(
            const RType::Network::PlayerDisconnectedPacket &packet,
            RType::Registry &registry
        );

        void handleEntitySpawn(
            const RType::Network::EntitySpawnPacket &packet,
            RType::Registry &registry,
            ResourceManager &resources,
            RType::Network::NetworkId localLinkedEntity
        );

        void handleEntityDespawn(
            const RType::Network::EntityDespawnPacket &packet,
            RType::Registry &registry
        );

        void handleComponentUpdate(
            const RType::Network::ComponentUpdatePacket &packet,
            RType::Registry &registry,
            RType::Network::NetworkId localLinkedEntity
        );

        void applyComponentData(
            RType::Entity entity,
            const RType::Network::ComponentData &compData,
            RType::Registry &registry,
            bool isLocalPlayer
        );

        /** @brief Adds the game component matching an ENTITY_KIND component. */
        void applyEntityKind(
            RType::Entity entity,
            RType::Network::ByteReader &reader,
            RType::Registry &registry
        );

        void processServerMessages(
            const std::vector<NetworkClient::ServerMessage> &messages,
            RType::Registry &registry,
            ResourceManager &resources,
            RType::Network::NetworkId localLinkedEntity
        );

        void sendLocalPlayerInput(
            InputManager &input,
            NetworkClient &client
        );

        std::unordered_map<RType::Network::NetworkId, std::size_t> _networkToEntity;
        std::unordered_map<std::size_t, RType::Network::NetworkId> _entityToNetwork;
        std::unordered_map<RType::Network::ClientId, RType::Network::NetworkId> _clientToNetwork;
        bool _wasConnected = false;
    };

} // namespace Game

#endif // GAME_NETWORKSYNCSYSTEM_H

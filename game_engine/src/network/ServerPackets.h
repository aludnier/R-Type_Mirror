/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ServerPackets.h
*/

#ifndef SERVERPACKETS_H
#define SERVERPACKETS_H

#include <cstdint>
#include <string>
#include <vector>

#include "ByteReader.h"
#include "ByteWriter.h"
#include "Protocol.h"

/**
 * @file ServerPackets.h
 * @brief Payloads sent by the server (docs/protocol.md, section 6.2).
 */
namespace RType::Network {
    /**
     * @brief One serialized component. The layout of @p data is defined by
     *        the component type itself.
     */
    struct ComponentData {
        ComponentTypeId typeId = 0;
        std::vector<std::uint8_t> data;

        void serialize(ByteWriter &writer) const;
        static ComponentData deserialize(ByteReader &reader);
        bool operator==(const ComponentData &other) const = default;
    };

    /**
     * @brief Components of one entity inside a COMPONENT_UPDATE.
     */
    struct EntityComponents {
        NetworkId networkId = NO_ENTITY;
        /** @brief At most 255 components. */
        std::vector<ComponentData> components;

        /** @throws std::length_error If there are more than 255 components. */
        void serialize(ByteWriter &writer) const;
        static EntityComponents deserialize(ByteReader &reader);
        bool operator==(const EntityComponents &other) const = default;
    };

    struct AcceptConnectPacket {
        static constexpr auto TYPE = MessageType::AcceptConnect;
        static constexpr bool RELIABLE = true;

        ClientId clientId = SERVER_CLIENT_ID;
        NetworkId linkedEntity = NO_ENTITY;
        std::uint16_t tickRate = DEFAULT_TICK_RATE;

        void serialize(ByteWriter &writer) const;
        static AcceptConnectPacket deserialize(ByteReader &reader);
        bool operator==(const AcceptConnectPacket &other) const = default;
    };

    struct ForceDisconnectPacket {
        static constexpr auto TYPE = MessageType::ForceDisconnect;
        // Not acked: the session is closed right after it is sent.
        static constexpr bool RELIABLE = false;

        DisconnectReason reason = DisconnectReason::Kicked;
        std::string message;

        void serialize(ByteWriter &writer) const;
        static ForceDisconnectPacket deserialize(ByteReader &reader);
        bool operator==(const ForceDisconnectPacket &other) const = default;
    };

    struct PlayerConnectedPacket {
        static constexpr auto TYPE = MessageType::PlayerConnected;
        static constexpr bool RELIABLE = true;

        ClientId clientId = SERVER_CLIENT_ID;
        NetworkId linkedEntity = NO_ENTITY;
        std::string playerName;

        void serialize(ByteWriter &writer) const;
        static PlayerConnectedPacket deserialize(ByteReader &reader);
        bool operator==(const PlayerConnectedPacket &other) const = default;
    };

    struct PlayerDisconnectedPacket {
        static constexpr auto TYPE = MessageType::PlayerDisconnected;
        static constexpr bool RELIABLE = true;

        ClientId clientId = SERVER_CLIENT_ID;
        DisconnectReason reason = DisconnectReason::Left;

        void serialize(ByteWriter &writer) const;
        static PlayerDisconnectedPacket deserialize(ByteReader &reader);
        bool operator==(const PlayerDisconnectedPacket &other) const = default;
    };

    struct EntitySpawnPacket {
        static constexpr auto TYPE = MessageType::EntitySpawn;
        static constexpr bool RELIABLE = true;

        NetworkId networkId = NO_ENTITY;
        /** @brief At most 255 components. */
        std::vector<ComponentData> components;

        /** @throws std::length_error If there are more than 255 components. */
        void serialize(ByteWriter &writer) const;
        static EntitySpawnPacket deserialize(ByteReader &reader);
        bool operator==(const EntitySpawnPacket &other) const = default;
    };

    struct EntityDespawnPacket {
        static constexpr auto TYPE = MessageType::EntityDespawn;
        static constexpr bool RELIABLE = true;

        NetworkId networkId = NO_ENTITY;

        void serialize(ByteWriter &writer) const;
        static EntityDespawnPacket deserialize(ByteReader &reader);
        bool operator==(const EntityDespawnPacket &other) const = default;
    };

    /**
     * @brief New values of the components of several entities, so that one
     *        datagram carries as many entities as fit in MAX_PAYLOAD_SIZE.
     *        The sender splits larger updates into several packets.
     */
    struct ComponentUpdatePacket {
        static constexpr auto TYPE = MessageType::ComponentUpdate;
        static constexpr bool RELIABLE = false;

        Tick tick = 0;
        /** @brief At most 255 entities. */
        std::vector<EntityComponents> entities;

        /** @throws std::length_error If there are more than 255 entities. */
        void serialize(ByteWriter &writer) const;
        static ComponentUpdatePacket deserialize(ByteReader &reader);
        bool operator==(const ComponentUpdatePacket &other) const = default;
    };

    /**
     * @brief Splits the update of @p entities into as few COMPONENT_UPDATE
     *        as possible, each one fitting in MAX_PAYLOAD_SIZE and holding at
     *        most 255 entities (docs/protocol.md, section 6.2).
     *
     * Entities keep their order. An entity whose components do not fit in
     * what is left of a packet is continued in the next one, under the same
     * network id. Every packet carries @p tick.
     *
     * @return No packet if @p entities is empty.
     * @throws std::length_error If one component alone cannot fit in a
     *         packet.
     */
    std::vector<ComponentUpdatePacket> splitComponentUpdate(
        Tick tick, const std::vector<EntityComponents> &entities);

    struct HeartbeatReplyPacket {
        static constexpr auto TYPE = MessageType::HeartbeatReply;
        static constexpr bool RELIABLE = false;

        /** @brief Copied from the HEARTBEAT being answered. */
        std::uint32_t clientTime = 0;
        Tick serverTick = 0;

        void serialize(ByteWriter &writer) const;
        static HeartbeatReplyPacket deserialize(ByteReader &reader);
        bool operator==(const HeartbeatReplyPacket &other) const = default;
    };
} // namespace RType::Network

#endif // SERVERPACKETS_H

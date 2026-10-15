/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ServerPackets.cpp
*/

#include "ServerPackets.h"

#include <limits>
#include <stdexcept>

namespace RType::Network {
    namespace {
        DisconnectReason readReason(ByteReader &reader)
        {
            auto reason = reader.read<DisconnectReason>();

            if (reason > DisconnectReason::ServerShutdown) {
                reader.fail();
            }
            return reason;
        }

        /** @brief Writes a u8 count followed by the elements. */
        template<class T>
        void writeList(ByteWriter &writer, const std::vector<T> &elements)
        {
            if (elements.size() > std::numeric_limits<std::uint8_t>::max()) {
                throw std::length_error("more than 255 elements in a list");
            }
            writer.write(static_cast<std::uint8_t>(elements.size()));
            for (const auto &element : elements) {
                element.serialize(writer);
            }
        }

        template<class T>
        std::vector<T> readList(ByteReader &reader)
        {
            auto count = reader.read<std::uint8_t>();
            std::vector<T> elements;

            elements.reserve(count);
            for (std::uint8_t i = 0; i < count && reader.ok(); ++i) {
                elements.push_back(T::deserialize(reader));
            }
            return elements;
        }
    } // namespace

    void ComponentData::serialize(ByteWriter &writer) const
    {
        writer.write(typeId);
        writer.writeBytes(data);
    }

    ComponentData ComponentData::deserialize(ByteReader &reader)
    {
        ComponentData component;

        component.typeId = reader.read<ComponentTypeId>();
        component.data = reader.readBytes();
        return component;
    }

    void EntityComponents::serialize(ByteWriter &writer) const
    {
        writer.write(networkId);
        writeList(writer, components);
    }

    EntityComponents EntityComponents::deserialize(ByteReader &reader)
    {
        EntityComponents entity;

        entity.networkId = reader.read<NetworkId>();
        entity.components = readList<ComponentData>(reader);
        return entity;
    }

    void AcceptConnectPacket::serialize(ByteWriter &writer) const
    {
        writer.write(clientId);
        writer.write(linkedEntity);
        writer.write(tickRate);
    }

    AcceptConnectPacket AcceptConnectPacket::deserialize(ByteReader &reader)
    {
        AcceptConnectPacket packet;

        packet.clientId = reader.read<ClientId>();
        packet.linkedEntity = reader.read<NetworkId>();
        packet.tickRate = reader.read<std::uint16_t>();
        return packet;
    }

    void ForceDisconnectPacket::serialize(ByteWriter &writer) const
    {
        writer.write(reason);
        writer.writeString(message);
    }

    ForceDisconnectPacket ForceDisconnectPacket::deserialize(ByteReader &reader)
    {
        ForceDisconnectPacket packet;

        packet.reason = readReason(reader);
        packet.message = reader.readString();
        return packet;
    }

    void PlayerConnectedPacket::serialize(ByteWriter &writer) const
    {
        writer.write(clientId);
        writer.write(linkedEntity);
        writer.writeString(playerName);
    }

    PlayerConnectedPacket PlayerConnectedPacket::deserialize(ByteReader &reader)
    {
        PlayerConnectedPacket packet;

        packet.clientId = reader.read<ClientId>();
        packet.linkedEntity = reader.read<NetworkId>();
        packet.playerName = reader.readString();
        return packet;
    }

    void PlayerDisconnectedPacket::serialize(ByteWriter &writer) const
    {
        writer.write(clientId);
        writer.write(reason);
    }

    PlayerDisconnectedPacket PlayerDisconnectedPacket::deserialize(
        ByteReader &reader)
    {
        PlayerDisconnectedPacket packet;

        packet.clientId = reader.read<ClientId>();
        packet.reason = readReason(reader);
        return packet;
    }

    void EntitySpawnPacket::serialize(ByteWriter &writer) const
    {
        writer.write(networkId);
        writeList(writer, components);
    }

    EntitySpawnPacket EntitySpawnPacket::deserialize(ByteReader &reader)
    {
        EntitySpawnPacket packet;

        packet.networkId = reader.read<NetworkId>();
        packet.components = readList<ComponentData>(reader);
        return packet;
    }

    void EntityDespawnPacket::serialize(ByteWriter &writer) const
    {
        writer.write(networkId);
    }

    EntityDespawnPacket EntityDespawnPacket::deserialize(ByteReader &reader)
    {
        return {.networkId = reader.read<NetworkId>()};
    }

    void ComponentUpdatePacket::serialize(ByteWriter &writer) const
    {
        writer.write(tick);
        writeList(writer, entities);
    }

    ComponentUpdatePacket ComponentUpdatePacket::deserialize(ByteReader &reader)
    {
        ComponentUpdatePacket packet;

        packet.tick = reader.read<Tick>();
        packet.entities = readList<EntityComponents>(reader);
        return packet;
    }

    std::vector<ComponentUpdatePacket> splitComponentUpdate(
        Tick tick, const std::vector<EntityComponents> &entities)
    {
        // Encoded sizes, see docs/protocol.md, section 6.2: tick and
        // entity_count, then network_id and component_count, then type_id
        // and the u16 length of data.
        constexpr std::size_t packetOverhead = sizeof(Tick) + 1;
        constexpr std::size_t entityOverhead = sizeof(NetworkId) + 1;
        constexpr std::size_t componentOverhead =
            sizeof(ComponentTypeId) + sizeof(std::uint16_t);
        constexpr std::size_t maxListSize =
            std::numeric_limits<std::uint8_t>::max();
        std::vector<ComponentUpdatePacket> packets;
        // Payload size of packets.back().
        std::size_t size = 0;

        auto addEntity = [&](NetworkId networkId, bool newPacket) {
            if (newPacket || packets.empty() ||
                packets.back().entities.size() == maxListSize ||
                size + entityOverhead > MAX_PAYLOAD_SIZE) {
                packets.push_back({.tick = tick, .entities = {}});
                size = packetOverhead;
            }
            packets.back().entities.push_back(
                {.networkId = networkId, .components = {}});
            size += entityOverhead;
        };

        for (const auto &entity : entities) {
            addEntity(entity.networkId, false);
            for (const auto &component : entity.components) {
                std::size_t componentSize =
                    componentOverhead + component.data.size();

                if (packetOverhead + entityOverhead + componentSize >
                    MAX_PAYLOAD_SIZE) {
                    throw std::length_error(
                        "component too large for a COMPONENT_UPDATE");
                }
                auto &current = packets.back().entities;

                if (size + componentSize > MAX_PAYLOAD_SIZE ||
                    current.back().components.size() == maxListSize) {
                    // Nothing was added to this entity yet: move it to the
                    // next packet instead of sending it empty.
                    if (current.back().components.empty()) {
                        current.pop_back();
                    }
                    addEntity(entity.networkId, true);
                }
                packets.back().entities.back().components.push_back(component);
                size += componentSize;
            }
        }
        return packets;
    }

    void HeartbeatReplyPacket::serialize(ByteWriter &writer) const
    {
        writer.write(clientTime);
        writer.write(serverTick);
    }

    HeartbeatReplyPacket HeartbeatReplyPacket::deserialize(ByteReader &reader)
    {
        HeartbeatReplyPacket packet;

        packet.clientTime = reader.read<std::uint32_t>();
        packet.serverTick = reader.read<Tick>();
        return packet;
    }
} // namespace RType::Network

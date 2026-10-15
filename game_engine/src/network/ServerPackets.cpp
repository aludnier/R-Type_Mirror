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

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ClientPackets.cpp
*/

#include "ClientPackets.h"

namespace RType::Network {
    void AskConnectPacket::serialize(ByteWriter &writer) const
    {
        writer.writeString(playerName);
    }

    AskConnectPacket AskConnectPacket::deserialize(ByteReader &reader)
    {
        return {.playerName = reader.readString()};
    }

    void ControllablePacket::serialize(ByteWriter &writer) const
    {
        writer.write(inputSequence);
        writer.write(actions);
    }

    ControllablePacket ControllablePacket::deserialize(ByteReader &reader)
    {
        ControllablePacket packet;

        packet.inputSequence = reader.read<std::uint32_t>();
        packet.actions = reader.read<std::uint32_t>();
        return packet;
    }

    void HeartbeatPacket::serialize(ByteWriter &writer) const
    {
        writer.write(clientTime);
    }

    HeartbeatPacket HeartbeatPacket::deserialize(ByteReader &reader)
    {
        return {.clientTime = reader.read<std::uint32_t>()};
    }
} // namespace RType::Network

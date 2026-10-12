/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ClientPackets.cpp
*/

#include "ClientPackets.h"

namespace RType::Network {
    namespace {
        constexpr std::uint8_t DIRECTION_UP = 1 << 0;
        constexpr std::uint8_t DIRECTION_DOWN = 1 << 1;
        constexpr std::uint8_t DIRECTION_LEFT = 1 << 2;
        constexpr std::uint8_t DIRECTION_RIGHT = 1 << 3;
        constexpr std::uint8_t DIRECTION_MASK = 0x0F;
    } // namespace

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
        auto directions = static_cast<std::uint8_t>((up ? DIRECTION_UP : 0)
            | (down ? DIRECTION_DOWN : 0) | (left ? DIRECTION_LEFT : 0)
            | (right ? DIRECTION_RIGHT : 0));

        writer.write(inputSequence);
        writer.write(directions);
        writer.write(actions);
    }

    ControllablePacket ControllablePacket::deserialize(ByteReader &reader)
    {
        ControllablePacket packet;

        packet.inputSequence = reader.read<std::uint32_t>();
        auto directions = reader.read<std::uint8_t>();

        if ((directions & ~DIRECTION_MASK) != 0) {
            reader.fail();
        }
        packet.up = (directions & DIRECTION_UP) != 0;
        packet.down = (directions & DIRECTION_DOWN) != 0;
        packet.left = (directions & DIRECTION_LEFT) != 0;
        packet.right = (directions & DIRECTION_RIGHT) != 0;
        packet.actions = reader.read<std::uint16_t>();
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

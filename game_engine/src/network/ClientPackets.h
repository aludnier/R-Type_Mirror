/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ClientPackets.h
*/

#ifndef CLIENTPACKETS_H
#define CLIENTPACKETS_H

#include <cstdint>
#include <string>

#include "ByteReader.h"
#include "ByteWriter.h"
#include "Protocol.h"

/**
 * @file ClientPackets.h
 * @brief Payloads sent by the client (docs/protocol.md, section 6.1).
 */
namespace RType::Network {
    struct AskConnectPacket {
        static constexpr MessageType TYPE = MessageType::AskConnect;
        // Not acked: the client resends it until the server answers.
        static constexpr bool RELIABLE = false;

        std::string playerName;

        void serialize(ByteWriter &writer) const;
        static AskConnectPacket deserialize(ByteReader &reader);
        bool operator==(const AskConnectPacket &other) const = default;
    };

    struct AskDisconnectPacket {
        static constexpr MessageType TYPE = MessageType::AskDisconnect;
        static constexpr bool RELIABLE = false;

        static void serialize(ByteWriter & /* writer */)
        {
        }

        static AskDisconnectPacket deserialize(ByteReader & /* reader */)
        {
            return {};
        }

        bool operator==(const AskDisconnectPacket &other) const = default;
    };

    /**
     * @brief State of the player's controls, never raw key events.
     */
    struct ControllablePacket {
        static constexpr MessageType TYPE = MessageType::Controllable;
        static constexpr bool RELIABLE = false;

        std::uint32_t inputSequence = 0;
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
        /** @brief Bitmask of game-defined actions (e.g. bit 0 = shoot). */
        std::uint16_t actions = 0;

        void serialize(ByteWriter &writer) const;
        static ControllablePacket deserialize(ByteReader &reader);
        bool operator==(const ControllablePacket &other) const = default;
    };

    struct HeartbeatPacket {
        static constexpr MessageType TYPE = MessageType::Heartbeat;
        static constexpr bool RELIABLE = false;

        /** @brief Client clock in milliseconds, echoed by the server. */
        std::uint32_t clientTime = 0;

        void serialize(ByteWriter &writer) const;
        static HeartbeatPacket deserialize(ByteReader &reader);
        bool operator==(const HeartbeatPacket &other) const = default;
    };
} // namespace RType::Network

#endif // CLIENTPACKETS_H

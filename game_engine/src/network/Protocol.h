/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Protocol.h
*/

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

/**
 * @file Protocol.h
 * @brief Constants and identifiers of the network protocol.
 *
 * Every value here mirrors docs/protocol.md: keep both in sync.
 */
namespace RType::Network {
    using ClientId = std::uint16_t;
    using NetworkId = std::uint32_t;
    using Tick = std::uint32_t;
    using Sequence = std::uint16_t;
    using ComponentTypeId = std::uint16_t;

    constexpr std::uint16_t PROTOCOL_MAGIC = 0x5452; // "RT" on the wire
    constexpr std::uint8_t PROTOCOL_VERSION = 1;

    constexpr std::size_t HEADER_SIZE = 15;
    constexpr std::size_t MAX_PACKET_SIZE = 1200;
    constexpr std::size_t MAX_PAYLOAD_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;
    constexpr std::size_t MAX_STRING_SIZE = 255;

    constexpr ClientId SERVER_CLIENT_ID = 0;
    constexpr NetworkId NO_ENTITY = 0;
    constexpr ComponentTypeId GAME_COMPONENT_TYPE_MIN = 0x8000;

    /** @brief Header flag: the sender resends the packet until it is acked. */
    constexpr std::uint8_t FLAG_RELIABLE = 0x01;

    constexpr std::uint16_t DEFAULT_TICK_RATE = 60;
    constexpr std::chrono::milliseconds HEARTBEAT_INTERVAL {1000};
    constexpr std::chrono::milliseconds CONNECTION_TIMEOUT {5000};
    constexpr std::chrono::milliseconds RESEND_TIMEOUT {100};
    constexpr std::chrono::milliseconds CONNECT_RETRY_INTERVAL {500};
    constexpr std::size_t CONNECT_MAX_ATTEMPTS = 10;

    /**
     * @brief Identifies the payload of a packet.
     *
     * 0x01-0x3F: engine, client to server.
     * 0x40-0x7F: engine, server to client.
     * 0x80-0xFF: game-defined, see GAME_MESSAGE_TYPE_MIN.
     */
    enum class MessageType : std::uint8_t {
        Invalid = 0x00,

        AskConnect = 0x01,
        AskDisconnect = 0x02,
        Controllable = 0x03,
        Heartbeat = 0x04,

        AcceptConnect = 0x40,
        ForceDisconnect = 0x41,
        PlayerConnected = 0x42,
        PlayerDisconnected = 0x43,
        EntitySpawn = 0x44,
        EntityDespawn = 0x45,
        ComponentUpdate = 0x46,
        HeartbeatReply = 0x47,
    };

    constexpr std::uint8_t GAME_MESSAGE_TYPE_MIN = 0x80;

    enum class DisconnectReason : std::uint8_t {
        Left = 0,
        Timeout = 1,
        Kicked = 2,
        ServerFull = 3,
        BadVersion = 4,
        ServerShutdown = 5,
    };

    constexpr bool isGameMessageType(MessageType type)
    {
        return static_cast<std::uint8_t>(type) >= GAME_MESSAGE_TYPE_MIN;
    }

    /**
     * @brief Tells whether @p lhs was sent after @p rhs, handling the
     *        wrap-around of the 16-bit sequence counter.
     */
    constexpr bool isSequenceMoreRecent(Sequence lhs, Sequence rhs)
    {
        constexpr Sequence half = 0x8000;

        return (lhs > rhs && lhs - rhs <= half)
            || (lhs < rhs && rhs - lhs > half);
    }

    /**
     * @brief Integer-like types written on the wire as little-endian
     *        unsigned integers of the same size.
     */
    template<class T>
    concept WireInteger = (std::integral<T> && !std::same_as<T, bool>)
        || std::is_enum_v<T>;

    template<WireInteger T>
    using wire_raw_t = std::make_unsigned_t<typename std::conditional_t<
        std::is_enum_v<T>, std::underlying_type<T>,
        std::type_identity<T>>::type>;
} // namespace RType::Network

#endif // PROTOCOL_H

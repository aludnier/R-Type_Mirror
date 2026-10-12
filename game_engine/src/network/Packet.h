/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Packet.h
*/

#ifndef PACKET_H
#define PACKET_H

#include <concepts>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "ByteReader.h"
#include "ByteWriter.h"
#include "PacketHeader.h"

namespace RType::Network {
    /**
     * @brief A header and its still-encoded payload: exactly one datagram.
     */
    struct Packet {
        PacketHeader header;
        std::vector<std::uint8_t> payload;

        /**
         * @brief Encodes the packet, with header.payloadSize set from the
         *        payload.
         * @throws std::length_error If the payload exceeds MAX_PAYLOAD_SIZE.
         */
        [[nodiscard]] std::vector<std::uint8_t> serialize() const;

        /**
         * @brief Decodes a received datagram. Never throws.
         * @return std::nullopt if the datagram size, magic or payload size is
         *         invalid. The version is not checked: the caller decides
         *         (see docs/protocol.md, section 4).
         */
        static std::optional<Packet> deserialize(
            std::span<const std::uint8_t> datagram);
    };

    /**
     * @brief A typed message payload (e.g. AskConnectPacket).
     *
     * deserialize() reads its fields from the reader and calls
     * ByteReader::fail() on invalid values. Size checks are done by
     * readPayload().
     */
    template<class P>
    concept PacketPayload =
        requires(const P &payload, ByteWriter &writer, ByteReader &reader) {
            { P::TYPE } -> std::convertible_to<MessageType>;
            { P::RELIABLE } -> std::convertible_to<bool>;
            { payload.serialize(writer) } -> std::same_as<void>;
            { P::deserialize(reader) } -> std::same_as<P>;
        };

    /**
     * @brief Builds a packet from a payload. Sequence and ack fields are left
     *        to the connection that sends it.
     */
    template<PacketPayload P>
    Packet makePacket(const P &payload)
    {
        ByteWriter writer;
        Packet packet;

        payload.serialize(writer);
        packet.header.type = P::TYPE;
        packet.header.flags = P::RELIABLE ? FLAG_RELIABLE : 0;
        packet.payload = writer.release();
        return packet;
    }

    /**
     * @brief Decodes the payload of @p packet as a @p P.
     * @return std::nullopt if the type differs, or if the payload is
     *         truncated, has trailing bytes or holds an invalid value.
     */
    template<PacketPayload P>
    std::optional<P> readPayload(const Packet &packet)
    {
        if (packet.header.type != P::TYPE) {
            return std::nullopt;
        }
        ByteReader reader(packet.payload);
        P payload = P::deserialize(reader);

        if (!reader.ok() || !reader.atEnd()) {
            return std::nullopt;
        }
        return payload;
    }
} // namespace RType::Network

#endif // PACKET_H

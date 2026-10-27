/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PacketHeader.h
*/

#ifndef PACKETHEADER_H
#define PACKETHEADER_H

#include <cstdint>

#include "ByteReader.h"
#include "ByteWriter.h"
#include "Protocol.h"

namespace RType::Network {
/**
 * @brief 15-byte header at the start of every datagram
 *        (docs/protocol.md, section 4).
 */
struct PacketHeader {
    std::uint16_t magic = PROTOCOL_MAGIC;
    std::uint8_t version = PROTOCOL_VERSION;
    MessageType type = MessageType::Invalid;
    std::uint8_t flags = 0;
    Sequence sequence = 0;
    Sequence ack = 0;
    std::uint32_t ackBits = 0;
    std::uint16_t payloadSize = 0;

    void serialize(ByteWriter &writer) const;

    /** @note Does not validate the fields, see Packet::deserialize(). */
    static PacketHeader deserialize(ByteReader &reader);

    [[nodiscard]] bool isReliable() const;
};
} // namespace RType::Network

#endif // PACKETHEADER_H

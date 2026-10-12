/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PacketHeader.cpp
*/

#include "PacketHeader.h"

namespace RType::Network {
    void PacketHeader::serialize(ByteWriter &writer) const
    {
        writer.write(magic);
        writer.write(version);
        writer.write(type);
        writer.write(flags);
        writer.write(sequence);
        writer.write(ack);
        writer.write(ackBits);
        writer.write(payloadSize);
    }

    PacketHeader PacketHeader::deserialize(ByteReader &reader)
    {
        PacketHeader header;

        header.magic = reader.read<std::uint16_t>();
        header.version = reader.read<std::uint8_t>();
        header.type = reader.read<MessageType>();
        header.flags = reader.read<std::uint8_t>();
        header.sequence = reader.read<Sequence>();
        header.ack = reader.read<Sequence>();
        header.ackBits = reader.read<std::uint32_t>();
        header.payloadSize = reader.read<std::uint16_t>();
        return header;
    }

    bool PacketHeader::isReliable() const
    {
        return (flags & FLAG_RELIABLE) != 0;
    }
} // namespace RType::Network

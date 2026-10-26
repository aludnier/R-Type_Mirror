/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Packet.cpp
*/

#include "Packet.h"

#include <stdexcept>

namespace RType::Network {
std::vector<std::uint8_t> Packet::serialize() const
{
    if (payload.size() > MAX_PAYLOAD_SIZE) {
        throw std::length_error("Packet: payload exceeds MAX_PAYLOAD_SIZE");
    }
    ByteWriter writer;
    PacketHeader sizedHeader = header;

    sizedHeader.payloadSize = static_cast<std::uint16_t>(payload.size());
    sizedHeader.serialize(writer);
    writer.writeRaw(payload);
    return writer.release();
}

std::optional<Packet> Packet::deserialize(
    std::span<const std::uint8_t> datagram)
{
    if (datagram.size() < HEADER_SIZE || datagram.size() > MAX_PACKET_SIZE) {
        return std::nullopt;
    }
    ByteReader reader(datagram);
    Packet packet;

    packet.header = PacketHeader::deserialize(reader);
    if (!reader.ok() || packet.header.magic != PROTOCOL_MAGIC ||
        packet.header.payloadSize != reader.remaining()) {
        return std::nullopt;
    }
    auto payload = datagram.subspan(HEADER_SIZE);

    packet.payload.assign(payload.begin(), payload.end());
    return packet;
}
} // namespace RType::Network

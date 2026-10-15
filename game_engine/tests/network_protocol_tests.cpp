#include <criterion/criterion.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "network/ByteReader.h"
#include "network/ByteWriter.h"
#include "network/ClientPackets.h"
#include "network/Packet.h"
#include "network/PacketHeader.h"
#include "network/Protocol.h"
#include "network/ServerPackets.h"

using namespace RType::Network;

namespace {
    using Bytes = std::vector<std::uint8_t>;

    template<PacketPayload P>
    std::optional<P> roundTrip(const P &payload)
    {
        auto datagram = makePacket(payload).serialize();
        auto packet = Packet::deserialize(datagram);

        if (!packet) {
            return std::nullopt;
        }
        return readPayload<P>(*packet);
    }

    template<PacketPayload P>
    std::optional<P> decodeRaw(const Bytes &payload)
    {
        Packet packet;

        packet.header.type = P::TYPE;
        packet.payload = payload;
        return readPayload<P>(packet);
    }

    template<PacketPayload P>
    void decodeAs(Packet packet)
    {
        packet.header.type = P::TYPE;
        (void) readPayload<P>(packet);
    }

    void decodeAsEveryPayload(const Packet &packet)
    {
        decodeAs<AskConnectPacket>(packet);
        decodeAs<AskDisconnectPacket>(packet);
        decodeAs<ControllablePacket>(packet);
        decodeAs<HeartbeatPacket>(packet);
        decodeAs<AcceptConnectPacket>(packet);
        decodeAs<ForceDisconnectPacket>(packet);
        decodeAs<PlayerConnectedPacket>(packet);
        decodeAs<PlayerDisconnectedPacket>(packet);
        decodeAs<EntitySpawnPacket>(packet);
        decodeAs<EntityDespawnPacket>(packet);
        decodeAs<ComponentUpdatePacket>(packet);
        decodeAs<HeartbeatReplyPacket>(packet);
    }
} // namespace

Test(ByteWriter, writesIntegersInLittleEndian)
{
    ByteWriter writer;

    writer.write(std::uint8_t {0x01});
    writer.write(std::uint16_t {0x0203});
    writer.write(std::uint32_t {0x04050607});
    writer.write(std::int16_t {-2});
    writer.write(MessageType::Heartbeat);

    cr_assert(writer.data() ==
        (Bytes {0x01, 0x03, 0x02, 0x07, 0x06, 0x05, 0x04, 0xFE, 0xFF, 0x04}));
}

Test(ByteWriter, writesLengthPrefixedStringsAndBytes)
{
    ByteWriter writer;

    writer.writeString("abc");
    writer.writeBytes(Bytes {9, 8});

    cr_assert(writer.data() == (Bytes {3, 'a', 'b', 'c', 2, 0, 9, 8}));
}

Test(ByteWriter, rejectsStringsLongerThan255Bytes)
{
    ByteWriter writer;

    writer.writeString(std::string(255, 'x'));
    cr_assert_eq(writer.data().size(), 256u);
    cr_assert_throw(
        writer.writeString(std::string(256, 'x')), std::length_error);
}

Test(ByteWriter, releaseLeavesTheWriterEmpty)
{
    ByteWriter writer;

    writer.write(std::uint32_t {1});
    auto bytes = writer.release();

    cr_assert_eq(bytes.size(), 4u);
    cr_assert(writer.data().empty());
}

Test(ByteReader, readsBackWhatWasWritten)
{
    ByteWriter writer;

    writer.write(std::uint8_t {200});
    writer.write(std::uint16_t {60000});
    writer.write(std::int32_t {-123456});
    writer.writeFloat(3.5F);
    writer.writeString("R-Type");
    writer.writeBytes(Bytes {1, 2, 3});
    ByteReader reader(writer.data());

    cr_assert_eq(reader.read<std::uint8_t>(), 200);
    cr_assert_eq(reader.read<std::uint16_t>(), 60000);
    cr_assert_eq(reader.read<std::int32_t>(), -123456);
    float value = reader.readFloat();

    cr_assert_float_eq(value, 3.5F, 0.0001F);
    cr_assert(reader.readString() == "R-Type");
    cr_assert(reader.readBytes() == (Bytes {1, 2, 3}));
    cr_assert(reader.ok());
    cr_assert(reader.atEnd());
}

Test(ByteReader, failsWithoutThrowingWhenOutOfBounds)
{
    Bytes bytes {1, 2, 3};
    ByteReader reader(bytes);

    cr_assert_eq(reader.read<std::uint32_t>(), 0u);
    cr_assert_not(reader.ok());
    cr_assert_eq(
        reader.read<std::uint8_t>(), 0, "the failed state must stay once set");
}

Test(ByteReader, failsOnTruncatedStringOrBytes)
{
    Bytes truncatedString {5, 'a', 'b'};
    Bytes truncatedBytes {4, 0, 1};
    ByteReader stringReader(truncatedString);
    ByteReader bytesReader(truncatedBytes);

    cr_assert(stringReader.readString().empty());
    cr_assert_not(stringReader.ok());
    cr_assert(bytesReader.readBytes().empty());
    cr_assert_not(bytesReader.ok());
}

Test(Protocol, comparesSequencesAcrossWrapAround)
{
    cr_assert(isSequenceMoreRecent(2, 1));
    cr_assert_not(isSequenceMoreRecent(1, 2));
    cr_assert_not(isSequenceMoreRecent(5, 5));
    cr_assert(isSequenceMoreRecent(0, 65535));
    cr_assert_not(isSequenceMoreRecent(65535, 0));
    cr_assert(isSequenceMoreRecent(7000, 40000));
    cr_assert_not(isSequenceMoreRecent(40000, 7000));
}

Test(Protocol, classifiesGameMessageTypes)
{
    cr_assert_not(isGameMessageType(MessageType::HeartbeatReply));
    cr_assert(isGameMessageType(static_cast<MessageType>(0x80)));
    cr_assert(isGameMessageType(static_cast<MessageType>(0xFF)));
}

Test(PacketHeader, roundTripsInFifteenBytes)
{
    PacketHeader header;
    ByteWriter writer;

    header.type = MessageType::EntitySpawn;
    header.flags = FLAG_RELIABLE;
    header.sequence = 513;
    header.ack = 512;
    header.ackBits = 0xDEADBEEF;
    header.payloadSize = 42;
    header.serialize(writer);
    ByteReader reader(writer.data());
    auto decoded = PacketHeader::deserialize(reader);

    cr_assert_eq(writer.data().size(), HEADER_SIZE);
    cr_assert_eq(writer.data()[0], 'R');
    cr_assert_eq(writer.data()[1], 'T');
    cr_assert_eq(decoded.magic, PROTOCOL_MAGIC);
    cr_assert_eq(decoded.version, PROTOCOL_VERSION);
    cr_assert(decoded.type == MessageType::EntitySpawn);
    cr_assert(decoded.isReliable());
    cr_assert_eq(decoded.sequence, 513);
    cr_assert_eq(decoded.ack, 512);
    cr_assert_eq(decoded.ackBits, 0xDEADBEEF);
    cr_assert_eq(decoded.payloadSize, 42);
}

Test(Packet, matchesTheSpecificationExample)
{
    auto packet = makePacket(HeartbeatPacket {.clientTime = 1000});

    packet.header.sequence = 7;
    packet.header.ack = 5;
    packet.header.ackBits = 0b11;

    // docs/protocol.md, appendix A
    cr_assert(packet.serialize() ==
        (Bytes {0x52,
            0x54,
            0x01,
            0x04,
            0x00,
            0x07,
            0x00,
            0x05,
            0x00,
            0x03,
            0x00,
            0x00,
            0x00,
            0x04,
            0x00,
            0xE8,
            0x03,
            0x00,
            0x00}));
}

Test(Packet, setsPayloadSizeAndReliableFlag)
{
    auto despawn = Packet::deserialize(
        makePacket(EntityDespawnPacket {.networkId = 1}).serialize());
    auto controllable = makePacket(ControllablePacket {});

    cr_assert(despawn.has_value());
    cr_assert_eq(despawn->header.payloadSize, 4);
    cr_assert(despawn->header.isReliable());
    cr_assert_not(controllable.header.isReliable());
}

Test(Packet, rejectsDatagramsWithInvalidFraming)
{
    auto valid = makePacket(HeartbeatPacket {.clientTime = 1}).serialize();
    auto badMagic = valid;
    auto trailingByte = valid;
    auto missingByte = valid;

    badMagic[0] = 'X';
    trailingByte.push_back(0);
    missingByte.pop_back();

    cr_assert(Packet::deserialize(valid).has_value());
    cr_assert_not(Packet::deserialize(Bytes(HEADER_SIZE - 1, 0)).has_value());
    cr_assert_not(
        Packet::deserialize(Bytes(MAX_PACKET_SIZE + 1, 0)).has_value());
    cr_assert_not(Packet::deserialize(badMagic).has_value());
    cr_assert_not(Packet::deserialize(trailingByte).has_value());
    cr_assert_not(Packet::deserialize(missingByte).has_value());
}

Test(Packet, keepsPacketsWithAnotherVersion)
{
    auto datagram = makePacket(AskConnectPacket {}).serialize();

    datagram[2] = PROTOCOL_VERSION + 1;
    auto packet = Packet::deserialize(datagram);

    cr_assert(packet.has_value(), "the receiver decides what to do with it");
    cr_assert_eq(packet->header.version, PROTOCOL_VERSION + 1);
}

Test(Packet, enforcesTheMaximumPayloadSize)
{
    Packet largest;
    Packet tooLarge;

    largest.payload.resize(MAX_PAYLOAD_SIZE);
    tooLarge.payload.resize(MAX_PAYLOAD_SIZE + 1);
    auto datagram = largest.serialize();

    cr_assert_eq(datagram.size(), MAX_PACKET_SIZE);
    cr_assert(Packet::deserialize(datagram).has_value());
    cr_assert_throw((void) tooLarge.serialize(), std::length_error);
}

Test(ClientPackets, roundTripThroughADatagram)
{
    AskConnectPacket askConnect {.playerName = "player one"};
    ControllablePacket controllable {
        .inputSequence = 42, .up = true, .right = true, .actions = 0b101};
    HeartbeatPacket heartbeat {.clientTime = 123456};

    cr_assert(roundTrip(askConnect) == askConnect);
    cr_assert(roundTrip(AskDisconnectPacket {}) == AskDisconnectPacket {});
    cr_assert(roundTrip(controllable) == controllable);
    cr_assert(roundTrip(heartbeat) == heartbeat);
}

Test(ServerPackets, roundTripThroughADatagram)
{
    AcceptConnectPacket accept {
        .clientId = 3, .linkedEntity = 17, .tickRate = 60};
    ForceDisconnectPacket force {
        .reason = DisconnectReason::ServerFull, .message = "server is full"};
    PlayerConnectedPacket connected {
        .clientId = 2, .linkedEntity = 8, .playerName = "player two"};
    PlayerDisconnectedPacket disconnected {
        .clientId = 2, .reason = DisconnectReason::Timeout};
    EntitySpawnPacket spawn {.networkId = 9,
        .components = {{.typeId = 1, .data = {1, 2, 3}},
            {.typeId = GAME_COMPONENT_TYPE_MIN, .data = {}}}};
    EntityDespawnPacket despawn {.networkId = 9};
    ComponentUpdatePacket update {.tick = 100,
        .entities = {
            {.networkId = 9, .components = {{.typeId = 1, .data = {4, 5}}}},
            {.networkId = 10, .components = {}}}};
    HeartbeatReplyPacket reply {.clientTime = 123456, .serverTick = 99};

    cr_assert(roundTrip(accept) == accept);
    cr_assert(roundTrip(force) == force);
    cr_assert(roundTrip(connected) == connected);
    cr_assert(roundTrip(disconnected) == disconnected);
    cr_assert(roundTrip(spawn) == spawn);
    cr_assert(roundTrip(despawn) == despawn);
    cr_assert(roundTrip(update) == update);
    cr_assert(roundTrip(reply) == reply);
}

Test(Payloads, encodesControllableDirectionsAsBits)
{
    auto packet = makePacket(ControllablePacket {.up = true, .left = true});

    cr_assert_eq(packet.payload.size(), 7u);
    cr_assert_eq(packet.payload[4], 0b0101);
}

Test(Payloads, encodesComponentUpdateAsInTheSpecification)
{
    ComponentUpdatePacket update {.tick = 1,
        .entities = {
            {.networkId = 2, .components = {{.typeId = 3, .data = {4}}}}}};

    // tick, entity_count, network_id, component_count, type_id, data
    cr_assert(makePacket(update).payload ==
        (Bytes {1, 0, 0, 0, 1, 2, 0, 0, 0, 1, 3, 0, 1, 0, 4}));
}

Test(Payloads, fitsSixtyEntityPositionsInOneDatagram)
{
    ComponentUpdatePacket update;

    for (NetworkId id = 1; id <= 60; ++id) {
        update.entities.push_back({.networkId = id,
            .components = {{.typeId = 1, .data = Bytes(8, 0)}}});
    }
    auto datagram = makePacket(update).serialize();

    cr_assert_leq(datagram.size(), MAX_PACKET_SIZE);
    cr_assert(roundTrip(update) == update);
}

Test(Payloads, rejectsReservedDirectionBits)
{
    Bytes allDirections {0, 0, 0, 0, 0x0F, 0, 0};
    Bytes reservedBit {0, 0, 0, 0, 0x10, 0, 0};

    cr_assert(decodeRaw<ControllablePacket>(allDirections).has_value());
    cr_assert_not(decodeRaw<ControllablePacket>(reservedBit).has_value());
}

Test(Payloads, rejectsUnknownDisconnectReason)
{
    cr_assert(decodeRaw<PlayerDisconnectedPacket>({1, 0, 5}).has_value());
    cr_assert_not(decodeRaw<PlayerDisconnectedPacket>({1, 0, 6}).has_value());
    cr_assert_not(decodeRaw<ForceDisconnectPacket>({0xFF, 0}).has_value());
}

Test(Payloads, rejectsTruncatedOrTrailingBytes)
{
    cr_assert(decodeRaw<HeartbeatPacket>({0xE8, 3, 0, 0}).has_value());
    cr_assert_not(decodeRaw<HeartbeatPacket>({0xE8, 3, 0}).has_value());
    cr_assert_not(decodeRaw<HeartbeatPacket>({0xE8, 3, 0, 0, 0}).has_value());
}

Test(Payloads, rejectsAnotherMessageType)
{
    auto heartbeat = makePacket(HeartbeatPacket {.clientTime = 1});

    // Same payload size: only the header type tells them apart.
    cr_assert_not(readPayload<EntityDespawnPacket>(heartbeat).has_value());
}

Test(Payloads, rejectsComponentCountLargerThanTheData)
{
    // network_id = 1, 3 components announced, only one present.
    Bytes payload {1, 0, 0, 0, 3, 1, 0, 1, 0, 42};

    cr_assert_not(decodeRaw<EntitySpawnPacket>(payload).has_value());
    payload[4] = 1;
    cr_assert(decodeRaw<EntitySpawnPacket>(payload).has_value());
}

Test(Payloads, refusesToSendMoreThan255Components)
{
    EntitySpawnPacket spawn;

    spawn.components.resize(256);
    cr_assert_throw((void) makePacket(spawn), std::length_error);
}

Test(Payloads, refusesToSendMoreThan255Entities)
{
    ComponentUpdatePacket update;

    update.entities.resize(256);
    cr_assert_throw((void) makePacket(update), std::length_error);
}

Test(Robustness, randomDatagramsNeverCrashTheDecoder)
{
    std::mt19937 random(42);
    std::uniform_int_distribution<std::size_t> size(0, MAX_PACKET_SIZE + 10);
    std::uniform_int_distribution<int> byte(0, 255);

    for (int i = 0; i < 20000; ++i) {
        Bytes datagram(size(random));

        for (auto &value : datagram) {
            value = static_cast<std::uint8_t>(byte(random));
        }
        // Half of the time, make the framing valid to reach the payloads.
        if (i % 2 == 0 && datagram.size() >= HEADER_SIZE) {
            auto payloadSize = datagram.size() - HEADER_SIZE;

            datagram[0] = 0x52;
            datagram[1] = 0x54;
            datagram[13] = static_cast<std::uint8_t>(payloadSize);
            datagram[14] = static_cast<std::uint8_t>(payloadSize >> 8);
        }
        auto packet = Packet::deserialize(datagram);

        if (packet) {
            decodeAsEveryPayload(*packet);
        }
    }
}

namespace {
    EntityComponents positionUpdate(NetworkId id)
    {
        return {.networkId = id,
            .components = {{.typeId = 1, .data = Bytes(8, 0)}}};
    }

    /**
     * @brief Puts the split entities back together: an entity continued in
     *        the next packet is merged with its first part.
     */
    std::vector<EntityComponents> joinUpdates(
        const std::vector<ComponentUpdatePacket> &packets)
    {
        std::vector<EntityComponents> entities;

        for (std::size_t i = 0; i < packets.size(); ++i) {
            for (std::size_t j = 0; j < packets[i].entities.size(); ++j) {
                const auto &entity = packets[i].entities[j];

                if (i > 0 && j == 0 &&
                    entities.back().networkId == entity.networkId) {
                    auto &components = entities.back().components;

                    components.insert(components.end(),
                        entity.components.begin(), entity.components.end());
                } else {
                    entities.push_back(entity);
                }
            }
        }
        return entities;
    }

    void assertEveryPacketFits(
        const std::vector<ComponentUpdatePacket> &packets, Tick tick)
    {
        for (const auto &packet : packets) {
            cr_assert_eq(packet.tick, tick);
            cr_assert_not(packet.entities.empty());
            cr_assert_leq(makePacket(packet).serialize().size(),
                MAX_PACKET_SIZE);
            for (const auto &entity : packet.entities) {
                cr_assert_not(entity.components.empty());
            }
        }
    }
} // namespace

Test(SplitComponentUpdate, nothingToSendGivesNoPacket)
{
    cr_assert(splitComponentUpdate(1, {}).empty());
}

Test(SplitComponentUpdate, smallUpdateStaysInOnePacket)
{
    std::vector<EntityComponents> entities = {
        positionUpdate(1), positionUpdate(2), positionUpdate(3)};
    auto packets = splitComponentUpdate(42, entities);

    cr_assert_eq(packets.size(), 1u);
    cr_assert(packets[0] ==
        (ComponentUpdatePacket {.tick = 42, .entities = entities}));
}

Test(SplitComponentUpdate, splitsManyEntitiesKeepingTheirOrder)
{
    std::vector<EntityComponents> entities;

    for (NetworkId id = 1; id <= 200; ++id) {
        entities.push_back(positionUpdate(id));
    }
    auto packets = splitComponentUpdate(7, entities);

    // 17 bytes per entity: 69 fit in the 1180 bytes left after tick and
    // entity_count.
    cr_assert_eq(packets.size(), 3u);
    cr_assert_eq(packets[0].entities.size(), 69u);
    assertEveryPacketFits(packets, 7);
    cr_assert(joinUpdates(packets) == entities);
}

Test(SplitComponentUpdate, continuesALargeEntityInTheNextPacket)
{
    EntityComponents boss {.networkId = 9, .components = {}};

    for (ComponentTypeId type = 0; type < 300; ++type) {
        boss.components.push_back(
            {.typeId = type, .data = Bytes(8, static_cast<std::uint8_t>(type))});
    }
    std::vector<EntityComponents> entities = {
        positionUpdate(1), boss, positionUpdate(2)};
    auto packets = splitComponentUpdate(3, entities);

    cr_assert_gt(packets.size(), 1u);
    assertEveryPacketFits(packets, 3);
    cr_assert(joinUpdates(packets) == entities);
}

Test(SplitComponentUpdate, movesAnEntityThatDoesNotStartToFit)
{
    std::vector<EntityComponents> entities;

    // 69 positions leave 7 bytes: room for an entity header, not for a
    // 1-byte component (5 bytes).
    for (NetworkId id = 1; id <= 69; ++id) {
        entities.push_back(positionUpdate(id));
    }
    entities.push_back({.networkId = 70,
        .components = {{.typeId = 1, .data = Bytes(1, 0)}}});
    auto packets = splitComponentUpdate(0, entities);

    cr_assert_eq(packets.size(), 2u);
    cr_assert_eq(packets[0].entities.size(), 69u);
    cr_assert_eq(packets[1].entities.size(), 1u);
    assertEveryPacketFits(packets, 0);
    cr_assert(joinUpdates(packets) == entities);
}

Test(SplitComponentUpdate, throwsOnAComponentLargerThanAPacket)
{
    std::vector<EntityComponents> entities = {{.networkId = 1,
        .components = {{.typeId = 1, .data = Bytes(MAX_PAYLOAD_SIZE, 0)}}}};

    cr_assert_throw(splitComponentUpdate(0, entities), std::length_error);
}

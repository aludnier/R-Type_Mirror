#include <catch2/catch_test_macros.hpp>

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

TEST_CASE("ByteWriter writes integers in little endian",
    "[network][byte-writer]")
{
    ByteWriter writer;

    writer.write(std::uint8_t {0x01});
    writer.write(std::uint16_t {0x0203});
    writer.write(std::uint32_t {0x04050607});
    writer.write(std::int16_t {-2});
    writer.write(MessageType::Heartbeat);

    REQUIRE(writer.data() ==
        (Bytes {0x01, 0x03, 0x02, 0x07, 0x06, 0x05, 0x04, 0xFE, 0xFF, 0x04}));
}

TEST_CASE("ByteWriter writes length prefixed strings and bytes",
    "[network][byte-writer]")
{
    ByteWriter writer;

    writer.writeString("abc");
    writer.writeBytes(Bytes {9, 8});

    REQUIRE(writer.data() == (Bytes {3, 'a', 'b', 'c', 2, 0, 9, 8}));
}

TEST_CASE("ByteWriter rejects strings longer than 255 bytes",
    "[network][byte-writer]")
{
    ByteWriter writer;

    writer.writeString(std::string(255, 'x'));
    REQUIRE(writer.data().size() == 256u);
    REQUIRE_THROWS_AS(
        writer.writeString(std::string(256, 'x')), std::length_error);
}

TEST_CASE("ByteWriter release leaves the writer empty",
    "[network][byte-writer]")
{
    ByteWriter writer;

    writer.write(std::uint32_t {1});
    auto bytes = writer.release();

    REQUIRE(bytes.size() == 4u);
    REQUIRE(writer.data().empty());
}

TEST_CASE("ByteReader reads back what was written", "[network][byte-reader]")
{
    ByteWriter writer;

    writer.write(std::uint8_t {200});
    writer.write(std::uint16_t {60000});
    writer.write(std::int32_t {-123456});
    writer.writeFloat(3.5F);
    writer.writeString("R-Type");
    writer.writeBytes(Bytes {1, 2, 3});
    ByteReader reader(writer.data());

    REQUIRE(reader.read<std::uint8_t>() == 200);
    REQUIRE(reader.read<std::uint16_t>() == 60000);
    REQUIRE(reader.read<std::int32_t>() == -123456);
    float value = reader.readFloat();

    REQUIRE(value == 3.5F);
    REQUIRE(reader.readString() == "R-Type");
    REQUIRE(reader.readBytes() == (Bytes {1, 2, 3}));
    REQUIRE(reader.ok());
    REQUIRE(reader.atEnd());
}

TEST_CASE("ByteReader fails without throwing when out of bounds",
    "[network][byte-reader]")
{
    Bytes bytes {1, 2, 3};
    ByteReader reader(bytes);

    REQUIRE(reader.read<std::uint32_t>() == 0u);
    REQUIRE_FALSE(reader.ok());
    INFO("the failed state must stay once set");
    REQUIRE(reader.read<std::uint8_t>() == 0);
}

TEST_CASE("ByteReader fails on truncated string or bytes",
    "[network][byte-reader]")
{
    Bytes truncatedString {5, 'a', 'b'};
    Bytes truncatedBytes {4, 0, 1};
    ByteReader stringReader(truncatedString);
    ByteReader bytesReader(truncatedBytes);

    REQUIRE(stringReader.readString().empty());
    REQUIRE_FALSE(stringReader.ok());
    REQUIRE(bytesReader.readBytes().empty());
    REQUIRE_FALSE(bytesReader.ok());
}

TEST_CASE("Protocol compares sequences across wrap around",
    "[network][protocol]")
{
    REQUIRE(isSequenceMoreRecent(2, 1));
    REQUIRE_FALSE(isSequenceMoreRecent(1, 2));
    REQUIRE_FALSE(isSequenceMoreRecent(5, 5));
    REQUIRE(isSequenceMoreRecent(0, 65535));
    REQUIRE_FALSE(isSequenceMoreRecent(65535, 0));
    REQUIRE(isSequenceMoreRecent(7000, 40000));
    REQUIRE_FALSE(isSequenceMoreRecent(40000, 7000));
}

TEST_CASE("Protocol classifies game message types", "[network][protocol]")
{
    REQUIRE_FALSE(isGameMessageType(MessageType::HeartbeatReply));
    REQUIRE(isGameMessageType(static_cast<MessageType>(0x80)));
    REQUIRE(isGameMessageType(static_cast<MessageType>(0xFF)));
}

TEST_CASE("PacketHeader round trips in fifteen bytes",
    "[network][packet-header]")
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

    REQUIRE(writer.data().size() == HEADER_SIZE);
    REQUIRE(writer.data()[0] == 'R');
    REQUIRE(writer.data()[1] == 'T');
    REQUIRE(decoded.magic == PROTOCOL_MAGIC);
    REQUIRE(decoded.version == PROTOCOL_VERSION);
    REQUIRE(decoded.type == MessageType::EntitySpawn);
    REQUIRE(decoded.isReliable());
    REQUIRE(decoded.sequence == 513);
    REQUIRE(decoded.ack == 512);
    REQUIRE(decoded.ackBits == 0xDEADBEEF);
    REQUIRE(decoded.payloadSize == 42);
}

TEST_CASE("Packet matches the specification example", "[network][packet]")
{
    auto packet = makePacket(HeartbeatPacket {.clientTime = 1000});

    packet.header.sequence = 7;
    packet.header.ack = 5;
    packet.header.ackBits = 0b11;

    // docs/protocol.md, appendix A
    REQUIRE(packet.serialize() ==
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

TEST_CASE("Packet sets payload size and reliable flag", "[network][packet]")
{
    auto despawn = Packet::deserialize(
        makePacket(EntityDespawnPacket {.networkId = 1}).serialize());
    auto controllable = makePacket(ControllablePacket {});

    REQUIRE(despawn.has_value());
    REQUIRE(despawn->header.payloadSize == 4);
    REQUIRE(despawn->header.isReliable());
    REQUIRE_FALSE(controllable.header.isReliable());
}

TEST_CASE("Packet rejects datagrams with invalid framing", "[network][packet]")
{
    auto valid = makePacket(HeartbeatPacket {.clientTime = 1}).serialize();
    auto badMagic = valid;
    auto trailingByte = valid;
    auto missingByte = valid;

    badMagic[0] = 'X';
    trailingByte.push_back(0);
    missingByte.pop_back();

    REQUIRE(Packet::deserialize(valid).has_value());
    REQUIRE_FALSE(Packet::deserialize(Bytes(HEADER_SIZE - 1, 0)).has_value());
    REQUIRE_FALSE(
        Packet::deserialize(Bytes(MAX_PACKET_SIZE + 1, 0)).has_value());
    REQUIRE_FALSE(Packet::deserialize(badMagic).has_value());
    REQUIRE_FALSE(Packet::deserialize(trailingByte).has_value());
    REQUIRE_FALSE(Packet::deserialize(missingByte).has_value());
}

TEST_CASE("Packet keeps packets with another version", "[network][packet]")
{
    auto datagram = makePacket(AskConnectPacket {}).serialize();

    datagram[2] = PROTOCOL_VERSION + 1;
    auto packet = Packet::deserialize(datagram);

    INFO("the receiver decides what to do with it");
    REQUIRE(packet.has_value());
    REQUIRE(packet->header.version == PROTOCOL_VERSION + 1);
}

TEST_CASE("Packet enforces the maximum payload size", "[network][packet]")
{
    Packet largest;
    Packet tooLarge;

    largest.payload.resize(MAX_PAYLOAD_SIZE);
    tooLarge.payload.resize(MAX_PAYLOAD_SIZE + 1);
    auto datagram = largest.serialize();

    REQUIRE(datagram.size() == MAX_PACKET_SIZE);
    REQUIRE(Packet::deserialize(datagram).has_value());
    REQUIRE_THROWS_AS((void) tooLarge.serialize(), std::length_error);
}

TEST_CASE("ClientPackets round trip through a datagram",
    "[network][client-packets]")
{
    AskConnectPacket askConnect {.playerName = "player one"};
    ControllablePacket controllable {
        .inputSequence = 42, .actions = 0x80000005};
    HeartbeatPacket heartbeat {.clientTime = 123456};

    REQUIRE(roundTrip(askConnect) == askConnect);
    REQUIRE(roundTrip(AskDisconnectPacket {}) == AskDisconnectPacket {});
    REQUIRE(roundTrip(controllable) == controllable);
    REQUIRE(roundTrip(heartbeat) == heartbeat);
}

TEST_CASE("ServerPackets round trip through a datagram",
    "[network][server-packets]")
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

    REQUIRE(roundTrip(accept) == accept);
    REQUIRE(roundTrip(force) == force);
    REQUIRE(roundTrip(connected) == connected);
    REQUIRE(roundTrip(disconnected) == disconnected);
    REQUIRE(roundTrip(spawn) == spawn);
    REQUIRE(roundTrip(despawn) == despawn);
    REQUIRE(roundTrip(update) == update);
    REQUIRE(roundTrip(reply) == reply);
}

TEST_CASE("Payloads encodes controllable actions as a u32 bitmask",
    "[network][payloads]")
{
    auto packet = makePacket(
        ControllablePacket {.inputSequence = 1, .actions = 0x80000005});

    // input_sequence, then actions in little-endian
    REQUIRE(packet.payload == (Bytes {1, 0, 0, 0, 0x05, 0, 0, 0x80}));
}

TEST_CASE("Payloads encodes component update as in the specification",
    "[network][payloads]")
{
    ComponentUpdatePacket update {.tick = 1,
        .entities = {
            {.networkId = 2, .components = {{.typeId = 3, .data = {4}}}}}};

    // tick, entity_count, network_id, component_count, type_id, data
    REQUIRE(makePacket(update).payload ==
        (Bytes {1, 0, 0, 0, 1, 2, 0, 0, 0, 1, 3, 0, 1, 0, 4}));
}

TEST_CASE("Payloads fits sixty entity positions in one datagram",
    "[network][payloads]")
{
    ComponentUpdatePacket update;

    for (NetworkId id = 1; id <= 60; ++id) {
        update.entities.push_back({.networkId = id,
            .components = {{.typeId = 1, .data = Bytes(8, 0)}}});
    }
    auto datagram = makePacket(update).serialize();

    REQUIRE(datagram.size() <= MAX_PACKET_SIZE);
    REQUIRE(roundTrip(update) == update);
}

TEST_CASE("Payloads accepts every controllable action bit",
    "[network][payloads]")
{
    auto controls = decodeRaw<ControllablePacket>(
        {0, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF});

    REQUIRE(controls.has_value());
    REQUIRE(controls->actions == 0xFFFFFFFFu);
}

TEST_CASE("Payloads rejects unknown disconnect reason", "[network][payloads]")
{
    REQUIRE(decodeRaw<PlayerDisconnectedPacket>({1, 0, 5}).has_value());
    REQUIRE_FALSE(decodeRaw<PlayerDisconnectedPacket>({1, 0, 6}).has_value());
    REQUIRE_FALSE(decodeRaw<ForceDisconnectPacket>({0xFF, 0}).has_value());
}

TEST_CASE("Payloads rejects truncated or trailing bytes", "[network][payloads]")
{
    REQUIRE(decodeRaw<HeartbeatPacket>({0xE8, 3, 0, 0}).has_value());
    REQUIRE_FALSE(decodeRaw<HeartbeatPacket>({0xE8, 3, 0}).has_value());
    REQUIRE_FALSE(decodeRaw<HeartbeatPacket>({0xE8, 3, 0, 0, 0}).has_value());
}

TEST_CASE("Payloads rejects another message type", "[network][payloads]")
{
    auto heartbeat = makePacket(HeartbeatPacket {.clientTime = 1});

    // Same payload size: only the header type tells them apart.
    REQUIRE_FALSE(readPayload<EntityDespawnPacket>(heartbeat).has_value());
}

TEST_CASE("Payloads rejects component count larger than the data",
    "[network][payloads]")
{
    // network_id = 1, 3 components announced, only one present.
    Bytes payload {1, 0, 0, 0, 3, 1, 0, 1, 0, 42};

    REQUIRE_FALSE(decodeRaw<EntitySpawnPacket>(payload).has_value());
    payload[4] = 1;
    REQUIRE(decodeRaw<EntitySpawnPacket>(payload).has_value());
}

TEST_CASE("Payloads refuses to send more than 255 components",
    "[network][payloads]")
{
    EntitySpawnPacket spawn;

    spawn.components.resize(256);
    REQUIRE_THROWS_AS((void) makePacket(spawn), std::length_error);
}

TEST_CASE("Payloads refuses to send more than 255 entities",
    "[network][payloads]")
{
    ComponentUpdatePacket update;

    update.entities.resize(256);
    REQUIRE_THROWS_AS((void) makePacket(update), std::length_error);
}

TEST_CASE("Robustness random datagrams never crash the decoder",
    "[network][robustness]")
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
            REQUIRE(packet.tick == tick);
            REQUIRE_FALSE(packet.entities.empty());
            REQUIRE(makePacket(packet).serialize().size() <= MAX_PACKET_SIZE);
            for (const auto &entity : packet.entities) {
                REQUIRE_FALSE(entity.components.empty());
            }
        }
    }
} // namespace

TEST_CASE("splitComponentUpdate gives no packet when nothing changed",
    "[network][component-update]")
{
    REQUIRE(splitComponentUpdate(1, {}).empty());
}

TEST_CASE("splitComponentUpdate keeps a small update in one packet",
    "[network][component-update]")
{
    std::vector<EntityComponents> entities = {
        positionUpdate(1), positionUpdate(2), positionUpdate(3)};
    auto packets = splitComponentUpdate(42, entities);

    REQUIRE(packets.size() == 1u);
    REQUIRE(packets[0] ==
        (ComponentUpdatePacket {.tick = 42, .entities = entities}));
}

TEST_CASE("splitComponentUpdate splits many entities keeping their order",
    "[network][component-update]")
{
    std::vector<EntityComponents> entities;

    for (NetworkId id = 1; id <= 200; ++id) {
        entities.push_back(positionUpdate(id));
    }
    auto packets = splitComponentUpdate(7, entities);

    // 17 bytes per entity: 69 fit in the 1180 bytes left after tick and
    // entity_count.
    REQUIRE(packets.size() == 3u);
    REQUIRE(packets[0].entities.size() == 69u);
    assertEveryPacketFits(packets, 7);
    REQUIRE(joinUpdates(packets) == entities);
}

TEST_CASE("splitComponentUpdate continues a large entity in the next packet",
    "[network][component-update]")
{
    EntityComponents boss {.networkId = 9, .components = {}};

    for (ComponentTypeId type = 0; type < 300; ++type) {
        boss.components.push_back(
            {.typeId = type, .data = Bytes(8, static_cast<std::uint8_t>(type))});
    }
    std::vector<EntityComponents> entities = {
        positionUpdate(1), boss, positionUpdate(2)};
    auto packets = splitComponentUpdate(3, entities);

    REQUIRE(packets.size() > 1u);
    assertEveryPacketFits(packets, 3);
    REQUIRE(joinUpdates(packets) == entities);
}

TEST_CASE("splitComponentUpdate moves an entity that does not start to fit",
    "[network][component-update]")
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

    REQUIRE(packets.size() == 2u);
    REQUIRE(packets[0].entities.size() == 69u);
    REQUIRE(packets[1].entities.size() == 1u);
    assertEveryPacketFits(packets, 0);
    REQUIRE(joinUpdates(packets) == entities);
}

TEST_CASE("splitComponentUpdate throws on a component larger than a packet",
    "[network][component-update]")
{
    std::vector<EntityComponents> entities = {{.networkId = 1,
        .components = {{.typeId = 1, .data = Bytes(MAX_PAYLOAD_SIZE, 0)}}}};

    REQUIRE_THROWS_AS(splitComponentUpdate(0, entities), std::length_error);
}

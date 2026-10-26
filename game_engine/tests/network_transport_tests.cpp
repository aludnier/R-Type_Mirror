#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <thread>
#include <utility>
#include <vector>

#include "network/ClientPackets.h"
#include "network/ServerPackets.h"
#include "network/Endpoint.h"
#include "network/NetworkThread.h"
#include "network/Packet.h"
#include "network/Protocol.h"
#include "network/ThreadSafeQueue.h"
#include "network/UdpSocket.h"

using namespace RType::Network;

namespace {
    using Bytes = std::vector<std::uint8_t>;

    constexpr auto TIMEOUT = std::chrono::seconds(2);

    /** @return true once @p done holds, false after TIMEOUT. */
    bool waitFor(const std::function<bool()> &done)
    {
        auto deadline = std::chrono::steady_clock::now() + TIMEOUT;

        while (!done()) {
            if (std::chrono::steady_clock::now() > deadline) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return true;
    }

    Endpoint loopback(std::uint16_t port)
    {
        return {"127.0.0.1", port};
    }

    Bytes heartbeatDatagram(std::uint32_t clientTime)
    {
        return makePacket(HeartbeatPacket {clientTime}).serialize();
    }

    /** @brief A UdpSocket with its own network thread, for sending. */
    class Sender {
    public:
        Sender() :
            _thread([this] { _socket.run([](auto &, auto) {}); })
        {
        }

        ~Sender()
        {
            _socket.close();
            _thread.join();
        }

        void send(std::uint16_t port, Bytes datagram) const
        {
            _socket.sendTo(loopback(port), std::move(datagram));
        }

        [[nodiscard]] std::uint16_t port() const
        {
            return _socket.localPort();
        }

    private:
        UdpSocket _socket {0};
        std::thread _thread;
    };
} // namespace

TEST_CASE("ThreadSafeQueue pops in push order",
    "[network][thread-safe-queue]")
{
    ThreadSafeQueue<int> queue(8);

    REQUIRE(queue.push(1));
    REQUIRE(queue.push(2));
    REQUIRE(queue.push(3));
    REQUIRE(queue.tryPop() == 1);
    REQUIRE(queue.tryPop() == 2);
    REQUIRE(queue.tryPop() == 3);
}

TEST_CASE("ThreadSafeQueue try pop on empty queue returns nothing",
    "[network][thread-safe-queue]")
{
    ThreadSafeQueue<int> queue(8);

    REQUIRE(queue.tryPop() == std::nullopt);
    queue.push(1);
    queue.tryPop();
    REQUIRE(queue.tryPop() == std::nullopt);
}

TEST_CASE("ThreadSafeQueue drops new values when full",
    "[network][thread-safe-queue]")
{
    ThreadSafeQueue<int> queue(2);

    REQUIRE(queue.push(1));
    REQUIRE(queue.push(2));
    REQUIRE_FALSE(queue.push(3));
    REQUIRE(queue.tryPop() == 1);
    REQUIRE(queue.push(4));
    REQUIRE(queue.tryPop() == 2);
    REQUIRE(queue.tryPop() == 4);
    REQUIRE(queue.tryPop() == std::nullopt);
}

TEST_CASE("ThreadSafeQueue keeps move only values",
    "[network][thread-safe-queue]")
{
    ThreadSafeQueue<std::unique_ptr<int>> queue(1);

    queue.push(std::make_unique<int>(42));
    auto value = queue.tryPop();

    REQUIRE(value.has_value());
    REQUIRE(**value == 42);
}

TEST_CASE("ThreadSafeQueue passes every value from one thread to another",
    "[network][thread-safe-queue]")
{
    constexpr int count = 10000;
    ThreadSafeQueue<int> queue(64);
    std::thread producer([&queue] {
        for (int i = 0; i < count; ++i) {
            while (!queue.push(i)) {
                std::this_thread::yield();
            }
        }
    });
    int expected = 0;

    while (expected < count) {
        if (auto value = queue.tryPop()) {
            REQUIRE(*value == expected);
            ++expected;
        }
    }
    producer.join();
    REQUIRE(queue.tryPop() == std::nullopt);
}

TEST_CASE("UdpSocket sends and receives on loopback",
    "[network][udp-socket]")
{
    UdpSocket receiver(0);
    ThreadSafeQueue<std::pair<Endpoint, Bytes>> received(8);
    std::thread thread([&] {
        receiver.run(
            [&](const Endpoint &from, std::span<const std::uint8_t> bytes) {
                received.push({from, Bytes(bytes.begin(), bytes.end())});
            });
    });
    Sender sender;
    std::optional<std::pair<Endpoint, Bytes>> datagram;

    sender.send(receiver.localPort(), {1, 2, 3});
    REQUIRE(
        waitFor([&] { return (datagram = received.tryPop()).has_value(); }));
    REQUIRE(datagram->first.address == "127.0.0.1");
    REQUIRE(datagram->first.port == sender.port());
    REQUIRE(datagram->second == Bytes({1, 2, 3}));
    receiver.close();
    thread.join();
}

TEST_CASE("UdpSocket oversized datagram is seen too long",
    "[network][udp-socket]")
{
    UdpSocket receiver(0);
    ThreadSafeQueue<std::size_t> sizes(8);
    std::thread thread([&] {
        receiver.run(
            [&](const Endpoint &, std::span<const std::uint8_t> bytes) {
                sizes.push(bytes.size());
            });
    });
    Sender sender;
    std::optional<std::size_t> size;

    sender.send(receiver.localPort(), Bytes(MAX_PACKET_SIZE + 100, 0));
    REQUIRE(waitFor([&] { return (size = sizes.tryPop()).has_value(); }));
    REQUIRE(*size > MAX_PACKET_SIZE);
    receiver.close();
    thread.join();
}

TEST_CASE("UdpSocket close stops run without traffic",
    "[network][udp-socket]")
{
    UdpSocket socket(0);
    std::thread thread([&] { socket.run([](auto &, auto) {}); });

    socket.close();
    thread.join();
}

TEST_CASE("NetworkThread delivers valid packets",
    "[network][network-thread]")
{
    NetworkThread network(0, 8);
    Sender sender;
    std::optional<NetworkThread::ReceivedPacket> received;

    network.start();
    sender.send(network.localPort(), heartbeatDatagram(1000));
    REQUIRE(waitFor([&] { return (received = network.poll()).has_value(); }));
    REQUIRE(received->from.port == sender.port());
    REQUIRE(received->packet.header.type == MessageType::Heartbeat);

    auto heartbeat = readPayload<HeartbeatPacket>(received->packet);

    REQUIRE(heartbeat.has_value());
    REQUIRE(heartbeat->clientTime == 1000u);
    REQUIRE(network.takeDroppedCount() == 0u);
}

TEST_CASE("NetworkThread drops malformed and oversized datagrams",
    "[network][network-thread]")
{
    NetworkThread network(0, 8);
    Sender sender;
    Bytes badMagic = heartbeatDatagram(1);
    Bytes truncated = heartbeatDatagram(1);
    Bytes oversized(MAX_PACKET_SIZE + 1, 0);
    std::optional<NetworkThread::ReceivedPacket> received;

    badMagic[0] = 0;
    truncated.pop_back();
    network.start();
    sender.send(network.localPort(), Bytes {1, 2, 3});
    sender.send(network.localPort(), badMagic);
    sender.send(network.localPort(), truncated);
    sender.send(network.localPort(), oversized);
    // Sent last: once it is received, the bad ones were handled.
    sender.send(network.localPort(), heartbeatDatagram(2));
    REQUIRE(waitFor([&] { return (received = network.poll()).has_value(); }));
    REQUIRE(readPayload<HeartbeatPacket>(received->packet)->clientTime == 2u);
    REQUIRE(network.poll().has_value() == false);
    REQUIRE(network.takeDroppedCount() == 4u);
    REQUIRE(network.takeDroppedCount() == 0u);
}

TEST_CASE("NetworkThread drops packets when queue is full",
    "[network][network-thread]")
{
    NetworkThread network(0, 1);
    std::size_t dropped = 0;

    network.start();
    for (std::uint32_t i = 0; i < 3; ++i) {
        Sender sender;
        sender.send(network.localPort(), heartbeatDatagram(i));
    }
    REQUIRE(
        waitFor([&] { return (dropped += network.takeDroppedCount()) >= 2; }));
    REQUIRE(dropped == 2u);

    auto first = network.poll();

    REQUIRE(first.has_value());
    REQUIRE(readPayload<HeartbeatPacket>(first->packet)->clientTime == 0u);
    REQUIRE(network.poll().has_value() == false);
}

TEST_CASE("NetworkThread counts datagrams dropped by the caller",
    "[network][network-thread]")
{
    NetworkThread network(0, 1);

    network.countDropped();
    network.countDropped();
    REQUIRE(network.takeDroppedCount() == 2u);
}

TEST_CASE("NetworkThread stop is safe without start and twice",
    "[network][network-thread]")
{
    NetworkThread notStarted(0, 1);

    notStarted.stop();

    NetworkThread started(0, 1);

    started.start();
    started.stop();
    started.stop();
}

TEST_CASE("NetworkThread sends before stop are delivered",
    "[network][network-thread]")
{
    NetworkThread receiver(0, 8);
    std::optional<NetworkThread::ReceivedPacket> received;

    receiver.start();
    {
        NetworkThread sender(0, 1);

        sender.start();
        sender.sendTo(loopback(receiver.localPort()), heartbeatDatagram(7));
        sender.stop();
    }
    REQUIRE(waitFor([&] { return (received = receiver.poll()).has_value(); }));
    REQUIRE(readPayload<HeartbeatPacket>(received->packet)->clientTime == 7u);
}

TEST_CASE("Client and Server Packets round-trip across NetworkThread",
    "[network][transport][packets]")
{
    NetworkThread server(0, 8);
    NetworkThread client(0, 8);
    server.start();
    client.start();

    // 1. Client sends AskConnectPacket to server
    auto askPacket = makePacket(AskConnectPacket{"TestPlayer"});
    client.sendTo(loopback(server.localPort()), askPacket.serialize());

    std::optional<NetworkThread::ReceivedPacket> receivedOnServer;
    REQUIRE(waitFor([&] { return (receivedOnServer = server.poll()).has_value(); }));
    REQUIRE(receivedOnServer->packet.header.type == MessageType::AskConnect);
    auto askDecoded = readPayload<AskConnectPacket>(receivedOnServer->packet);
    REQUIRE(askDecoded.has_value());
    REQUIRE(askDecoded->playerName == "TestPlayer");

    // 2. Server replies with AcceptConnectPacket
    auto acceptPacket = makePacket(AcceptConnectPacket{1, 42, 60});
    server.sendTo(receivedOnServer->from, acceptPacket.serialize());

    std::optional<NetworkThread::ReceivedPacket> receivedOnClient;
    REQUIRE(waitFor([&] { return (receivedOnClient = client.poll()).has_value(); }));
    REQUIRE(receivedOnClient->packet.header.type == MessageType::AcceptConnect);
    auto acceptDecoded = readPayload<AcceptConnectPacket>(receivedOnClient->packet);
    REQUIRE(acceptDecoded.has_value());
    REQUIRE(acceptDecoded->clientId == 1);
    REQUIRE(acceptDecoded->linkedEntity == 42);
    REQUIRE(acceptDecoded->tickRate == 60);

    // 3. Client sends ControllablePacket
    auto ctrlPacket = makePacket(ControllablePacket{10, 0x05}); // Up + Shoot
    client.sendTo(loopback(server.localPort()), ctrlPacket.serialize());

    REQUIRE(waitFor([&] { return (receivedOnServer = server.poll()).has_value(); }));
    REQUIRE(receivedOnServer->packet.header.type == MessageType::Controllable);
    auto ctrlDecoded = readPayload<ControllablePacket>(receivedOnServer->packet);
    REQUIRE(ctrlDecoded.has_value());
    REQUIRE(ctrlDecoded->inputSequence == 10);
    REQUIRE(ctrlDecoded->actions == 0x05);
}

#include <criterion/criterion.h>

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

        void send(std::uint16_t port, Bytes datagram)
        {
            _socket.sendTo(loopback(port), std::move(datagram));
        }

        std::uint16_t port() const
        {
            return _socket.localPort();
        }

    private:
        UdpSocket _socket {0};
        std::thread _thread;
    };
} // namespace

Test(ThreadSafeQueue, popsInPushOrder)
{
    ThreadSafeQueue<int> queue(8);

    cr_assert(queue.push(1));
    cr_assert(queue.push(2));
    cr_assert(queue.push(3));
    cr_assert_eq(queue.tryPop(), 1);
    cr_assert_eq(queue.tryPop(), 2);
    cr_assert_eq(queue.tryPop(), 3);
}

Test(ThreadSafeQueue, tryPopOnEmptyQueueReturnsNothing)
{
    ThreadSafeQueue<int> queue(8);

    cr_assert_eq(queue.tryPop(), std::nullopt);
    queue.push(1);
    queue.tryPop();
    cr_assert_eq(queue.tryPop(), std::nullopt);
}

Test(ThreadSafeQueue, dropsNewValuesWhenFull)
{
    ThreadSafeQueue<int> queue(2);

    cr_assert(queue.push(1));
    cr_assert(queue.push(2));
    cr_assert_not(queue.push(3));
    cr_assert_eq(queue.tryPop(), 1);
    cr_assert(queue.push(4));
    cr_assert_eq(queue.tryPop(), 2);
    cr_assert_eq(queue.tryPop(), 4);
    cr_assert_eq(queue.tryPop(), std::nullopt);
}

Test(ThreadSafeQueue, keepsMoveOnlyValues)
{
    ThreadSafeQueue<std::unique_ptr<int>> queue(1);

    queue.push(std::make_unique<int>(42));
    auto value = queue.tryPop();

    cr_assert(value.has_value());
    cr_assert_eq(**value, 42);
}

Test(ThreadSafeQueue, passesEveryValueFromOneThreadToAnother)
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
            cr_assert_eq(*value, expected);
            ++expected;
        }
    }
    producer.join();
    cr_assert_eq(queue.tryPop(), std::nullopt);
}

Test(UdpSocket, sendsAndReceivesOnLoopback)
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
    cr_assert(
        waitFor([&] { return (datagram = received.tryPop()).has_value(); }));
    cr_assert_eq(datagram->first.address, "127.0.0.1");
    cr_assert_eq(datagram->first.port, sender.port());
    cr_assert(datagram->second == Bytes({1, 2, 3}));
    receiver.close();
    thread.join();
}

Test(UdpSocket, oversizedDatagramIsSeenTooLong)
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
    cr_assert(waitFor([&] { return (size = sizes.tryPop()).has_value(); }));
    cr_assert_gt(*size, MAX_PACKET_SIZE);
    receiver.close();
    thread.join();
}

Test(UdpSocket, closeStopsRunWithoutTraffic)
{
    UdpSocket socket(0);
    std::thread thread([&] { socket.run([](auto &, auto) {}); });

    socket.close();
    thread.join();
}

Test(NetworkThread, deliversValidPackets)
{
    NetworkThread network(0, 8);
    Sender sender;
    std::optional<NetworkThread::ReceivedPacket> received;

    network.start();
    sender.send(network.localPort(), heartbeatDatagram(1000));
    cr_assert(waitFor([&] { return (received = network.poll()).has_value(); }));
    cr_assert_eq(received->from.port, sender.port());
    cr_assert_eq(received->packet.header.type, MessageType::Heartbeat);

    auto heartbeat = readPayload<HeartbeatPacket>(received->packet);

    cr_assert(heartbeat.has_value());
    cr_assert_eq(heartbeat->clientTime, 1000u);
    cr_assert_eq(network.takeDroppedCount(), 0u);
}

Test(NetworkThread, dropsMalformedAndOversizedDatagrams)
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
    cr_assert(waitFor([&] { return (received = network.poll()).has_value(); }));
    cr_assert_eq(
        readPayload<HeartbeatPacket>(received->packet)->clientTime, 2u);
    cr_assert_eq(network.poll().has_value(), false);
    cr_assert_eq(network.takeDroppedCount(), 4u);
    cr_assert_eq(network.takeDroppedCount(), 0u);
}

Test(NetworkThread, dropsPacketsWhenQueueIsFull)
{
    NetworkThread network(0, 1);
    Sender sender;
    std::size_t dropped = 0;

    network.start();
    for (std::uint32_t i = 0; i < 3; ++i) {
        sender.send(network.localPort(), heartbeatDatagram(i));
    }
    cr_assert(
        waitFor([&] { return (dropped += network.takeDroppedCount()) >= 2; }));
    cr_assert_eq(dropped, 2u);

    auto first = network.poll();

    cr_assert(first.has_value());
    cr_assert_eq(readPayload<HeartbeatPacket>(first->packet)->clientTime, 0u);
    cr_assert_eq(network.poll().has_value(), false);
}

Test(NetworkThread, countsDatagramsDroppedByTheCaller)
{
    NetworkThread network(0, 1);

    network.countDropped();
    network.countDropped();
    cr_assert_eq(network.takeDroppedCount(), 2u);
}

Test(NetworkThread, stopIsSafeWithoutStartAndTwice)
{
    NetworkThread notStarted(0, 1);

    notStarted.stop();

    NetworkThread started(0, 1);

    started.start();
    started.stop();
    started.stop();
}

Test(NetworkThread, sendsBeforeStopAreDelivered)
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
    cr_assert(
        waitFor([&] { return (received = receiver.poll()).has_value(); }));
    cr_assert_eq(
        readPayload<HeartbeatPacket>(received->packet)->clientTime, 7u);
}

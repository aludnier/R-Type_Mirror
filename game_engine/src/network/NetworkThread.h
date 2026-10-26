/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** NetworkThread.h
*/

#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <thread>
#include <vector>

#include "Endpoint.h"
#include "Packet.h"
#include "ThreadSafeQueue.h"
#include "UdpSocket.h"

namespace RType::Network {
    /**
     * @brief Runs a UdpSocket on its own thread, so that the game loop is
     *        never blocked by the network. Shared by the server and the
     *        client.
     *
     * The network thread decodes each datagram, drops the malformed ones
     * (docs/protocol.md, section 4) and pushes the others into a bounded
     * queue. The game thread reads them with poll(), once per tick.
     */
    class NetworkThread {
    public:
        using Clock = std::chrono::steady_clock;

        /** @brief A datagram whose header and framing are valid. */
        struct ReceivedPacket {
            Endpoint from;
            Packet packet;
            Clock::time_point receivedAt;
        };

        /**
         * @param port 0 lets the system choose a free port (for a client).
         * @param queueSize Packets kept until poll() reads them. Beyond
         *        that, new packets are dropped.
         * @throws std::system_error If the port cannot be bound.
         */
        NetworkThread(std::uint16_t port, std::size_t queueSize);
        /** @brief Calls stop(). */
        ~NetworkThread();

        NetworkThread(const NetworkThread &) = delete;
        NetworkThread &operator=(const NetworkThread &) = delete;

        /** @brief Starts receiving. Call it once. */
        void start();

        /**
         * @brief Closes the socket once the datagrams sent before this call
         *        are out, then joins the thread. Safe to call more than
         *        once, and without start().
         */
        void stop();

        /** @brief Sends @p datagram to @p to. Thread-safe. */
        void sendTo(const Endpoint &to, std::vector<std::uint8_t> datagram) const;

        /** @return The oldest received packet, if any. Thread-safe. */
        std::optional<ReceivedPacket> poll();

        /**
         * @brief Counts a datagram dropped by the caller (e.g. a payload
         *        that cannot be decoded), next to the ones dropped here.
         */
        void countDropped();

        /** @return Datagrams dropped since the last call, then resets. */
        std::size_t takeDroppedCount();

        [[nodiscard]] std::uint16_t localPort() const;

    private:
        void onDatagram(
            const Endpoint &from, std::span<const std::uint8_t> datagram);

        UdpSocket _socket;
        ThreadSafeQueue<ReceivedPacket> _incoming;
        std::atomic<std::size_t> _dropped {0};
        std::thread _thread;
    };
} // namespace RType::Network

#endif // NETWORKTHREAD_H

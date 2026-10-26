/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** UdpSocket.h
*/

#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "Endpoint.h"

namespace RType::Network {
    /**
     * @brief UDP socket running on its own network thread.
     *
     * Encapsulates Asio: it is only included by UdpSocket.cpp, so neither
     * the game nor the server depend on it.
     *
     * Threading: run() is called by the network thread, and the receive
     * handler is called on that thread. sendTo() and close() may be called
     * from any thread: they hand the work over to the network thread, which
     * is the only one that touches the Asio socket.
     */
    class UdpSocket {
    public:
        /**
         * @brief Called on the network thread for every datagram received.
         *        The bytes are only valid during the call.
         */
        using ReceiveHandler = std::function<void(
            const Endpoint &from, std::span<const std::uint8_t> datagram)>;

        /**
         * @brief Opens an IPv4 socket bound to @p port on every interface.
         * @param port 0 lets the system choose a free port (for a client).
         * @throws std::system_error If the port cannot be bound.
         */
        explicit UdpSocket(std::uint16_t port);
        ~UdpSocket();

        UdpSocket(const UdpSocket &) = delete;
        UdpSocket &operator=(const UdpSocket &) = delete;

        /**
         * @brief Receives datagrams and sends the queued ones until close()
         *        is called. Blocks: call it from the network thread.
         *
         * Receive errors never stop the loop: a datagram that cannot be
         * received is skipped, and the socket keeps listening.
         */
        void run(ReceiveHandler handler) const;

        /**
         * @brief Queues @p datagram to be sent to @p to. Thread-safe. Send
         *        errors are ignored, as for any lost UDP datagram.
         */
        void sendTo(const Endpoint &to, std::vector<std::uint8_t> datagram) const;

        /**
         * @brief Closes the socket once the datagrams queued before this
         *        call are sent, then makes run() return. Thread-safe.
         */
        void close() const;

        [[nodiscard]] std::uint16_t localPort() const;

    private:
        // Pimpl: hides the Asio types from this header.
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
} // namespace RType::Network

#endif // UDPSOCKET_H

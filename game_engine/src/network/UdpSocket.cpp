/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** UdpSocket.cpp
*/

#include "UdpSocket.h"

#include <array>
#include <asio.hpp>
#include <iostream>

#include "Protocol.h"

namespace RType::Network {
    struct UdpSocket::Impl {
        explicit Impl(std::uint16_t port) :
            socket(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
        {
        }

        void startReceive()
        {
            socket.async_receive_from(asio::buffer(buffer), sender,
                [this](std::error_code error, std::size_t size) {
                    onReceive(error, size);
                });
        }

        void onReceive(std::error_code error, std::size_t size)
        {
            // Only close() stops the loop. Any other error is skipped: on
            // Windows, an ICMP "port unreachable" sent back by a crashed
            // client shows up here as connection_refused / reset, and must
            // not stop the server from hearing the other clients.
            if (error == asio::error::operation_aborted || !socket.is_open()) {
                return;
            }
            if (!error) {
                Endpoint from {sender.address().to_string(), sender.port()};

                handler(from, std::span(buffer.data(), size));
            }
            startReceive();
        }

        asio::io_context io;
        asio::ip::udp::socket socket;
        asio::ip::udp::endpoint sender;
        // One byte more than the maximum, so that an oversized datagram is
        // seen as too long (and dropped by Packet::deserialize()) instead of
        // being silently truncated to a valid size.
        std::array<std::uint8_t, MAX_PACKET_SIZE + 1> buffer {};
        ReceiveHandler handler;
    };

    UdpSocket::UdpSocket(std::uint16_t port) :
        _impl(std::make_unique<Impl>(port))
    {
    }

    UdpSocket::~UdpSocket() = default;

    void UdpSocket::run(ReceiveHandler handler)
    {
        _impl->handler = std::move(handler);
        _impl->startReceive();
        // Returns when there is no more work: after close(), the pending
        // receive is cancelled and the queued sends are done.
        _impl->io.run();
    }

    void UdpSocket::sendTo(const Endpoint &to, std::vector<std::uint8_t> datagram)
    {
        auto send = [impl = _impl.get(), to, bytes = std::move(datagram)] {
            std::error_code error;
            auto address = asio::ip::make_address(to.address, error);

            if (error || !impl->socket.is_open()) {
                return;
            }
            impl->socket.send_to(asio::buffer(bytes),
                asio::ip::udp::endpoint(address, to.port), 0, error);
            if (error) {
                std::cerr << "[network] send to " << to.toString()
                          << " failed: " << error.message() << '\n';
            }
        };

        asio::post(_impl->io, std::move(send));
    }

    void UdpSocket::close()
    {
        // Posted after the sends already queued, so they still go out:
        // handlers of a single-threaded io_context run in posting order.
        asio::post(_impl->io, [impl = _impl.get()] {
            std::error_code ignored;

            impl->socket.close(ignored);
        });
    }

    std::uint16_t UdpSocket::localPort() const
    {
        return _impl->socket.local_endpoint().port();
    }
} // namespace RType::Network

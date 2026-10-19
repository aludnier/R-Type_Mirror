/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** NetworkThread.cpp
*/

#include "NetworkThread.h"

namespace RType::Network {
    NetworkThread::NetworkThread(std::uint16_t port, std::size_t queueSize) :
        _socket(port), _incoming(queueSize)
    {
    }

    NetworkThread::~NetworkThread()
    {
        stop();
    }

    void NetworkThread::start()
    {
        _thread = std::thread([this] {
            _socket.run([this](const Endpoint &from,
                            std::span<const std::uint8_t>
                                datagram) { onDatagram(from, datagram); });
        });
    }

    void NetworkThread::stop()
    {
        if (_thread.joinable()) {
            _socket.close();
            _thread.join();
        }
    }

    void NetworkThread::sendTo(
        const Endpoint &to, std::vector<std::uint8_t> datagram)
    {
        _socket.sendTo(to, std::move(datagram));
    }

    std::optional<NetworkThread::ReceivedPacket> NetworkThread::poll()
    {
        return _incoming.tryPop();
    }

    void NetworkThread::countDropped()
    {
        ++_dropped;
    }

    std::size_t NetworkThread::takeDroppedCount()
    {
        return _dropped.exchange(0);
    }

    std::uint16_t NetworkThread::localPort() const
    {
        return _socket.localPort();
    }

    void NetworkThread::onDatagram(
        const Endpoint &from, std::span<const std::uint8_t> datagram)
    {
        auto packet = Packet::deserialize(datagram);

        if (!packet) {
            countDropped();
            return;
        }
        if (!_incoming.push({from, std::move(*packet), Clock::now()})) {
            countDropped();
        }
    }
} // namespace RType::Network

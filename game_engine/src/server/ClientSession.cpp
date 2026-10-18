/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ClientSession.cpp
*/

#include "ClientSession.h"

#include <algorithm>

namespace RType {
    ClientSession::ClientSession(Network::ClientId id,
        Network::Endpoint endpoint, Network::NetworkId linkedEntity,
        std::string playerName, Clock::time_point now) :
        _id(id),
        _endpoint(std::move(endpoint)),
        _linkedEntity(linkedEntity),
        _playerName(std::move(playerName)),
        _lastReceived(now)
    {
    }

    Network::ClientId ClientSession::id() const
    {
        return _id;
    }

    const Network::Endpoint &ClientSession::endpoint() const
    {
        return _endpoint;
    }

    Network::NetworkId ClientSession::linkedEntity() const
    {
        return _linkedEntity;
    }

    const std::string &ClientSession::playerName() const
    {
        return _playerName;
    }

    void ClientSession::onReceived(
        Network::Sequence sequence, Clock::time_point receivedAt)
    {
        // Datagrams can be reordered, so a late one must not move the
        // timeout backwards.
        _lastReceived = std::max(_lastReceived, receivedAt);
        if (!_hasReceived) {
            _hasReceived = true;
            _remoteAck = sequence;
            _remoteAckBits = 0;
            return;
        }
        if (Network::isSequenceMoreRecent(sequence, _remoteAck)) {
            // Newer: the old ack becomes bit (shift - 1). The cast handles
            // the wrap-around of the 16-bit counter.
            auto shift = static_cast<Network::Sequence>(sequence - _remoteAck);

            _remoteAckBits = shift < 32 ? _remoteAckBits << shift : 0;
            if (shift <= 32) {
                _remoteAckBits |= 1U << (shift - 1);
            }
            _remoteAck = sequence;
            return;
        }
        // Older (reordered) or duplicate: mark it if it fits in the 32 bits.
        auto age = static_cast<Network::Sequence>(_remoteAck - sequence);

        if (age >= 1 && age <= 32) {
            _remoteAckBits |= 1U << (age - 1);
        }
    }

    bool ClientSession::hasTimedOut(Clock::time_point now) const
    {
        return now - _lastReceived > Network::CONNECTION_TIMEOUT;
    }

    void ClientSession::updateControls(
        const Network::ControllablePacket &controls)
    {
        if (_hasControls && controls.inputSequence <= _controls.inputSequence) {
            return;
        }
        _controls = controls;
        _hasControls = true;
    }

    const Network::ControllablePacket &ClientSession::controls() const
    {
        return _controls;
    }

    void ClientSession::stampHeader(Network::PacketHeader &header)
    {
        header.sequence = _localSequence++;
        header.ack = _remoteAck;
        header.ackBits = _remoteAckBits;
    }
} // namespace RType

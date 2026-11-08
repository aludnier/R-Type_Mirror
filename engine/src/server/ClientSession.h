/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ClientSession.h
*/

#ifndef CLIENTSESSION_H
#define CLIENTSESSION_H

#include <chrono>
#include <string>

#include "network/ClientPackets.h"
#include "network/Endpoint.h"
#include "network/PacketHeader.h"
#include "network/Protocol.h"

namespace RType {
/**
 * @brief State the server keeps for one connected client, from its
 *        ASK_CONNECT to its disconnection (docs/protocol.md, section 8).
 */
class ClientSession {
public:
    using Clock = std::chrono::steady_clock;

    ClientSession(Network::ClientId id, Network::Endpoint endpoint,
        Network::NetworkId linkedEntity, std::string playerName,
        Clock::time_point now);

    [[nodiscard]] Network::ClientId id() const;
    [[nodiscard]] const Network::Endpoint &endpoint() const;
    [[nodiscard]] Network::NetworkId linkedEntity() const;
    [[nodiscard]] const std::string &playerName() const;

    /**
     * @brief Records a datagram received from the client: refreshes the
     *        timeout and the acknowledgement state.
     * @param sequence Sequence of the datagram received.
     * @param receivedAt When the network thread received it.
     */
    void onReceived(Network::Sequence sequence, Clock::time_point receivedAt);

    /** @return true if nothing was received for CONNECTION_TIMEOUT. */
    [[nodiscard]] bool hasTimedOut(Clock::time_point now) const;

    /**
     * @brief Keeps @p controls if they are more recent than the stored
     *        ones: only the highest input_sequence counts (section 7.3).
     */
    void updateControls(const Network::ControllablePacket &controls);

    [[nodiscard]] const Network::ControllablePacket &controls() const;

    /**
     * @brief Fills sequence, ack and ackBits of a header about to be sent
     *        to this client (section 7.1).
     */
    void stampHeader(Network::PacketHeader &header);

private:
    Network::ClientId _id;
    Network::Endpoint _endpoint;
    Network::NetworkId _linkedEntity;
    std::string _playerName;

    Clock::time_point _lastReceived;
    Network::ControllablePacket _controls;
    bool _hasControls = false;

    /** @brief Sequence of the next datagram sent to the client. */
    Network::Sequence _localSequence = 0;
    /** @brief Most recent sequence received from the client. */
    Network::Sequence _remoteAck = 0;
    /** @brief Bit i set: sequence _remoteAck - 1 - i was also received. */
    std::uint32_t _remoteAckBits = 0;
    bool _hasReceived = false;
};
} // namespace RType

#endif // CLIENTSESSION_H

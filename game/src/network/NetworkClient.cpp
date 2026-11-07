/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** NetworkClient implementation
*/

#include "network/NetworkClient.h"
#include <iostream>

namespace Game {

NetworkClient::NetworkClient() = default;

NetworkClient::~NetworkClient()
{
    disconnect();
    if (_network) {
        _network->stop();
    }
}

void NetworkClient::connect(
    const std::string &host, std::uint16_t port, const std::string &playerName)
{
    _serverHost = host;
    _serverPort = port;
    _playerName = playerName.empty() ? "Player" : playerName;
    _serverEndpoint = {host, port};

    _localSequence = 0;
    _remoteAck = 0;
    _remoteAckBits = 0;
    _hasReceived = false;
    _inputSequence = 0;
    _lastError.clear();
    _pendingMessages.clear();

    if (!_network) {
        try {
            _network = std::make_unique<RType::Network::NetworkThread>(0, 1024);
            _network->start();
        } catch (const std::exception &e) {
            _status = ConnectionStatus::ConnectionFailed;
            _lastError = std::string("Socket error: ") + e.what();
            return;
        }
    }

    _status = ConnectionStatus::Connecting;
    _connectAttempts = 0;
    _connectStartTime = Clock::now();
    _lastConnectAttempt = Clock::now() - RType::Network::CONNECT_RETRY_INTERVAL;
    _lastReceivedTime = Clock::now();

    sendAskConnect();
}

void NetworkClient::disconnect()
{
    if (_status == ConnectionStatus::Connected && _network) {
        for (int i = 0; i < 3; ++i) {
            send(RType::Network::AskDisconnectPacket {});
        }
    }
    _status = ConnectionStatus::Disconnected;
    _clientId = RType::Network::SERVER_CLIENT_ID;
    _linkedEntity = RType::Network::NO_ENTITY;
}

void NetworkClient::update(float /* dt */)
{
    auto now = Clock::now();

    pollIncomingPackets();

    if (_status == ConnectionStatus::Connecting) {
        updateConnectingState(now);
    } else if (_status == ConnectionStatus::Connected) {
        updateConnectedState(now);
    }
}

void NetworkClient::pollIncomingPackets()
{
    if (!_network) {
        return;
    }
    while (auto received = _network->poll()) {
        handleReceivedPacket(*received);
    }
}

void NetworkClient::updateConnectingState(Clock::time_point now)
{
    if (now - _lastConnectAttempt < RType::Network::CONNECT_RETRY_INTERVAL) {
        return;
    }

    if (_connectAttempts >= RType::Network::CONNECT_MAX_ATTEMPTS) {
        _status = ConnectionStatus::ConnectionFailed;
        _lastError = "Connection timed out (no response from server)";
    } else {
        sendAskConnect();
    }
}

void NetworkClient::updateConnectedState(Clock::time_point now)
{
    if (now - _lastHeartbeatSent >= RType::Network::HEARTBEAT_INTERVAL) {
        sendHeartbeat();
    }
    if (now - _lastReceivedTime > RType::Network::CONNECTION_TIMEOUT) {
        _status = ConnectionStatus::ConnectionFailed;
        _lastError = "Connection lost (server timed out)";
    }
}

void NetworkClient::sendControls(std::uint32_t actions)
{
    if (_status != ConnectionStatus::Connected) {
        return;
    }
    send(RType::Network::ControllablePacket {++_inputSequence, actions});
}

void NetworkClient::sendAskConnect()
{
    _lastConnectAttempt = Clock::now();
    ++_connectAttempts;
    send(RType::Network::AskConnectPacket {_playerName});
}

void NetworkClient::sendHeartbeat()
{
    _lastHeartbeatSent = Clock::now();
    auto clientTime = static_cast<std::uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            _lastHeartbeatSent.time_since_epoch())
            .count() &
        0xFFFFFFFF);
    send(RType::Network::HeartbeatPacket {clientTime});
}

void NetworkClient::recordSequence(
    RType::Network::Sequence seq, Clock::time_point receivedAt)
{
    _lastReceivedTime = std::max(_lastReceivedTime, receivedAt);
    if (!_hasReceived) {
        _hasReceived = true;
        _remoteAck = seq;
        _remoteAckBits = 0;
        return;
    }
    if (RType::Network::isSequenceMoreRecent(seq, _remoteAck)) {
        auto shift = static_cast<RType::Network::Sequence>(seq - _remoteAck);
        _remoteAckBits = shift < 32 ? _remoteAckBits << shift : 0;
        if (shift <= 32) {
            _remoteAckBits |= 1U << (shift - 1);
        }
        _remoteAck = seq;
        return;
    }
    auto age = static_cast<RType::Network::Sequence>(_remoteAck - seq);
    if (age >= 1 && age <= 32) {
        _remoteAckBits |= 1U << (age - 1);
    }
}

void NetworkClient::stampHeader(RType::Network::PacketHeader &header)
{
    header.sequence = _localSequence++;
    header.ack = _remoteAck;
    header.ackBits = _remoteAckBits;
}

void NetworkClient::handleReceivedPacket(
    const RType::Network::NetworkThread::ReceivedPacket &received)
{
    const auto &header = received.packet.header;
    if (header.version != RType::Network::PROTOCOL_VERSION) {
        return;
    }

    recordSequence(header.sequence, received.receivedAt);

    switch (header.type) {
        case RType::Network::MessageType::AcceptConnect: {
            if (auto payload = RType::Network::readPayload<
                    RType::Network::AcceptConnectPacket>(received.packet)) {
                _clientId = payload->clientId;
                _linkedEntity = payload->linkedEntity;
                _tickRate = payload->tickRate;
                _status = ConnectionStatus::Connected;
                _lastError.clear();
                _pendingMessages.emplace_back(*payload);
            }
            break;
        }
        case RType::Network::MessageType::ForceDisconnect: {
            if (auto payload = RType::Network::readPayload<
                    RType::Network::ForceDisconnectPacket>(received.packet)) {
                _status = ConnectionStatus::DisconnectedByServer;
                _lastError = payload->message.empty() ? "Disconnected by server"
                                                      : payload->message;
                _pendingMessages.emplace_back(*payload);
            }
            break;
        }
        case RType::Network::MessageType::PlayerConnected: {
            if (auto payload = RType::Network::readPayload<
                    RType::Network::PlayerConnectedPacket>(received.packet)) {
                _pendingMessages.emplace_back(*payload);
            }
            break;
        }
        case RType::Network::MessageType::PlayerDisconnected: {
            if (auto payload = RType::Network::readPayload<
                    RType::Network::PlayerDisconnectedPacket>(
                    received.packet)) {
                _pendingMessages.emplace_back(*payload);
            }
            break;
        }
        case RType::Network::MessageType::EntitySpawn: {
            if (auto payload = RType::Network::readPayload<
                    RType::Network::EntitySpawnPacket>(received.packet)) {
                _pendingMessages.emplace_back(*payload);
            }
            break;
        }
        case RType::Network::MessageType::EntityDespawn: {
            if (auto payload = RType::Network::readPayload<
                    RType::Network::EntityDespawnPacket>(received.packet)) {
                _pendingMessages.emplace_back(*payload);
            }
            break;
        }
        case RType::Network::MessageType::ComponentUpdate: {
            if (auto payload = RType::Network::readPayload<
                    RType::Network::ComponentUpdatePacket>(received.packet)) {
                _pendingMessages.emplace_back(*payload);
            }
            break;
        }
        case RType::Network::MessageType::HeartbeatReply: {
            if (auto payload = RType::Network::readPayload<
                    RType::Network::HeartbeatReplyPacket>(received.packet)) {
                auto nowMs = static_cast<std::uint32_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        Clock::now().time_since_epoch())
                        .count() &
                    0xFFFFFFFF);
                if (nowMs >= payload->clientTime) {
                    _pingMs = nowMs - payload->clientTime;
                }
                _serverTick = payload->serverTick;
                _pendingMessages.emplace_back(*payload);
            }
            break;
        }
        default:
            break;
    }
}

std::vector<NetworkClient::ServerMessage> NetworkClient::pollServerMessages()
{
    std::vector<ServerMessage> msgs;
    msgs.swap(_pendingMessages);
    return msgs;
}

NetworkClient::ConnectionStatus NetworkClient::getStatus() const
{
    return _status;
}

bool NetworkClient::isConnected() const
{
    return _status == ConnectionStatus::Connected;
}

RType::Network::ClientId NetworkClient::getClientId() const
{
    return _clientId;
}

RType::Network::NetworkId NetworkClient::getLinkedEntity() const
{
    return _linkedEntity;
}

std::uint16_t NetworkClient::getTickRate() const
{
    return _tickRate;
}

RType::Network::Tick NetworkClient::getServerTick() const
{
    return _serverTick;
}

std::uint32_t NetworkClient::getPingMs() const
{
    return _pingMs;
}

const std::string &NetworkClient::getLastError() const
{
    return _lastError;
}

const std::string &NetworkClient::getServerHost() const
{
    return _serverHost;
}

std::uint16_t NetworkClient::getServerPort() const
{
    return _serverPort;
}

const std::string &NetworkClient::getPlayerName() const
{
    return _playerName;
}

} // namespace Game

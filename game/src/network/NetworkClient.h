/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** NetworkClient header
*/

#ifndef GAME_NETWORKCLIENT_H
#define GAME_NETWORKCLIENT_H

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <variant>

#include "network/ClientPackets.h"
#include "network/Endpoint.h"
#include "network/NetworkThread.h"
#include "network/Packet.h"
#include "network/Protocol.h"
#include "network/ServerPackets.h"

namespace Game {

    /**
     * @brief High-level client networking class managing server connection, packets, and heartbeat.
     */
    class NetworkClient {
    public:
        using Clock = std::chrono::steady_clock;

        enum class ConnectionStatus {
            Disconnected,
            Connecting,
            Connected,
            ConnectionFailed,
            DisconnectedByServer
        };

        using ServerMessage = std::variant<
            RType::Network::AcceptConnectPacket,
            RType::Network::ForceDisconnectPacket,
            RType::Network::PlayerConnectedPacket,
            RType::Network::PlayerDisconnectedPacket,
            RType::Network::EntitySpawnPacket,
            RType::Network::EntityDespawnPacket,
            RType::Network::ComponentUpdatePacket,
            RType::Network::HeartbeatReplyPacket
        >;

        NetworkClient();
        ~NetworkClient();

        NetworkClient(const NetworkClient &) = delete;
        NetworkClient &operator=(const NetworkClient &) = delete;

        void connect(const std::string &host, std::uint16_t port, const std::string &playerName);
        void disconnect();
        void update(float dt);
        void sendControls(std::uint32_t actions);

        [[nodiscard]] ConnectionStatus getStatus() const;
        [[nodiscard]] bool isConnected() const;
        [[nodiscard]] RType::Network::ClientId getClientId() const;
        [[nodiscard]] RType::Network::NetworkId getLinkedEntity() const;
        [[nodiscard]] std::uint16_t getTickRate() const;
        [[nodiscard]] RType::Network::Tick getServerTick() const;
        [[nodiscard]] std::uint32_t getPingMs() const;
        [[nodiscard]] const std::string &getLastError() const;
        [[nodiscard]] const std::string &getServerHost() const;
        [[nodiscard]] std::uint16_t getServerPort() const;
        [[nodiscard]] const std::string &getPlayerName() const;

        /**
         * @brief Polls all received server messages since the last call.
         */
        std::vector<ServerMessage> pollServerMessages();

    private:
        void pollIncomingPackets();
        void updateConnectingState(Clock::time_point now);
        void updateConnectedState(Clock::time_point now);
        void handleReceivedPacket(const RType::Network::NetworkThread::ReceivedPacket &received);
        void recordSequence(RType::Network::Sequence seq, Clock::time_point receivedAt);
        void sendAskConnect();
        void sendHeartbeat();

        template<RType::Network::PacketPayload P>
        void send(const P &payload)
        {
            if (!_network) {
                return;
            }
            auto packet = RType::Network::makePacket(payload);
            stampHeader(packet.header);
            _network->sendTo(_serverEndpoint, packet.serialize());
        }

        void stampHeader(RType::Network::PacketHeader &header);

        std::unique_ptr<RType::Network::NetworkThread> _network;
        RType::Network::Endpoint _serverEndpoint;
        std::string _serverHost = "127.0.0.1";
        std::uint16_t _serverPort = 4242;
        std::string _playerName = "Player";

        ConnectionStatus _status = ConnectionStatus::Disconnected;
        std::string _lastError;

        RType::Network::ClientId _clientId = RType::Network::SERVER_CLIENT_ID;
        RType::Network::NetworkId _linkedEntity = RType::Network::NO_ENTITY;
        std::uint16_t _tickRate = RType::Network::DEFAULT_TICK_RATE;
        RType::Network::Tick _serverTick = 0;
        std::uint32_t _pingMs = 0;
        std::uint32_t _inputSequence = 0;

        // Sequence tracking
        RType::Network::Sequence _localSequence = 0;
        RType::Network::Sequence _remoteAck = 0;
        std::uint32_t _remoteAckBits = 0;
        bool _hasReceived = false;

        // Timing
        Clock::time_point _connectStartTime;
        Clock::time_point _lastConnectAttempt;
        std::size_t _connectAttempts = 0;
        Clock::time_point _lastHeartbeatSent;
        Clock::time_point _lastReceivedTime;

        std::vector<ServerMessage> _pendingMessages;
    };

} // namespace Game

#endif // GAME_NETWORKCLIENT_H

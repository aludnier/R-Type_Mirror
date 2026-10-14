/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameServer.h
*/

#ifndef GAMESERVER_H
#define GAMESERVER_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <thread>

#include "ClientSession.h"
#include "network/Endpoint.h"
#include "network/Packet.h"
#include "network/Protocol.h"
#include "network/ThreadSafeQueue.h"
#include "network/UdpSocket.h"

namespace RType {
    struct ServerConfig {
        std::uint16_t port = 4242;
        /** @brief Player slots. R-Type is played by up to 4 players. */
        std::size_t maxPlayers = 4;
        std::uint16_t tickRate = Network::DEFAULT_TICK_RATE;
    };

    /**
     * @brief Authoritative server: runs the fixed-timestep loop and manages
     *        the client sessions (docs/protocol.md, section 8).
     *
     * Two threads:
     * - the network thread runs the UdpSocket. It decodes each datagram,
     *   drops the malformed ones and pushes the others into a queue;
     * - the game thread runs run(). It is the only one that touches the
     *   sessions (and later the registry), so they need no lock.
     */
    class GameServer {
    public:
        using Clock = std::chrono::steady_clock;

        /** @throws std::system_error If the port cannot be bound. */
        explicit GameServer(ServerConfig config);
        ~GameServer();

        GameServer(const GameServer &) = delete;
        GameServer &operator=(const GameServer &) = delete;

        /**
         * @brief Runs the game loop until @p stopRequested becomes true, then
         *        disconnects every client. Blocks: call it from the main
         *        thread.
         */
        void run(const std::atomic<bool> &stopRequested);

    private:
        /** @brief A valid datagram, handed from the network thread. */
        struct ReceivedPacket {
            Network::Endpoint from;
            Network::Packet packet;
            Clock::time_point receivedAt;
        };

        // Network thread
        void onDatagram(const Network::Endpoint &from,
            std::span<const std::uint8_t> datagram);

        // Game thread
        void tick();
        void processIncoming();
        void handlePacket(const ReceivedPacket &received);
        void handleAskConnect(const ReceivedPacket &received);
        void handleSessionPacket(
            ClientSession &session, const ReceivedPacket &received);
        void checkTimeouts(Clock::time_point now);
        void reportDroppedDatagrams();

        /**
         * @brief Removes the session and tells the other clients, with
         *        PLAYER_DISCONNECTED.
         */
        void closeSession(
            Network::ClientId id, Network::DisconnectReason reason);

        /**
         * @brief Sends FORCE_DISCONNECT to every client, then stops the
         *        network thread.
         */
        void shutdown();

        ClientSession *findSession(const Network::Endpoint &endpoint);

        /** @return The lowest free id in [1, maxPlayers], if any. */
        [[nodiscard]] std::optional<Network::ClientId> findFreeClientId() const;

        /**
         * @brief Decodes the payload of @p received as a @p P.
         * @return std::nullopt, and counts the datagram as dropped, if the
         *         payload is invalid.
         */
        template<Network::PacketPayload P>
        std::optional<P> decode(const ReceivedPacket &received)
        {
            auto payload = Network::readPayload<P>(received.packet);

            if (!payload) {
                ++_droppedDatagrams;
            }
            return payload;
        }

        template<Network::PacketPayload P>
        void send(ClientSession &session, const P &payload)
        {
            auto packet = Network::makePacket(payload);

            session.stampHeader(packet.header);
            _socket.sendTo(session.endpoint(), packet.serialize());
        }

        /**
         * @brief Sends to an endpoint that has no session (e.g. a refused
         *        ASK_CONNECT). Sequence and acks are left to 0.
         */
        template<Network::PacketPayload P>
        void sendWithoutSession(const Network::Endpoint &to, const P &payload)
        {
            _socket.sendTo(to, Network::makePacket(payload).serialize());
        }

        ServerConfig _config;
        Network::UdpSocket _socket;
        std::thread _networkThread;
        Network::ThreadSafeQueue<ReceivedPacket> _incoming;

        std::map<Network::ClientId, ClientSession> _sessions;
        std::map<Network::Endpoint, Network::ClientId> _sessionIds;

        /** @brief Never reused during a run (docs/protocol.md, section 3). */
        Network::NetworkId _nextNetworkId = 1;
        Network::Tick _tick = 0;

        /** @brief Malformed or unexpected datagrams, for the logs. */
        std::atomic<std::size_t> _droppedDatagrams {0};
    };
} // namespace RType

#endif // GAMESERVER_H

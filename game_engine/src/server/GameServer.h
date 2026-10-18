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
#include <vector>

#include "ClientSession.h"
#include "network/Endpoint.h"
#include "network/NetworkThread.h"
#include "network/Packet.h"
#include "network/Protocol.h"
#include "network/ServerPackets.h"

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
     * - the network thread (Network::NetworkThread) receives the datagrams,
     *   drops the malformed ones and queues the others;
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

        /** @return The UDP port the server listens on. */
        [[nodiscard]] std::uint16_t localPort() const;

    private:
        using ReceivedPacket = Network::NetworkThread::ReceivedPacket;

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
                _network.countDropped();
            }
            return payload;
        }

        template<Network::PacketPayload P>
        void send(ClientSession &session, const P &payload)
        {
            auto packet = Network::makePacket(payload);

            sendPacket(session, packet);
        }

        /**
         * @brief Sends @p payload to the client @p id. Does nothing if it has
         *        no session.
         */
        template<Network::PacketPayload P>
        void sendTo(Network::ClientId id, const P &payload)
        {
            auto it = _sessions.find(id);

            if (it != _sessions.end()) {
                send(it->second, payload);
            }
        }

        /**
         * @brief Sends @p payload to every connected client. It is encoded
         *        once, each client still gets its own sequence and acks.
         */
        template<Network::PacketPayload P>
        void sendToAll(const P &payload)
        {
            auto packet = Network::makePacket(payload);

            broadcastPacket(packet, std::nullopt);
        }

        /**
         * @brief Same as sendToAll(), except to the client @p excluded
         *        (e.g. the one whose arrival is being announced).
         */
        template<Network::PacketPayload P>
        void sendToAllExcept(Network::ClientId excluded, const P &payload)
        {
            auto packet = Network::makePacket(payload);

            broadcastPacket(packet, excluded);
        }

        /**
         * @brief Sends the COMPONENT_UPDATE of @p entities to every client,
         *        split into as many packets as needed.
         */
        void sendComponentUpdate(
            const std::vector<Network::EntityComponents> &entities);

        /** @brief Stamps the header of @p packet for @p session, sends it. */
        void sendPacket(ClientSession &session, Network::Packet &packet);

        /** @brief Sends @p packet to every client but @p excluded. */
        void broadcastPacket(Network::Packet &packet,
            std::optional<Network::ClientId> excluded);

        /**
         * @brief Sends to an endpoint that has no session (e.g. a refused
         *        ASK_CONNECT). Sequence and acks are left to 0.
         */
        template<Network::PacketPayload P>
        void sendWithoutSession(const Network::Endpoint &to, const P &payload)
        {
            _network.sendTo(to, Network::makePacket(payload).serialize());
        }

        ServerConfig _config;
        Network::NetworkThread _network;

        std::map<Network::ClientId, ClientSession> _sessions;
        std::map<Network::Endpoint, Network::ClientId> _sessionIds;

        /** @brief Never reused during a run (docs/protocol.md, section 3). */
        Network::NetworkId _nextNetworkId = 1;
        Network::Tick _tick = 0;
    };
} // namespace RType

#endif // GAMESERVER_H

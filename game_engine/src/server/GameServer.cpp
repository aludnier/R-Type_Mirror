/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameServer.cpp
*/

#include "GameServer.h"

#include <algorithm>
#include <iostream>
#include <thread>
#include <vector>

#include "network/ClientPackets.h"
#include "network/ServerPackets.h"

namespace RType {
    namespace {
        /**
         * @brief Bound of the inbound queue. At 60 ticks/s and 4 players,
         *        a tick normally receives a handful of datagrams.
         */
        constexpr std::size_t INCOMING_QUEUE_SIZE = 1024;

        /**
         * @brief Ticks the loop may run back to back to catch up after a
         *        stall. Beyond that, the lost time is skipped instead of
         *        making the server fall further and further behind.
         */
        constexpr int MAX_CATCH_UP_TICKS = 5;
    } // namespace

    GameServer::GameServer(ServerConfig config) :
        _config(config), _network(config.port, INCOMING_QUEUE_SIZE)
    {
    }

    GameServer::~GameServer() = default;

    void GameServer::run(const std::atomic<bool> &stopRequested)
    {
        _network.start();
        std::cout << "[server] listening on UDP port " << _network.localPort()
                  << ", " << _config.tickRate << " ticks/s\n";

        // Fixed timestep: the simulation advances by whole ticks, whatever
        // the speed of the machine. lag is the time not simulated yet.
        const auto tickDuration = std::chrono::duration_cast<Clock::duration>(
            std::chrono::seconds(1)) / _config.tickRate;
        auto previous = Clock::now();
        Clock::duration lag {0};

        while (!stopRequested) {
            auto now = Clock::now();

            lag += now - previous;
            previous = now;
            lag = std::min(lag, tickDuration * MAX_CATCH_UP_TICKS);
            while (lag >= tickDuration) {
                tick();
                lag -= tickDuration;
            }
            // Sleep until the next tick is due instead of spinning.
            std::this_thread::sleep_for(tickDuration - lag);
        }
        shutdown();
    }

    std::uint16_t GameServer::localPort() const
    {
        return _network.localPort();
    }

    void GameServer::tick()
    {
        processIncoming();
        checkTimeouts(Clock::now());
        // TODO(ECS): registry.run_systems(), then replicate the changes with
        // sendComponentUpdate().
        if (_tick % _config.tickRate == 0) {
            reportDroppedDatagrams();
        }
        ++_tick;
    }

    void GameServer::processIncoming()
    {
        while (auto received = _network.poll()) {
            handlePacket(*received);
        }
    }

    void GameServer::handlePacket(const ReceivedPacket &received)
    {
        const auto &header = received.packet.header;
        ClientSession *session = findSession(received.from);

        // Section 4: only an ASK_CONNECT gets an answer to a bad version.
        if (header.version != Network::PROTOCOL_VERSION) {
            if (header.type == Network::MessageType::AskConnect &&
                session == nullptr) {
                sendWithoutSession(received.from,
                    Network::ForceDisconnectPacket {
                        Network::DisconnectReason::BadVersion,
                        "protocol version " +
                            std::to_string(Network::PROTOCOL_VERSION) +
                            " expected"});
            }
            _network.countDropped();
            return;
        }
        if (session != nullptr) {
            handleSessionPacket(*session, received);
        } else if (header.type == Network::MessageType::AskConnect) {
            handleAskConnect(received);
        } else {
            // Section 2: packets from an endpoint without a session are
            // ignored.
            _network.countDropped();
        }
    }

    void GameServer::handleAskConnect(const ReceivedPacket &received)
    {
        auto ask = decode<Network::AskConnectPacket>(received);

        if (!ask) {
            return;
        }
        auto id = findFreeClientId();

        if (!id) {
            sendWithoutSession(received.from,
                Network::ForceDisconnectPacket {
                    Network::DisconnectReason::ServerFull, "server is full"});
            return;
        }
        // TODO(ECS): spawn the player's entity and use its NetworkId.
        Network::NetworkId linkedEntity = _nextNetworkId++;
        auto [it, inserted] = _sessions.try_emplace(*id, *id, received.from,
            linkedEntity, ask->playerName, received.receivedAt);
        ClientSession &session = it->second;

        _sessionIds[received.from] = *id;
        session.onReceived(
            received.packet.header.sequence, received.receivedAt);
        std::cout << "[server] client " << *id << " (\"" << ask->playerName
                  << "\") connected from " << received.from.toString() << '\n';

        send(session,
            Network::AcceptConnectPacket {
                *id, linkedEntity, _config.tickRate});
        // TODO(ECS): one ENTITY_SPAWN per replicated entity, its own
        // included.
        for (auto &[otherId, other] : _sessions) {
            if (otherId != *id) {
                send(session,
                    Network::PlayerConnectedPacket {
                        otherId, other.linkedEntity(), other.playerName()});
            }
        }
        sendToAllExcept(*id,
            Network::PlayerConnectedPacket {
                *id, linkedEntity, session.playerName()});
    }

    void GameServer::handleSessionPacket(
        ClientSession &session, const ReceivedPacket &received)
    {
        const auto &header = received.packet.header;

        switch (header.type) {
            case Network::MessageType::AskConnect: {
                // Section 8.1: our ACCEPT_CONNECT was lost, send it again.
                // No second session is created.
                if (!decode<Network::AskConnectPacket>(received)) {
                    return;
                }
                session.onReceived(header.sequence, received.receivedAt);
                send(session,
                    Network::AcceptConnectPacket {session.id(),
                        session.linkedEntity(), _config.tickRate});
                break;
            }
            case Network::MessageType::AskDisconnect: {
                if (!decode<Network::AskDisconnectPacket>(received)) {
                    return;
                }
                closeSession(session.id(), Network::DisconnectReason::Left);
                break;
            }
            case Network::MessageType::Controllable: {
                auto controls = decode<Network::ControllablePacket>(received);

                if (!controls) {
                    return;
                }
                session.onReceived(header.sequence, received.receivedAt);
                session.updateControls(*controls);
                break;
            }
            case Network::MessageType::Heartbeat: {
                auto heartbeat = decode<Network::HeartbeatPacket>(received);

                if (!heartbeat) {
                    return;
                }
                session.onReceived(header.sequence, received.receivedAt);
                send(session,
                    Network::HeartbeatReplyPacket {
                        heartbeat->clientTime, _tick});
                break;
            }
            default:
                // Server to client types, or game messages not handled yet.
                _network.countDropped();
                break;
        }
    }

    void GameServer::checkTimeouts(Clock::time_point now)
    {
        std::vector<Network::ClientId> timedOut;

        for (const auto &[id, session] : _sessions) {
            if (session.hasTimedOut(now)) {
                timedOut.push_back(id);
            }
        }
        // Closed after the loop: closeSession() erases from _sessions.
        for (auto id : timedOut) {
            closeSession(id, Network::DisconnectReason::Timeout);
        }
    }

    void GameServer::reportDroppedDatagrams()
    {
        auto dropped = _network.takeDroppedCount();

        if (dropped > 0) {
            std::cerr << "[server] dropped " << dropped
                      << " invalid or unexpected datagram(s)\n";
        }
    }

    void GameServer::closeSession(
        Network::ClientId id, Network::DisconnectReason reason)
    {
        auto it = _sessions.find(id);

        if (it == _sessions.end()) {
            return;
        }
        std::cout << "[server] client " << id << " disconnected ("
                  << (reason == Network::DisconnectReason::Timeout ? "timeout"
                                                                   : "left")
                  << ")\n";
        _sessionIds.erase(it->second.endpoint());
        _sessions.erase(it);
        // TODO(ECS): despawn the linked entity and send ENTITY_DESPAWN.
        sendToAll(Network::PlayerDisconnectedPacket {id, reason});
    }

    void GameServer::shutdown()
    {
        std::cout << "[server] shutting down\n";
        sendToAll(Network::ForceDisconnectPacket {
            Network::DisconnectReason::ServerShutdown, "server shutdown"});
        _sessions.clear();
        _sessionIds.clear();
        // The FORCE_DISCONNECTs above are sent before the socket closes.
        _network.stop();
    }

    void GameServer::sendComponentUpdate(
        const std::vector<Network::EntityComponents> &entities)
    {
        for (const auto &update :
            Network::splitComponentUpdate(_tick, entities)) {
            sendToAll(update);
        }
    }

    void GameServer::sendPacket(
        ClientSession &session, Network::Packet &packet)
    {
        // Overwrites sequence and acks: the same packet can be stamped again
        // for the next client.
        session.stampHeader(packet.header);
        _network.sendTo(session.endpoint(), packet.serialize());
    }

    void GameServer::broadcastPacket(
        Network::Packet &packet, std::optional<Network::ClientId> excluded)
    {
        for (auto &[id, session] : _sessions) {
            if (id != excluded) {
                sendPacket(session, packet);
            }
        }
    }

    ClientSession *GameServer::findSession(const Network::Endpoint &endpoint)
    {
        auto it = _sessionIds.find(endpoint);

        return it == _sessionIds.end() ? nullptr : &_sessions.at(it->second);
    }

    std::optional<Network::ClientId> GameServer::findFreeClientId() const
    {
        for (std::size_t id = 1; id <= _config.maxPlayers; ++id) {
            auto clientId = static_cast<Network::ClientId>(id);

            if (!_sessions.contains(clientId)) {
                return clientId;
            }
        }
        return std::nullopt;
    }
} // namespace RType

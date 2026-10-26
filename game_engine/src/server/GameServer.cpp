/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameServer.cpp
*/

#include "GameServer.h"

#include <algorithm>
#include <iostream>
#include <ranges>
#include <thread>
#include <vector>

#include "components/Controllable.h"
#include "components/Transform2D.h"
#include "network/ByteWriter.h"
#include "network/ClientPackets.h"
#include "network/ServerPackets.h"
#include "systems/ControllableSystem.h"

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

        /** @brief Where the player @p id appears: one row per player. */
        Vector2 playerSpawnPosition(Network::ClientId id)
        {
            return {100.f, 100.f * static_cast<float>(id)};
        }

        /** @brief POSITION component (docs/protocol.md, section 6.3). */
        Network::ComponentData encodePosition(const Vector2 &position)
        {
            Network::ByteWriter writer;

            writer.writeFloat(position.x);
            writer.writeFloat(position.y);
            return {static_cast<Network::ComponentTypeId>(
                        Network::EngineComponentType::Position),
                writer.release()};
        }
    } // namespace

    GameServer::GameServer(const ServerConfig &config) :
        _config(config), _network(config.port, INCOMING_QUEUE_SIZE)
    {
        setupRegistry();
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
        applyControls();
        _registry.runSystems();
        replicate();
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
        Network::NetworkId linkedEntity = spawnPlayer(*id);
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
        for (const auto &networkId : _replicated | std::views::keys) {
            send(session, makeSpawnPacket(networkId));
        }
        for (auto &[otherId, other] : _sessions) {
            if (otherId != *id) {
                send(session,
                    Network::PlayerConnectedPacket {
                        otherId, other.linkedEntity(), other.playerName()});
            }
        }
        sendToAllExcept(*id, makeSpawnPacket(linkedEntity));
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
        auto linkedEntity = it->second.linkedEntity();

        _sessionIds.erase(it->second.endpoint());
        _sessions.erase(it);
        despawn(linkedEntity);
        sendToAll(Network::PlayerDisconnectedPacket {id, reason});
    }

    void GameServer::setupRegistry()
    {
        _registry.registerComponent<Transform2D>();
        _registry.registerComponent<Controllable>();
        addControllableSystem(_registry, _config.playerSpeed,
            1.f / static_cast<float>(_config.tickRate));
    }

    Network::NetworkId GameServer::spawnPlayer(Network::ClientId id)
    {
        Entity entity = _registry.spawnEntity();
        Network::NetworkId networkId = _nextNetworkId++;

        _registry.addComponent(entity, Transform2D {playerSpawnPosition(id)});
        _registry.addComponent(entity, Controllable {});
        _replicated.emplace(networkId, entity);
        return networkId;
    }

    Registry &GameServer::registry()
    {
        return _registry;
    }

    float GameServer::tickDelta() const
    {
        return 1.f / static_cast<float>(_config.tickRate);
    }

    Network::NetworkId GameServer::replicateEntity(Entity entity,
        std::vector<Network::ComponentData> spawnComponents, bool predictable)
    {
        Network::NetworkId networkId = _nextNetworkId++;

        _replicated.emplace(networkId, entity);
        if (!spawnComponents.empty()) {
            _spawnComponents.emplace(networkId, std::move(spawnComponents));
        }
        if (predictable) {
            _predictable.insert(networkId);
        }
        sendToAll(makeSpawnPacket(networkId));
        return networkId;
    }

    void GameServer::despawnEntity(Entity entity)
    {
        auto it = std::ranges::find(
            _replicated, entity, [](const auto &pair) { return pair.second; });

        if (it != _replicated.end()) {
            despawn(it->first);
        }
    }

    void GameServer::despawn(Network::NetworkId networkId)
    {
        auto it = _replicated.find(networkId);

        if (it == _replicated.end()) {
            return;
        }
        // Deferred by the registry: the components go at the end of the
        // next runSystems().
        _registry.killEntity(it->second);
        _replicated.erase(it);
        _lastSentPositions.erase(networkId);
        _spawnComponents.erase(networkId);
        _predictable.erase(networkId);
        sendToAll(Network::EntityDespawnPacket {networkId});
    }

    void GameServer::applyControls()
    {
        auto &controllables = _registry.getComponents<Controllable>();

        for (const auto &session : _sessions | std::views::values) {
            auto entity = _replicated.find(session.linkedEntity());

            if (entity == _replicated.end()) {
                continue;
            }
            // Controllable holds 8 actions: the higher bits are not used.
            controllables[entity->second] = Controllable {
                static_cast<std::uint8_t>(session.controls().actions)};
        }
    }

    void GameServer::replicate()
    {
        auto &transforms = _registry.getComponents<Transform2D>();
        bool sendAll = _tick % _config.tickRate == 0;
        std::vector<Network::EntityComponents> updates;

        for (const auto &[networkId, entity] : _replicated) {
            const auto &transform = transforms[entity];

            if (!transform) {
                continue;
            }
            auto last = _lastSentPositions.find(networkId);

            if (!sendAll && last != _lastSentPositions.end() &&
                (last->second == transform->position ||
                    _predictable.contains(networkId))) {
                continue;
            }
            _lastSentPositions[networkId] = transform->position;
            updates.push_back({networkId, {encodePosition(transform->position)}});
        }
        sendComponentUpdate(updates);
    }

    Network::EntitySpawnPacket GameServer::makeSpawnPacket(
        Network::NetworkId networkId)
    {
        auto components = replicatedComponents(_replicated.at(networkId));
        auto extra = _spawnComponents.find(networkId);

        if (extra != _spawnComponents.end()) {
            components.insert(
                components.end(), extra->second.begin(), extra->second.end());
        }
        return {networkId, std::move(components)};
    }

    std::vector<Network::ComponentData> GameServer::replicatedComponents(
        Entity entity)
    {
        std::vector<Network::ComponentData> components;

        if (const auto &transform =
                _registry.getComponents<Transform2D>()[entity]) {
            components.push_back(encodePosition(transform->position));
        }
        return components;
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
        ClientSession &session, Network::Packet &packet) const
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

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "components/Action.h"
#include "network/ByteReader.h"
#include "network/ClientPackets.h"
#include "network/Endpoint.h"
#include "network/NetworkThread.h"
#include "network/Packet.h"
#include "network/Protocol.h"
#include "network/ServerPackets.h"
#include "server/GameServer.h"

using namespace RType;
using namespace RType::Network;

namespace {
    constexpr auto TIMEOUT = std::chrono::seconds(2);
    /** @brief How long a client listens to check that nothing comes. */
    constexpr auto SILENCE = std::chrono::milliseconds(200);

    /** @brief A GameServer running on its own thread, on a free port. */
    class RunningServer {
    public:
        RunningServer() : _thread([this] { _server.run(_stopRequested); }) {}

        ~RunningServer()
        {
            stop();
        }

        /** @brief Stops the loop: the server sends its FORCE_DISCONNECTs. */
        void stop()
        {
            _stopRequested = true;
            if (_thread.joinable()) {
                _thread.join();
            }
        }

        std::uint16_t port() const
        {
            return _server.localPort();
        }

    private:
        std::atomic<bool> _stopRequested {false};
        GameServer _server {ServerConfig {.port = 0}};
        std::thread _thread;
    };

    /** @brief A minimal client: sends raw packets, records what it gets. */
    class TestClient {
    public:
        explicit TestClient(std::uint16_t serverPort) :
            _server {"127.0.0.1", serverPort}
        {
            _network.start();
        }

        template<PacketPayload P>
        void send(const P &payload)
        {
            auto packet = makePacket(payload);

            packet.header.sequence = _sequence++;
            _network.sendTo(_server, packet.serialize());
        }

        /**
         * @return The next packet, if one comes before @p timeout.
         *         COMPONENT_UPDATEs are skipped unless @p withUpdates: the
         *         server sends them on its own every second.
         */
        std::optional<Packet> next(
            std::chrono::milliseconds timeout = TIMEOUT,
            bool withUpdates = false)
        {
            auto deadline = std::chrono::steady_clock::now() + timeout;

            while (std::chrono::steady_clock::now() < deadline) {
                if (auto received = _network.poll()) {
                    _sequences.push_back(received->packet.header.sequence);
                    if (!withUpdates &&
                        received->packet.header.type ==
                            MessageType::ComponentUpdate) {
                        continue;
                    }
                    return received->packet;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return std::nullopt;
        }

        /** @return The next packet decoded as a @p P, if it is one. */
        template<PacketPayload P>
        std::optional<P> expect()
        {
            auto packet = next();

            return packet ? readPayload<P>(*packet) : std::nullopt;
        }

        /**
         * @return The position of @p networkId in the next COMPONENT_UPDATE
         *         that carries it, if one comes before @p timeout.
         */
        std::optional<std::pair<float, float>> nextPosition(
            NetworkId networkId, std::chrono::milliseconds timeout = TIMEOUT)
        {
            auto deadline = std::chrono::steady_clock::now() + timeout;

            while (true) {
                auto remaining =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        deadline - std::chrono::steady_clock::now());

                if (remaining.count() <= 0) {
                    break;
                }
                auto packet = next(remaining, true);

                if (!packet) {
                    break;
                }
                auto update = readPayload<ComponentUpdatePacket>(*packet);

                if (!update) {
                    continue;
                }
                for (const auto &[entityNetworkId, components] : update->entities) {
                    if (entityNetworkId != networkId) {
                        continue;
                    }
                    if (auto position = decodePosition(components)) {
                        return position;
                    }
                }
            }
            return std::nullopt;
        }

        /** @return The POSITION among @p components, if there is one. */
        static std::optional<std::pair<float, float>> decodePosition(
            const std::vector<ComponentData> &components)
        {
            for (const auto &[typeId, data] : components) {
                if (typeId !=
                    static_cast<ComponentTypeId>(
                        EngineComponentType::Position)) {
                    continue;
                }
                ByteReader reader(data);
                float x = reader.readFloat();
                float y = reader.readFloat();

                return std::pair {x, y};
            }
            return std::nullopt;
        }

        bool receivesNothing()
        {
            return !next(SILENCE).has_value();
        }

        /** @brief Sends ASK_CONNECT and waits for ACCEPT_CONNECT. */
        AcceptConnectPacket connect(const std::string &name)
        {
            send(AskConnectPacket {name});
            auto accept = expect<AcceptConnectPacket>();

            INFO("no ACCEPT_CONNECT for " << name);
            REQUIRE(accept.has_value());
            return *accept;
        }

        /** @brief Sequences of every packet received so far, in order. */
        [[nodiscard]] const std::vector<Sequence> &sequences() const
        {
            return _sequences;
        }

    private:
        Endpoint _server;
        NetworkThread _network {0, 64};
        Sequence _sequence = 0;
        std::vector<Sequence> _sequences;
    };

    void assertSequencesFollow(const TestClient &client)
    {
        const auto &sequences = client.sequences();

        for (std::size_t i = 0; i < sequences.size(); ++i) {
            INFO("packet " << i << " has sequence " << sequences[i]);
            REQUIRE(sequences[i] == i);
        }
    }
} // namespace

TEST_CASE("Server spawns the player's entity on connection",
    "[server][sending][ecs]")
{
    RunningServer server;
    TestClient alice(server.port());
    auto accept = alice.connect("alice");

    REQUIRE(accept.linkedEntity != NO_ENTITY);
    // Its own entity, with the position it appears at.
    auto spawn = alice.expect<EntitySpawnPacket>();

    REQUIRE(spawn.has_value());
    REQUIRE(spawn->networkId == accept.linkedEntity);
    REQUIRE(TestClient::decodePosition(spawn->components).has_value());
    REQUIRE(alice.receivesNothing());
}

TEST_CASE("Server announces a new player to the others only",
    "[server][sending]")
{
    RunningServer server;
    TestClient alice(server.port());
    TestClient bob(server.port());
    auto aliceAccept = alice.connect("alice");

    REQUIRE(alice.expect<EntitySpawnPacket>().has_value());
    auto bobAccept = bob.connect("bob");

    // Bob gets every entity, Alice's and his own, then is told about Alice,
    // who was already there.
    std::vector<NetworkId> spawnedForBob;

    for (int i = 0; i < 2; ++i) {
        auto spawn = bob.expect<EntitySpawnPacket>();

        REQUIRE(spawn.has_value());
        spawnedForBob.push_back(spawn->networkId);
    }
    REQUIRE(spawnedForBob ==
        std::vector {aliceAccept.linkedEntity, bobAccept.linkedEntity});
    auto aliceForBob = bob.expect<PlayerConnectedPacket>();

    REQUIRE(aliceForBob.has_value());
    REQUIRE(aliceForBob->clientId == aliceAccept.clientId);
    REQUIRE(aliceForBob->linkedEntity == aliceAccept.linkedEntity);
    REQUIRE(aliceForBob->playerName == "alice");

    // Alice gets Bob's entity and is told about him, Bob is not told about
    // himself.
    auto bobEntity = alice.expect<EntitySpawnPacket>();

    REQUIRE(bobEntity.has_value());
    REQUIRE(bobEntity->networkId == bobAccept.linkedEntity);
    auto bobForAlice = alice.expect<PlayerConnectedPacket>();

    REQUIRE(bobForAlice.has_value());
    REQUIRE(bobForAlice->clientId == bobAccept.clientId);
    REQUIRE(bobForAlice->playerName == "bob");
    REQUIRE(alice.receivesNothing());
    REQUIRE(bob.receivesNothing());
}

TEST_CASE("Server announces a departure to the remaining clients",
    "[server][sending]")
{
    RunningServer server;
    TestClient alice(server.port());
    TestClient bob(server.port());
    TestClient carol(server.port());
    auto aliceAccept = alice.connect("alice");

    bob.connect("bob");
    carol.connect("carol");
    // Drain the connection: each client gets 3 ENTITY_SPAWN and 2
    // PLAYER_CONNECTED, in an order that depends on when it joined.
    for (auto *client : {&alice, &bob, &carol}) {
        int spawns = 0;
        int players = 0;

        while (spawns < 3 || players < 2) {
            auto packet = client->next();

            REQUIRE(packet.has_value());
            if (packet->header.type == MessageType::EntitySpawn) {
                ++spawns;
            } else if (packet->header.type == MessageType::PlayerConnected) {
                ++players;
            }
        }
    }

    alice.send(AskDisconnectPacket {});
    for (auto *client : {&bob, &carol}) {
        auto despawn = client->expect<EntityDespawnPacket>();

        REQUIRE(despawn.has_value());
        REQUIRE(despawn->networkId == aliceAccept.linkedEntity);
        auto left = client->expect<PlayerDisconnectedPacket>();

        REQUIRE(left.has_value());
        REQUIRE(left->clientId == aliceAccept.clientId);
        REQUIRE(left->reason == DisconnectReason::Left);
    }
    REQUIRE(alice.receivesNothing());
    // A packet sent to everyone still uses each client's own sequence.
    for (auto *client : {&alice, &bob, &carol}) {
        assertSequencesFollow(*client);
    }
}

TEST_CASE("Server moves a player's entity with its controls",
    "[server][sending][ecs]")
{
    RunningServer server;
    TestClient alice(server.port());
    auto accept = alice.connect("alice");
    auto spawn = alice.expect<EntitySpawnPacket>();

    REQUIRE(spawn.has_value());
    auto start = TestClient::decodePosition(spawn->components);

    REQUIRE(start.has_value());
    alice.send(ControllablePacket {1,
        1U << static_cast<unsigned>(Action::MoveRight)});
    // The entity moves right only: x grows, y stays.
    std::optional<std::pair<float, float>> position;

    for (int i = 0; i < 10; ++i) {
        position = alice.nextPosition(accept.linkedEntity);
        REQUIRE(position.has_value());
        if (position->first > start->first) {
            break;
        }
    }
    REQUIRE(position->first > start->first);
    REQUIRE(position->second == start->second);
}

TEST_CASE("Server disconnects every client on shutdown",
    "[server][sending]")
{
    RunningServer server;
    TestClient alice(server.port());
    TestClient bob(server.port());

    alice.connect("alice");
    bob.connect("bob");
    REQUIRE(alice.expect<EntitySpawnPacket>().has_value());
    REQUIRE(alice.expect<EntitySpawnPacket>().has_value());
    REQUIRE(alice.expect<PlayerConnectedPacket>().has_value());
    REQUIRE(bob.expect<EntitySpawnPacket>().has_value());
    REQUIRE(bob.expect<EntitySpawnPacket>().has_value());
    REQUIRE(bob.expect<PlayerConnectedPacket>().has_value());

    server.stop();
    for (auto *client : {&alice, &bob}) {
        auto shutdown = client->expect<ForceDisconnectPacket>();

        REQUIRE(shutdown.has_value());
        REQUIRE(shutdown->reason == DisconnectReason::ServerShutdown);
        assertSequencesFollow(*client);
    }
}

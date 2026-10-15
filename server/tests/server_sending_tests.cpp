#include <criterion/criterion.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "GameServer.h"
#include "network/ClientPackets.h"
#include "network/Endpoint.h"
#include "network/NetworkThread.h"
#include "network/Packet.h"
#include "network/Protocol.h"
#include "network/ServerPackets.h"

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

        /** @return The next packet, if one comes before @p timeout. */
        std::optional<Packet> next(
            std::chrono::milliseconds timeout = TIMEOUT)
        {
            auto deadline = std::chrono::steady_clock::now() + timeout;

            while (std::chrono::steady_clock::now() < deadline) {
                if (auto received = _network.poll()) {
                    _sequences.push_back(received->packet.header.sequence);
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

        bool receivesNothing()
        {
            return !next(SILENCE).has_value();
        }

        /** @brief Sends ASK_CONNECT and waits for ACCEPT_CONNECT. */
        ClientId connect(const std::string &name)
        {
            send(AskConnectPacket {name});
            auto accept = expect<AcceptConnectPacket>();

            cr_assert(accept.has_value(), "no ACCEPT_CONNECT for %s",
                name.c_str());
            return accept->clientId;
        }

        /** @brief Sequences of every packet received so far, in order. */
        const std::vector<Sequence> &sequences() const
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
            cr_assert_eq(sequences[i], i, "packet %zu has sequence %u", i,
                static_cast<unsigned>(sequences[i]));
        }
    }
} // namespace

Test(ServerSending, announcesANewPlayerToTheOthersOnly)
{
    RunningServer server;
    TestClient alice(server.port());
    TestClient bob(server.port());
    ClientId aliceId = alice.connect("alice");
    ClientId bobId = bob.connect("bob");

    // Bob is told about Alice, who was already there.
    auto aliceForBob = bob.expect<PlayerConnectedPacket>();

    cr_assert(aliceForBob.has_value());
    cr_assert_eq(aliceForBob->clientId, aliceId);
    cr_assert_eq(aliceForBob->playerName, "alice");

    // Alice is told about Bob, Bob is not told about himself.
    auto bobForAlice = alice.expect<PlayerConnectedPacket>();

    cr_assert(bobForAlice.has_value());
    cr_assert_eq(bobForAlice->clientId, bobId);
    cr_assert_eq(bobForAlice->playerName, "bob");
    cr_assert(alice.receivesNothing());
    cr_assert(bob.receivesNothing());
}

Test(ServerSending, announcesADepartureToTheRemainingClients)
{
    RunningServer server;
    TestClient alice(server.port());
    TestClient bob(server.port());
    TestClient carol(server.port());
    ClientId aliceId = alice.connect("alice");

    bob.connect("bob");
    carol.connect("carol");
    // Drain the PLAYER_CONNECTEDs: 2 for Alice, 2 for Bob, 2 for Carol.
    for (auto *client : {&alice, &bob, &carol}) {
        cr_assert(client->expect<PlayerConnectedPacket>().has_value());
        cr_assert(client->expect<PlayerConnectedPacket>().has_value());
    }

    alice.send(AskDisconnectPacket {});
    for (auto *client : {&bob, &carol}) {
        auto left = client->expect<PlayerDisconnectedPacket>();

        cr_assert(left.has_value());
        cr_assert_eq(left->clientId, aliceId);
        cr_assert_eq(left->reason, DisconnectReason::Left);
    }
    cr_assert(alice.receivesNothing());
    // A packet sent to everyone still uses each client's own sequence.
    for (auto *client : {&alice, &bob, &carol}) {
        assertSequencesFollow(*client);
    }
}

Test(ServerSending, disconnectsEveryClientOnShutdown)
{
    RunningServer server;
    TestClient alice(server.port());
    TestClient bob(server.port());

    alice.connect("alice");
    bob.connect("bob");
    cr_assert(alice.expect<PlayerConnectedPacket>().has_value());
    cr_assert(bob.expect<PlayerConnectedPacket>().has_value());

    server.stop();
    for (auto *client : {&alice, &bob}) {
        auto shutdown = client->expect<ForceDisconnectPacket>();

        cr_assert(shutdown.has_value());
        cr_assert_eq(shutdown->reason, DisconnectReason::ServerShutdown);
        assertSequencesFollow(*client);
    }
}

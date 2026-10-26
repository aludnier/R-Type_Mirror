/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** main.cpp
*/

#include <atomic>
#include <charconv>
#include <csignal>
#include <exception>
#include <iostream>
#include <optional>
#include <string_view>

#include "RTypeServer.h"

namespace {
    // Written by the signal handler: only lock-free atomics are safe there.
    std::atomic stopRequested {false};
    static_assert(std::atomic<bool>::is_always_lock_free);

    void onSignal(int /* signal */)
    {
        stopRequested = true;
    }

    std::optional<std::uint16_t> parsePort(std::string_view text)
    {
        unsigned int port = 0;
        auto [end, error] =
            std::from_chars(text.data(), text.data() + text.size(), port);

        if (error != std::errc {} || end != text.data() + text.size() ||
            port == 0 || port > 65535) {
            return std::nullopt;
        }
        return static_cast<std::uint16_t>(port);
    }

    void printUsage(std::string_view program)
    {
        std::cout << "USAGE: " << program << " [-p port]\n"
                  << "    -p port    UDP port to listen on (default 4242)\n";
    }
} // namespace

int main(int argc, char **argv)
{
    RType::ServerConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "-p" && i + 1 < argc) {
            auto port = parsePort(argv[++i]);

            if (!port) {
                std::cerr << "Invalid port: " << argv[i] << '\n';
                return 84;
            }
            config.port = *port;
            continue;
        }
        printUsage(argv[0]);
        return 84;
    }
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);
    try {
        RType::RTypeServer server(config);

        server.run(stopRequested);
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 84;
    }
    return 0;
}

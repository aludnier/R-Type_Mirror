/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** main.cpp
*/

#include <atomic>
#include <csignal>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <filesystem>

#include "RTypeServer.h"
#include "configuration/Configuration.hpp"

namespace {
// Written by the signal handler: only lock-free atomics are safe there.
std::atomic stopRequested {false};
static_assert(std::atomic<bool>::is_always_lock_free);

void onSignal(int /* signal */)
{
    stopRequested = true;
}

void printUsage(std::string_view program)
{
    std::cout << "USAGE: " << program << " [-c config]\n"
              << "    -c config  Shared game configuration file\n";
}
} // namespace

int main(int argc, char **argv)
{
    std::filesystem::path configFile = "../config/game.json";

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        if (arg == "-c" && i + 1 < argc) {
            configFile = argv[++i];
            continue;
        }
        printUsage(argv[0]);
        return 84;
    }
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);
    try {
        const RType::Configuration configuration(configFile);
        RType::RTypeServer server(configuration);

        server.run(stopRequested);
    } catch (const std::exception &error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 84;
    }
    return 0;
}

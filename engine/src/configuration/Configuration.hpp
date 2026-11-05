/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Configuration.hpp
*/

#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <nlohmann/json.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace RType {
struct GameConfig {
    std::string name = "R-Type";
    std::string pluginDirectory = "modules";
    std::vector<std::string> plugins;
    std::uint16_t port = 4242;
    std::size_t maxPlayers = 4;
    std::uint16_t tickRate = 60;
    float playerSpeed = 300.f;
    std::string gameName = "R-Type";
};

class Configuration {
public:
    explicit Configuration(const std::filesystem::path &configFilePath);
    ~Configuration() = default;

    [[nodiscard]] const GameConfig &game() const;
    [[nodiscard]] const std::filesystem::path &path() const;

private:
    std::filesystem::path _path;
    GameConfig _game;
};
} // namespace RType

#endif

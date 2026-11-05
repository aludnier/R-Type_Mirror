/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Configuration.cpp
*/

#include "Configuration.hpp"

#include <fstream>
#include <stdexcept>

namespace RType {
Configuration::Configuration(const std::filesystem::path &configFilePath) :
    _path(configFilePath)
{
    std::ifstream file(configFilePath);
    if (!file)
        throw std::runtime_error("Cannot open configuration: " + configFilePath.string());
    try {
        nlohmann::json json;
        file >> json;
        _game.name = json.value("name", _game.name);
        _game.gameName = _game.name;
        _game.pluginDirectory = json.value("plugin_directory",
            json.value("directory", _game.pluginDirectory));
        _game.plugins = json.value("plugins", json.value("modules", _game.plugins));
        _game.port = json.value("port", _game.port);
        _game.maxPlayers = json.value("max_players", _game.maxPlayers);
        _game.tickRate = json.value("tick_rate", _game.tickRate);
        _game.playerSpeed = json.value("player_speed", _game.playerSpeed);
    } catch (const nlohmann::json::exception &error) {
        throw std::runtime_error(
            "Invalid configuration " + configFilePath.string() + ": " +
            error.what());
    }
}

const GameConfig &Configuration::game() const
{
    return _game;
}

const std::filesystem::path &Configuration::path() const
{
    return _path;
}

} // RType
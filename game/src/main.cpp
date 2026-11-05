/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Main entry point for R-Type game client prototype
*/

#include "GameApp.h"
#include "configuration/Configuration.hpp"

int main(int argc, char **argv)
{
    const std::filesystem::path configFile =
        argc > 1 ? argv[1] : "../config/game.json";
    const RType::Configuration configuration(configFile);
    Game::GameApp app(configuration);
    return app.run();
}

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Main entry point for R-Type game client prototype
*/

#include "GameApp.h"

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;
    Game::GameApp app;
    return app.run();
}

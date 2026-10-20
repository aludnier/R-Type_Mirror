/*
** EPITECH PROJECT, 2026
** R-type
** File description:
** engine unit test
*/

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <thread>

#include "library/engine.h"

TEST_CASE("EngineLib game loop can be stopped", "[engine]") {
    EngineLib engine;
    engine.setActionPerSeconds(2);
    std::thread gameLoopThread([&engine]() {
        engine.gameLoop();
    });

    std::this_thread::sleep_for(std::chrono::seconds(2));
    engine.stopGameLoop();
    gameLoopThread.join();

    SUCCEED("game loop stopped and joined");
}

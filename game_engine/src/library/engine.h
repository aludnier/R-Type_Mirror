/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** engine
*/

#ifndef ENGINE_H
#define ENGINE_H
#include "ecs/Registry.h"
#include <chrono>
#include <iostream>
class EngineLib
{
public:
    EngineLib(/* args */);
    ~EngineLib();

    using clock = std::chrono::steady_clock;

    void gameLoop() {

        clock::time_point lastAction = clock::now();
        std::chrono::duration<double, std::milli> timeStep = std::chrono::milliseconds(1000) / _actionPerSeconds;

        _isRunning = true;
        while (_isRunning) {
            std::this_thread::sleep_until(lastAction + timeStep);
            lastAction = clock::now();
            _registry.runSystems();
        }
    };

    void setActionPerSeconds(uint aps) {
        if (aps == 0) {
            return;
        }
        _actionPerSeconds = aps;
    }

    void stopGameLoop() {
        _isRunning = false;
    }

private:
    RType::Registry _registry;
    uint _actionPerSeconds = 60;
    bool _isRunning = false;
};

EngineLib::EngineLib(/* args */)
{
}

EngineLib::~EngineLib()
{
}


#endif

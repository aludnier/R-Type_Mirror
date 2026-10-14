/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** engine
*/

#ifndef ENGINE_H
#define ENGINE_H
#include "ecs/Registry.h"

class EngineLib
{
public:
    EngineLib(/* args */);
    ~EngineLib();

    void gameLoop() {
        bool isRunning = true;
        while (isRunning) {

        }
    };
private:
    RType::Registry _registry;
    int _actionPerSeconds;
};

EngineLib::EngineLib(/* args */)
{
}

EngineLib::~EngineLib()
{
}


#endif

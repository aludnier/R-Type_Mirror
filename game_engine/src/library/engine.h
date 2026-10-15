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
#include <cstdint>
#include <string>
#include <thread>
#include <unordered_map>
#include "ecs/SparseArray.h"

namespace {
    struct Sprite {
        std::string path;
    };
}

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

    void setActionPerSeconds(std::uint32_t aps) {
        if (aps == 0) {
            return;
        }
        _actionPerSeconds = aps;
    }

    void stopGameLoop() {
        _isRunning = false;
    }

    void importAsset(std::string assetName, std::string assetPath) {
        _assets[assetName] = assetPath;
    }

    void applyAsset(RType::Entity entity, std::string assetName, std::string assetPath = "") {
        if (!_assets.contains(assetName)) {
            if (assetPath.empty()) {
                return;
            }
            _assets[assetName] = assetPath;
        }
        _registry.addComponent<Sprite>(entity, Sprite{_assets[assetName]});
    }

private:
    RType::Registry _registry;
    std::uint32_t _actionPerSeconds = 60;
    bool _isRunning = false;
    std::unordered_map<std::string, std::string> _assets;
};

inline EngineLib::EngineLib(/* args */)
{
    _registry.registerComponent<Sprite>();
}

inline EngineLib::~EngineLib()
{
}


#endif

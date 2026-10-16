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
#include <cstdint>
#include <string>
#include <thread>
#include "InputManager.h"
#include "RessourceManager.h"

namespace {
    struct Sprite {
        std::uint32_t assetId;
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
        _resources.addAsset(assetName, assetPath);
    }

    void applyAsset(RType::Entity entity, std::string assetName, std::string assetPath = "") {
        try {
            _resources.getAsset(assetName);
        } catch (const std::exception &) {
            _resources.addAsset(assetName, assetPath);
        }
        _registry.addComponent<Sprite>(entity, Sprite{_resources.getAsset(assetName).id});
    }

    void bindAction(Action action, InputManager::KeyCode key) {
        _input.bind(action, key);
    }

    void unbindAction(Action action, InputManager::KeyCode key) {
        _input.unbind(action, key);
    }

    void clearActionBindings(Action action) {
        _input.clearBindings(action);
    }

    void setKeyState(InputManager::KeyCode key, bool pressed) {
        _input.setKeyState(key, pressed);
    }

    bool isActionDown(Action action) const {
        return _input.isActionDown(action);
    }

    std::uint8_t actionMask() const {
        return _input.actionMask();
    }

    ResourceManager &resources() {
        return _resources;
    }

    const ResourceManager &resources() const {
        return _resources;
    }

    InputManager &input() {
        return _input;
    }

    const InputManager &input() const {
        return _input;
    }

private:
    ResourceManager _resources;
    InputManager _input;
    RType::Registry _registry;
    std::uint32_t _actionPerSeconds = 60;
    bool _isRunning = false;
};

inline EngineLib::EngineLib(/* args */)
{
    _registry.registerComponent<Sprite>();
}

inline EngineLib::~EngineLib()
{
}

#endif

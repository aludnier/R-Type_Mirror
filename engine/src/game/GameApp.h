/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Reusable game application base
*/

#ifndef ENGINE_GAMEAPP_H
#define ENGINE_GAMEAPP_H

#include <SFML/Graphics.hpp>
#include <memory>

#include "DLLoader.hpp"
#include "Module.h"
#include "ecs/Registry.h"
#include "library/InputManager.h"
#include "library/RessourceManager.h"

namespace Game {

class GameApp {
public:
    GameApp();
    virtual ~GameApp() = default;

    void init();
    void processEvents();
    void update(float dt);
    void render();
    int run();

    [[nodiscard]] RType::Registry &getRegistry();
    [[nodiscard]] InputManager &getInputManager();
    [[nodiscard]] ResourceManager &getResourceManager();
    [[nodiscard]] sf::RenderWindow &getWindow() const;

protected:
    virtual void registerComponents() {}
    virtual void setupResources() {}
    virtual void setupSystems() {}
    virtual void spawnInitialEntities() {}
    virtual void updateGame(float) {}
    virtual void renderGame() {}
    virtual void onWindowCreated() {}
    virtual void onShutdown() {}
    virtual void handleEvent(const sf::Event &event);

    void setupKeyBindings();
    void close();

    InputManager _inputManager {};
    ResourceManager _resourceManager {};
    std::unique_ptr<sf::RenderWindow> _window {nullptr};
    RType::Registry _registry {};
    RType::DLLoader<RType::ModuleDescriptor> _physics2DModuleLoader {};
    RType::DLLoader<RType::ModuleDescriptor> _environmentModuleLoader {};
    bool _running = false;
    float _frameRate = 60.0f;
    float _fixedDeltaTime = 1.0f / 60.0f;
};

} // namespace Game

#endif // ENGINE_GAMEAPP_H

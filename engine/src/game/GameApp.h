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
#include "configuration/Configuration.hpp"
#include "ecs/Registry.h"
#include "library/InputManager.h"
#include "library/PluginManager.h"
#include "library/RessourceManager.h"

namespace RType::Game {

class GameApp {
public:
    explicit GameApp(const Configuration &configuration);
    virtual ~GameApp() = default;

    void init();
    void processEvents();
    void update(float dt);
    void render();
    int run();

    [[nodiscard]] Registry &getRegistry();
    [[nodiscard]] InputManager &getInputManager();
    [[nodiscard]] ResourceManager &getResourceManager();
    [[nodiscard]] sf::RenderWindow &getWindow() const;

protected:
    virtual void registerComponents()
    {
    }

    virtual void setupResources()
    {
    }

    virtual void setupSystems();

    virtual void spawnInitialEntities()
    {
    }

    virtual void updateGame(float)
    {
    }

    virtual void renderGame()
    {
    }

    virtual void onWindowCreated()
    {
    }

    virtual void onShutdown()
    {
    }

    virtual void handleEvent(const sf::Event &event);

    virtual void setupKeyBindings();
    void close();

    InputManager _inputManager {};
    ResourceManager _resourceManager {};
    Configuration _configuration;
    PluginManager _pluginManager;
    std::unique_ptr<sf::RenderWindow> _window {nullptr};
    Registry _registry {};
    bool _running = false;
    float _frameRate = 60.0f;
    float _fixedDeltaTime = 1.0f / 60.0f;
};

} // namespace RType::Game

#endif // ENGINE_GAMEAPP_H

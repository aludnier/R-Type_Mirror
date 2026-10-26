/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameApp header
*/

#ifndef GAME_GAMEAPP_H
#define GAME_GAMEAPP_H

#include <SFML/Graphics.hpp>
#include <memory>
#include "components/ParallaxBackground.h"
#include "ecs/Registry.h"
#include "library/InputManager.h"
#include "library/RessourceManager.h"
#include "network/NetworkClient.h"
#include "systems/NetworkSyncSystem.h"
#include "ui/ConnectionDialog.h"

#include "DLLoader.hpp"
#include "Module.h"

namespace Game {

/**
 * @brief Screen state indicating whether the client is in the connection lobby
 * or active gameplay.
 */
enum class ScreenState { Connection, Game };

using ParallaxBackground = RType::Environment::ParallaxBackground;

/**
 * @brief Main Game Application class managing game lifecycle, systems,
 * resources, and main loop.
 */
class GameApp {
public:
    GameApp();
    ~GameApp() = default;

    void init();
    void setupResources();
    void setupKeyBindings();
    void setupSystems();
    void spawnInitialEntities();

    void processEvents();
    void update(float dt);
    void render();
    int run();

    [[nodiscard]] ScreenState getScreenState() const;
    void setScreenState(ScreenState state);

    [[nodiscard]] RType::Registry &getRegistry();
    [[nodiscard]] InputManager &getInputManager();
    [[nodiscard]] ResourceManager &getResourceManager();
    [[nodiscard]] NetworkClient &getNetworkClient();
    [[nodiscard]] sf::RenderWindow &getWindow() const;

private:
    void handleEvent(const sf::Event &event);
    void handleKeyPressed(const sf::Event::KeyPressed &keyPressed);
    void handleKeyReleased(const sf::Event::KeyReleased &keyReleased);
    RType::Registry _registry{};
    InputManager _inputManager{};
    ResourceManager _resourceManager{};
    NetworkClient _networkClient{};
    ConnectionDialog _connectionDialog{};
    NetworkSyncSystem _networkSyncSystem{};
    std::unique_ptr<sf::RenderWindow> _window{nullptr};
    ScreenState _screenState = ScreenState::Connection;
    bool _running = false;
    float _frameRate = 60.0f;
    float _fixedDeltaTime = 1.0f / 60.0f;
    RType::DLLoader<RType::ModuleDescriptor> _physics2DModuleLoader{};
    RType::DLLoader<RType::ModuleDescriptor> _environmentModuleLoader{};
};

} // namespace Game

#endif // GAME_GAMEAPP_H

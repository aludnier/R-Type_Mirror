/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameApp header
*/

#ifndef GAME_GAMEAPP_H
#define GAME_GAMEAPP_H

#include "ecs/Registry.h"
#include "library/InputManager.h"
#include "library/RessourceManager.h"
#include "network/NetworkClient.h"
#include "ui/ConnectionDialog.h"
#include "systems/NetworkSyncSystem.h"
#include "components/ParallaxBackground.h"
#include <SFML/Graphics.hpp>
#include <memory>

namespace Game {

    /**
     * @brief Screen state indicating whether the client is in the connection lobby or active gameplay.
     */
    enum class ScreenState {
        Connection,
        Game
    };

    using ParallaxBackground = RType::Environment::ParallaxBackground;

    /**
     * @brief Main Game Application class managing game lifecycle, systems, resources, and main loop.
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

        RType::Registry &getRegistry();
        InputManager &getInputManager();
        ResourceManager &getResourceManager();
        NetworkClient &getNetworkClient();
        sf::RenderWindow &getWindow();

    private:
        void handleEvent(const sf::Event &event);
        void handleKeyPressed(const sf::Event::KeyPressed &keyPressed);
        void handleKeyReleased(const sf::Event::KeyReleased &keyReleased);
        RType::Registry _registry;
        InputManager _inputManager;
        ResourceManager _resourceManager;
        NetworkClient _networkClient;
        ConnectionDialog _connectionDialog;
        NetworkSyncSystem _networkSyncSystem;
        std::unique_ptr<sf::RenderWindow> _window;
        ScreenState _screenState = ScreenState::Connection;
        bool _running = false;
        float _frameRate = 60.0f;
        float _fixedDeltaTime = 1.0f / 60.0f;
    };

} // namespace Game

#endif // GAME_GAMEAPP_H

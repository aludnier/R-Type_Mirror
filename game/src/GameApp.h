/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** R-Type game application
*/

#ifndef GAME_GAMEAPP_H
#define GAME_GAMEAPP_H

#include <imgui-SFML.h>
#include <memory>

#include "components/ParallaxBackground.h"
#include "game/GameApp.h"
#include "network/NetworkClient.h"
#include "systems/NetworkSyncSystem.h"
#include "ui/ConnectionDialog.h"

namespace Game {

enum class ScreenState { Connection, Game };
using ParallaxBackground = RType::Environment::ParallaxBackground;

class GameApp : public RType::Game::GameApp {
public:
    explicit GameApp(const RType::Configuration &configuration) :
        RType::Game::GameApp(configuration)
    {
    }

    ~GameApp() override = default;

    [[nodiscard]] ScreenState getScreenState() const;
    void setScreenState(ScreenState state);
    [[nodiscard]] NetworkClient &getNetworkClient();

protected:
    void registerComponents() override;
    void setupResources() override;
    void setupSystems() override;
    void spawnInitialEntities() override;
    void updateGame(float dt) override;
    void renderGame() override;
    void onWindowCreated() override;
    void onShutdown() override;
    void handleEvent(const sf::Event &event) override;

private:
    void handleKeyPressed(const sf::Event::KeyPressed &keyPressed);
    void handleKeyReleased(const sf::Event::KeyReleased &keyReleased);

    NetworkClient _networkClient {};
    ConnectionDialog _connectionDialog {};
    NetworkSyncSystem _networkSyncSystem {};
    ScreenState _screenState = ScreenState::Connection;
};

} // namespace Game

#endif // GAME_GAMEAPP_H

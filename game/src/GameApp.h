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
#include "scripting/ScriptEngine.h"
#include "systems/NetworkSyncSystem.h"
#include "ui/ImGuiUiRenderer.h"
#include "ui/UiManager.h"

namespace Game {

enum class ScreenState { Connection, Game };
using ParallaxBackground = RType::Environment::ParallaxBackground;

class GameApp : public RType::Game::GameApp {
public:
    explicit GameApp(const RType::Configuration &configuration);

    ~GameApp() override = default;

    [[nodiscard]] ScreenState getScreenState() const;
    void setScreenState(ScreenState state);
    [[nodiscard]] NetworkClient &getNetworkClient();
    [[nodiscard]] RType::UI::UiManager &getUiManager();

protected:
    void registerComponents() override;
    void setupKeyBindings() override;
    void setupResources() override;
    void setupSystems() override;
    void spawnInitialEntities() override;
    void updateGame(float dt) override;
    void renderGame() override;
    void onWindowCreated() override;
    void onShutdown() override;
    void handleEvent(const sf::Event &event) override;

private:
    void renderHud();
    void setupUi();
    void handleUiClicks();
    void updateConnectionStatus();
    void updateInGameOverlay();

    void handleKeyPressed(const sf::Event::KeyPressed &keyPressed);
    void handleKeyReleased(const sf::Event::KeyReleased &keyReleased);

    bool _inGameOverlayVisible = false;

    NetworkClient _networkClient {};
    NetworkSyncSystem _networkSyncSystem {};
    RType::UI::UiManager _uiManager {};
    RType::UI::ImGuiUiRenderer _uiRenderer {};
    ScreenState _screenState = ScreenState::Connection;
    std::unique_ptr<RType::ScriptEngine> _scripts;
};

} // namespace Game

#endif // GAME_GAMEAPP_H

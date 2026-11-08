/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** R-Type game application
*/

#include "GameApp.h"

#include <SFML/Window/Keyboard.hpp>
#include <cmath>
#include <imgui.h>
#include <iostream>

#include "Physics2DBindings.h"
#include "components/Controllable.h"
#include "components/Enemy.h"
#include "components/ForcePod.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Name.h"
#include "components/NetworkEntity.h"
#include "components/Player.h"
#include "components/Projectile.h"
#include "components/Transform2D.h"
#include "ecs/Zipper.h"
#include "scripting/GameBindings.h"
#include "scripting/ScriptEngine.h"
#include "scripting/ScriptSystem.h"
#include "systems/RenderSystem.h"

namespace Game {

void GameApp::registerComponents()
{
    auto &registry = getRegistry();
    registry.registerComponent<NetworkEntity>();
    registry.registerComponent<Player>();
    registry.registerComponent<Enemy>();
    registry.registerComponent<Projectile>();
    registry.registerComponent<Health>();
    registry.registerComponent<Hitbox>();
    registry.registerComponent<ParallaxBackground>();
    registry.registerComponent<ForcePod>();
}

void GameApp::setupResources()
{
    getClientAssetManager().metadata().loadConfigFile(
        _configuration.path().parent_path() / "assets.json");
}

void GameApp::setupSystems()
{
    RType::Game::GameApp::setupSystems();
    _pluginManager.loadModule(GAME_ASSET_MODULE_PATH,
        getRegistry(),
        getClientAssetManager().loaderRegistry());
    auto &registry = getRegistry();

    // Initialize network sync system with registry and resource references
    _networkSyncSystem.init(registry, getAssetManager());

    // Set up Lua scripting
    _scripts = std::make_unique<RType::ScriptEngine>(registry);
    // Physics2D components must be bound before game scripts use them
    RType::Physics2D::bindScripting(*_scripts);
    Game::bindGameComponents(*_scripts);
    _scripts->defineAction("MoveUp", 0);
    _scripts->defineAction("MoveDown", 1);
    _scripts->defineAction("MoveLeft", 2);
    _scripts->defineAction("MoveRight", 3);
    _scripts->defineAction("Shoot", 4);
    RType::addScriptSystem(registry, *_scripts);

    // Load game scripts
    auto scriptsPath =
        _configuration.path().parent_path() / "game_scripts.json";
    try {
        _scripts->loadConfigFile(scriptsPath);
    } catch (...) {
        std::cerr << "Failed to load game scripts from " << scriptsPath
                  << std::endl;
    }

    // Set up UI documents
    setupUi();
}

void GameApp::spawnInitialEntities()
{
    auto &registry = getRegistry();
    for (const float speed : {30.0f, 80.0f}) {
        const auto entity = registry.spawnEntity();
        registry.addComponent<RType::Transform2D>(
            entity, RType::Transform2D({0.0f, 0.0f}));
        registry.addComponent<ParallaxBackground>(
            entity, ParallaxBackground(speed));
    }
}

void GameApp::handleEvent(const sf::Event &event)
{
    if (getWindow().isOpen())
        ImGui::SFML::ProcessEvent(getWindow(), event);
    if (event.is<sf::Event::Closed>()) {
        RType::Game::GameApp::handleEvent(event);
    } else if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        handleKeyPressed(*keyPressed);
    } else if (const auto *keyReleased =
                   event.getIf<sf::Event::KeyReleased>()) {
        handleKeyReleased(*keyReleased);
    }
}

void GameApp::handleKeyPressed(const sf::Event::KeyPressed &key)
{
    if (key.code == sf::Keyboard::Key::Escape &&
        _screenState == ScreenState::Game)
        _inGameOverlayVisible = !_inGameOverlayVisible;
    else if (!ImGui::GetIO().WantCaptureKeyboard)
        getInputManager().setKeyState(static_cast<int>(key.code), true);
}

void GameApp::handleKeyReleased(const sf::Event::KeyReleased &key)
{
    if (!ImGui::GetIO().WantCaptureKeyboard)
        getInputManager().setKeyState(static_cast<int>(key.code), false);
}

void GameApp::updateGame(float dt)
{
    auto &window = getWindow();
    ImGui::SFML::Update(window, sf::seconds(dt));
    _uiRenderer.render(_uiManager, getAssetManager(), window);
    handleUiClicks();
    auto &registry = getRegistry();
    if (_screenState == ScreenState::Connection) {
        _networkClient.update(dt);
        updateConnectionStatus();
        if (_networkClient.isConnected()) {
            _screenState = ScreenState::Game;
            _uiManager.hide("connection");
            _uiManager.hide("ingame_overlay");
            _inGameOverlayVisible = false;
        }
        registry.runSystems();
        return;
    }
    if (!_networkClient.isConnected()) {
        _networkSyncSystem.reset();
        _screenState = ScreenState::Connection;
        _uiManager.show("connection");
        _uiManager.hide("ingame_overlay");
        _inGameOverlayVisible = false;
        return;
    }
    // Show/hide in-game overlay based on Escape key state
    if (_inGameOverlayVisible)
        _uiManager.show("ingame_overlay");
    else
        _uiManager.hide("ingame_overlay");
    updateInGameOverlay();
    auto &controllables = registry.getComponents<RType::Controllable>();
    auto &networkEntities = registry.getComponents<NetworkEntity>();
    const auto actions = getInputManager().actionMask();
    for (std::size_t i = 0; i < controllables.size(); ++i) {
        if (!controllables[i].has_value())
            continue;
        if (i < networkEntities.size() && networkEntities[i].has_value() &&
            networkEntities[i]->isLocalPlayer)
            controllables[i]->actionMask = actions;
        else
            controllables[i]->actionMask = 0;
    }
    registry.runSystems();
    if (_scripts)
        _scripts->setDeltaTime(dt);
    _networkSyncSystem.syncWithServer(
        getInputManager(), _networkClient, &getAudioManager());
}

void GameApp::renderGame()
{
    getWindow().clear(sf::Color(10, 10, 25));
    RType::RenderSystem renderSystem;
    renderSystem.render(getRegistry(), getAssetManager(), getWindow());
    // Render HUD elements
    renderHud();
    ImGui::SFML::Render(getWindow());
}

void GameApp::setupUi()
{
    auto uiPath = _configuration.path().parent_path() / "ui";
    _uiManager.load("connection", uiPath / "connection.json");
    _uiManager.load("ingame_overlay", uiPath / "ingame_overlay.json");
    _uiManager.show("connection");
    _uiManager.hide("ingame_overlay");

    // Connection screen: connect button
    _uiManager.onClick("connection/connect", [this]() {
        std::string host = _uiManager.getText("connection/host");
        std::string portStr = _uiManager.getText("connection/port");
        std::string playerName = _uiManager.getText("connection/playerName");
        int port = 4242;
        try {
            port = std::stoi(portStr);
        } catch (...) {
        }
        if (port < 1)
            port = 1;
        if (port > 65535)
            port = 65535;
        _networkClient.connect(
            host, static_cast<std::uint16_t>(port), playerName);
    });

    // In-game overlay: resume button
    _uiManager.onClick("ingame_overlay/resume", [this]() {
        _inGameOverlayVisible = false;
        _uiManager.hide("ingame_overlay");
    });

    // In-game overlay: disconnect button
    _uiManager.onClick("ingame_overlay/disconnect", [this]() {
        _networkClient.disconnect();
        _inGameOverlayVisible = false;
        _uiManager.hide("ingame_overlay");
    });
}

void GameApp::handleUiClicks()
{
    // Clicks are already processed by ImGuiUiRenderer via _uiManager.click()
    // This method is reserved for any additional click handling logic
}

void GameApp::updateConnectionStatus()
{
    auto status = _networkClient.getStatus();
    if (status == NetworkClient::ConnectionStatus::Connecting) {
        std::string host = _uiManager.getText("connection/host");
        std::string portStr = _uiManager.getText("connection/port");
        _uiManager.setText("connection/status",
            "Connecting to " + host + ":" + portStr + "...");
    } else if (status == NetworkClient::ConnectionStatus::ConnectionFailed) {
        std::string error = _networkClient.getLastError();
        _uiManager.setText("connection/status",
            "Connection Failed!" + (error.empty() ? "" : " (" + error + ")"));
    } else if (status ==
        NetworkClient::ConnectionStatus::DisconnectedByServer) {
        std::string reason = _networkClient.getLastError();
        _uiManager.setText("connection/status",
            "Disconnected from server." +
                (reason.empty() ? "" : " (" + reason + ")"));
    } else {
        _uiManager.setText("connection/status", "");
    }
}

void GameApp::updateInGameOverlay()
{
    _uiManager.setText("ingame_overlay/clientId",
        "Client ID: " + std::to_string(_networkClient.getClientId()));
    _uiManager.setText("ingame_overlay/linkedEntity",
        "Linked Entity: " + std::to_string(_networkClient.getLinkedEntity()));
    _uiManager.setText("ingame_overlay/serverTick",
        "Server Tick: " + std::to_string(_networkClient.getServerTick()) +
            " (Rate: " + std::to_string(_networkClient.getTickRate()) + " Hz)");
    _uiManager.setText("ingame_overlay/ping",
        "Ping: " + std::to_string(_networkClient.getPingMs()) + " ms");
}

void GameApp::renderHud()
{
    auto &registry = getRegistry();
    auto &players = registry.getComponents<Player>();
    for (auto [player] : RType::Zipper(players)) {
        // Charge Meter Background
        sf::RectangleShape chargeBg(sf::Vector2f(200.0f, 14.0f));
        chargeBg.setPosition(sf::Vector2f(20.0f, 1040.0f));
        chargeBg.setFillColor(sf::Color(40, 40, 60));
        chargeBg.setOutlineThickness(1.5f);
        chargeBg.setOutlineColor(sf::Color(150, 150, 200));
        getWindow().draw(chargeBg);

        // Charge Meter Fill
        if (player.chargeTime > 0.0f) {
            float chargeRatio = std::min(player.chargeTime / 1.5f, 1.0f);
            sf::RectangleShape chargeFill(
                sf::Vector2f(196.0f * chargeRatio, 10.0f));
            chargeFill.setPosition(sf::Vector2f(22.0f, 1042.0f));
            chargeFill.setFillColor(chargeRatio >= 1.0f
                    ? sf::Color(100, 240, 255)
                    : sf::Color(60, 160, 240));
            getWindow().draw(chargeFill);
        }

        // Lives representation
        for (int l = 0; l < player.lives; ++l) {
            sf::ConvexShape miniShip;
            miniShip.setPointCount(3);
            miniShip.setPoint(0, sf::Vector2f(12.0f, 0.0f));
            miniShip.setPoint(1, sf::Vector2f(-6.0f, -6.0f));
            miniShip.setPoint(2, sf::Vector2f(-6.0f, 6.0f));
            miniShip.setFillColor(sf::Color(50, 150, 255));
            miniShip.setPosition(
                sf::Vector2f(30.0f + static_cast<float>(l * 24), 30.0f));
            getWindow().draw(miniShip);
        }
        break; // HUD for main player
    }
}

void GameApp::onWindowCreated()
{
    if (!ImGui::SFML::Init(getWindow()))
        std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
    else if (ImGui::GetIO().Fonts->Fonts.empty())
        ImGui::GetIO().Fonts->AddFontDefault()->Scale = 2;
}

void GameApp::onShutdown()
{
    ImGui::SFML::Shutdown(getWindow());
}

GameApp::GameApp(const RType::Configuration &configuration) :
    RType::Game::GameApp(configuration)
{
}

ScreenState GameApp::getScreenState() const
{
    return _screenState;
}

void GameApp::setScreenState(ScreenState state)
{
    _screenState = state;
}

NetworkClient &GameApp::getNetworkClient()
{
    return _networkClient;
}

RType::UI::UiManager &GameApp::getUiManager()
{
    return _uiManager;
}

} // namespace Game

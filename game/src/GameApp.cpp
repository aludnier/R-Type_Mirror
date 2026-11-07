/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** R-Type game application
*/

#include "GameApp.h"

#include <SFML/Window/Keyboard.hpp>
#include <imgui.h>
#include <iostream>

#include "components/Controllable.h"
#include "components/Enemy.h"
#include "components/ForcePod.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Name.h"
#include "components/NetworkEntity.h"
#include "components/Player.h"
#include "components/Projectile.h"
#include "components/Rigidbody2D.h"
#include "components/Transform2D.h"
#include "ecs/Zipper.h"
#include "systems/BoundaryCleanupSystem.h"
#include "systems/CollisionSystem.h"
#include "systems/ForcePodSystem.h"
#include "systems/GameRenderSystem.h"
#include "systems/ParallaxSystem.h"
#include "systems/PlayerWeaponSystem.h"

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
    PlayerWeaponSystem::addSystem(registry, getInputManager(), _fixedDeltaTime);
    ForcePodSystem::addSystem(registry);
    CollisionSystem::addSystem(registry);
    BoundaryCleanupSystem::addSystem(registry, 1920.0f, 1080.0f);
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
        _connectionDialog.toggleVisible();
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
    const auto size = window.getSize();
    if (_screenState == ScreenState::Connection)
        _connectionDialog.renderScreen(_networkClient,
            static_cast<float>(size.x),
            static_cast<float>(size.y));
    else
        _connectionDialog.renderInGameOverlay(_networkClient,
            static_cast<float>(size.x),
            static_cast<float>(size.y));
    auto &registry = getRegistry();
    if (_screenState == ScreenState::Connection) {
        _networkClient.update(dt);
        if (_networkClient.isConnected()) {
            _screenState = ScreenState::Game;
            _connectionDialog.setVisible(false);
        }
        auto &transforms = registry.getComponents<RType::Transform2D>();
        auto &backgrounds = registry.getComponents<ParallaxBackground>();
        for (auto [transform, background] :
            RType::Zipper(transforms, backgrounds)) {
            transform.position.x -= background.scrollSpeed * dt;
            if (transform.position.x <= -1920.0f)
                transform.position.x += 1920.0f;
        }
        return;
    }
    if (!_networkClient.isConnected()) {
        _networkSyncSystem.reset(registry);
        _screenState = ScreenState::Connection;
        _connectionDialog.setVisible(true);
        return;
    }
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
    _networkSyncSystem.syncWithServer(
        registry, getInputManager(), getAssetManager(), _networkClient,
        &getAudioManager());
}

void GameApp::renderGame()
{
    GameRenderSystem::renderFrame(
        getRegistry(), getAssetManager(), getWindow());
    ImGui::SFML::Render(getWindow());
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

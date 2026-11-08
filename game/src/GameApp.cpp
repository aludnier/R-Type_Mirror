/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** R-Type game application
*/

#include "GameApp.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <iostream>

#include "Physics2DBindings.h"
#include "components/Animator.h"
#include "components/Controllable.h"
#include "components/Enemy.h"
#include "components/ForcePod.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Name.h"
#include "components/NetworkEntity.h"
#include "components/Player.h"
#include "components/Projectile.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/Zipper.h"
#include "library/NlohmanReader.h"
#include "network/GameProtocol.h"
#include "scripting/GameBindings.h"
#include "scripting/ScriptEngine.h"
#include "scripting/ScriptSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/RenderSystem.h"

namespace Game {
namespace {
    /** @brief Seconds a "LEVEL n" or "BOSS" banner stays on screen. */
    constexpr float BANNER_TIME = 2.5f;
    /** @brief Seconds of charge before the energy shows, and how far in
     *         front of the ship it is. */
    constexpr float CHARGE_EFFECT_DELAY = 0.2f;
    constexpr float CHARGE_EFFECT_OFFSET = 50.f;
    /** @brief Volume of the music loop, from 0 to 1. */
    constexpr float MUSIC_VOLUME = 0.45f;
    /** @brief Width and height of the ImGui font texture, in pixels. */
    constexpr int FONT_TEXTURE_SIZE = 1024;
} // namespace

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

void GameApp::setupKeyBindings()
{
    getInputManager().defineAction("MoveUp", 0);
    getInputManager().defineAction("MoveDown", 1);
    getInputManager().defineAction("MoveLeft", 2);
    getInputManager().defineAction("MoveRight", 3);
    getInputManager().defineAction("Shoot", 4);
    getInputManager().loadConfigFile(
        _configuration.path().parent_path() / "inputs.json");
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
    for (const auto &[name, action] : getInputManager().actions())
        _scripts->defineAction(name, action);
    RType::addScriptSystem(registry, *_scripts);

    // Load game scripts individually so one failure doesn't block the rest
    auto scriptsPath =
        _configuration.path().parent_path() / "game_scripts.json";
    try {
        RType::nlohamReader reader(scriptsPath);
        std::filesystem::path directory =
            reader.get<std::string>("directory", ".");
        if (directory.is_relative())
            directory = scriptsPath.parent_path() / directory;
        _scripts->setScriptDirectory(directory);
        for (const auto &script :
            reader.get<std::vector<std::string>>("scripts", {})) {
            if (!_scripts->loadFile(directory / script)) {
                std::cerr << "[GameApp] script warning: "
                          << _scripts->lastError() << std::endl;
            }
        }
    } catch (...) {
        std::cerr << "Failed to load game scripts from " << scriptsPath
                  << std::endl;
    }

    // Set up UI documents
    setupUi();
}

void GameApp::spawnInitialEntities()
{
    struct Layer {
        const char *asset;
        float speed;
        int zIndex;
        float top;
    };

    // Far layers scroll slower. Each layer is two 1920 px tiles side by
    // side, so it loops without a gap.
    // The ground is in front of the stars, behind the ships.
    constexpr Layer LAYERS[] = {
        {"bg_nebula", 15.f, -30, 0.f},
        {"bg_stars_far", 40.f, -20, 0.f},
        {"bg_stars_near", 110.f, -10, 0.f},
        {"bg_ground", Protocol::GROUND_SCROLL_SPEED, -5, Protocol::GROUND_TOP},
    };
    constexpr float TILE_WIDTH = 1920.f;
    auto &registry = getRegistry();

    for (const auto &[asset, speed, zIndex, top] : LAYERS) {
        if (!getAssetManager().hasAsset(asset))
            continue;
        const auto assetId = getAssetManager().getAsset(asset).id;
        for (const float x : {0.f, TILE_WIDTH}) {
            const auto entity = registry.spawnEntity();
            RType::Sprite sprite(assetId, zIndex);

            sprite.originX = 0.f;
            sprite.originY = 0.f;
            registry.addComponent<RType::Transform2D>(
                entity, RType::Transform2D({x, top}));
            registry.addComponent(entity, RType::Sprite {sprite});
            registry.addComponent<ParallaxBackground>(
                entity, ParallaxBackground(speed, -TILE_WIDTH, TILE_WIDTH));
        }
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
    if (key.code == sf::Keyboard::Key::Enter && _networkSyncSystem.isGameOver())
        playAgain();
    else if (key.code == sf::Keyboard::Key::Escape &&
        _screenState == ScreenState::Game && !_networkSyncSystem.isGameOver())
        _inGameOverlayVisible = !_inGameOverlayVisible;
    else if (_screenState == ScreenState::Game ||
        !ImGui::GetIO().WantCaptureKeyboard)
        getInputManager().setKeyState(static_cast<int>(key.code), true);
}

void GameApp::handleKeyReleased(const sf::Event::KeyReleased &key)
{
    if (_screenState == ScreenState::Game ||
        !ImGui::GetIO().WantCaptureKeyboard)
        getInputManager().setKeyState(static_cast<int>(key.code), false);
}

void GameApp::updateGame(float dt)
{
    auto &window = getWindow();
    ImGui::SFML::Update(window, sf::seconds(dt));
    warmUpFont();
    _uiRenderer.render(_uiManager, getAssetManager(), window);
    handleUiClicks();
    auto &registry = getRegistry();
    if (_screenState == ScreenState::Connection) {
        // After Play again: join the same server as a new player.
        if (_reconnect) {
            _reconnect = false;
            _networkClient.connect(_lastHost, _lastPort, _lastPlayerName);
        }
        _networkClient.update(dt);
        updateConnectionStatus();
        if (_networkClient.isConnected()) {
            _screenState = ScreenState::Game;
            if (!getAudioManager().isMusicPlaying())
                getAudioManager().playMusic("bgm_stage1", true, MUSIC_VOLUME);
            _uiManager.hide("connection");
            _uiManager.hide("ingame_overlay");
            _inGameOverlayVisible = false;
        }
        registry.runSystems();
        return;
    }
    if (!_networkClient.isConnected()) {
        _networkSyncSystem.reset();
        _shipTilt.reset();
        if (_chargeEffect)
            getRegistry().killEntity(*_chargeEffect);
        _chargeEffect.reset();
        _screenState = ScreenState::Connection;
        _uiManager.show("connection");
        _uiManager.hide("ingame_overlay");
        _uiManager.hide("game_over");
        _uiManager.hide("banner");
        _inGameOverlayVisible = false;
        return;
    }
    // Show/hide in-game overlay based on Escape key state. The game over
    // screen replaces it.
    if (_networkSyncSystem.isGameOver())
        _inGameOverlayVisible = false;
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
    // Like the server: the predicted ship stays in the play area.
    auto &transforms = registry.getComponents<RType::Transform2D>();
    for (auto [transform, network] : RType::Zipper(transforms, networkEntities))
        if (network.isLocalPlayer)
            transform.position = Protocol::clampToPlayArea(transform.position);
    if (_scripts)
        _scripts->setDeltaTime(dt);
    _networkSyncSystem.syncWithServer(
        getInputManager(), _networkClient, &getAudioManager());
    _shipTilt.update(registry, getInputManager(), dt);
    updateChargeEffect();
    if (auto banner = _networkSyncSystem.takeBanner()) {
        _bannerText = std::move(*banner);
        _bannerTime = BANNER_TIME;
    }
    _bannerTime = std::max(_bannerTime - dt, 0.f);
    updateEndOfGameUi();
}

void GameApp::renderGame()
{
    getWindow().clear(sf::Color(10, 10, 25));
    _renderSystem.render(getRegistry(), getAssetManager(), getWindow());
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
        _lastHost = host;
        _lastPort = static_cast<std::uint16_t>(port);
        _lastPlayerName = playerName;
        _networkClient.connect(host, _lastPort, playerName);
    });

    // Game over screen: play again on the same server, or quit.
    _uiManager.load("game_over", uiPath / "game_over.json");
    _uiManager.load("banner", uiPath / "banner.json");
    _uiManager.hide("game_over");
    _uiManager.hide("banner");
    _uiManager.onClick("game_over/play_again", [this]() { playAgain(); });
    _uiManager.onClick("game_over/quit", [this]() { close(); });

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

void GameApp::warmUpFont()
{
    if (_fontWarmedUp)
        return;
    _fontWarmedUp = true;
    // Every printable character, at the size the UI uses, on the first
    // frame: no glyph is added to the font texture later.
    ImFontBaked *baked = ImGui::GetFontBaked();
    for (ImWchar c = 0x20; c < 0x7F; ++c)
        baked->FindGlyph(c);
}

void GameApp::updateChargeEffect()
{
    auto &registry = getRegistry();
    auto &players = registry.getComponents<Player>();
    auto &transforms = registry.getComponents<RType::Transform2D>();
    auto &networkEntities = registry.getComponents<NetworkEntity>();
    std::optional<RType::Vector2> charging;

    for (auto [player, transform, network] :
        RType::Zipper(players, transforms, networkEntities))
        if (network.isLocalPlayer && player.chargeTime > CHARGE_EFFECT_DELAY)
            charging = transform.position;
    if (!charging) {
        if (_chargeEffect)
            registry.killEntity(*_chargeEffect);
        _chargeEffect.reset();
        return;
    }
    if (!_chargeEffect && getAssetManager().hasAsset("laser_bullet")) {
        // The energy gathering in front of the ship, as in R-Type.
        static const std::vector<RType::Animation::AnimationFrame> FRAMES = {
            {134, 18, 32, 32},
            {167, 20, 32, 29},
            {200, 21, 32, 25},
            {300, 21, 32, 25},
            {333, 20, 32, 29},
            {366, 18, 32, 32}};
        const auto entity = registry.spawnEntity();
        const auto &first = FRAMES.front();

        registry.addComponent(entity, RType::Transform2D(*charging));
        registry.addComponent(entity,
            RType::Sprite(getAssetManager().getAsset("laser_bullet").id,
                first.left,
                first.top,
                first.width,
                first.height,
                12));
        auto &animator = registry.addComponent<RType::Animation::Animator>(
            entity, RType::Animation::Animator {});
        if (animator) {
            RType::Animation::AnimationSystem::addClip(*animator,
                "charge",
                RType::Animation::AnimationSystem::createClip(FRAMES, 0.06f));
            RType::Animation::AnimationSystem::play(*animator, "charge", true);
        }
        _chargeEffect = entity;
    }
    if (_chargeEffect && *_chargeEffect < transforms.size() &&
        transforms[*_chargeEffect])
        transforms[*_chargeEffect]->position = {
            charging->x + CHARGE_EFFECT_OFFSET, charging->y};
}

void GameApp::playAgain()
{
    // The server gives a new ship with full lives to a new connection.
    _networkClient.disconnect();
    _uiManager.hide("game_over");
    _reconnect = true;
}

void GameApp::updateEndOfGameUi()
{
    const bool gameOver = _networkSyncSystem.isGameOver();

    if (gameOver)
        _uiManager.show("game_over");
    else
        _uiManager.hide("game_over");
    if (_bannerTime > 0.f && !gameOver) {
        _uiManager.setText("banner/text", _bannerText);
        _uiManager.show("banner");
    } else {
        _uiManager.hide("banner");
    }
}

void GameApp::renderHud()
{
    auto &registry = getRegistry();
    auto &players = registry.getComponents<Player>();
    auto &networkEntities = registry.getComponents<NetworkEntity>();
    for (auto [player, network] : RType::Zipper(players, networkEntities)) {
        if (!network.isLocalPlayer)
            continue;
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
        break; // HUD of the local player only
    }
}

void GameApp::onWindowCreated()
{
    // ImGui bakes the glyphs when they are first drawn. ImGui-SFML garbles
    // the glyphs added once the font texture has to grow, so start with a
    // texture big enough for every glyph (see warmUpFont()).
    if (!ImGui::SFML::Init(getWindow(), false)) {
        std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
        return;
    }
    ImGui::GetIO().Fonts->TexMinWidth = FONT_TEXTURE_SIZE;
    ImGui::GetIO().Fonts->TexMinHeight = FONT_TEXTURE_SIZE;
    ImGui::GetIO().Fonts->AddFontDefault();
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

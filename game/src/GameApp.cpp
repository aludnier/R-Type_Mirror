/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** GameApp implementation
*/

#include "GameApp.h"
#include "ecs/SparseArray.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <SFML/Window/Keyboard.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

// Engine headers
#include "components/Action.h"
#include "components/Controllable.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "systems/ControllableSystem.h"

// Game component headers
#include "components/Player.h"
#include "components/Enemy.h"
#include "components/Rigidbody2D.h"
#include "components/PhysicsSettings2D.h"
#include "components/Projectile.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/ForcePod.h"
#include "components/NetworkEntity.h"

// Game system headers
#include "systems/MovementSystem.h"
#include "systems/EnemyAISystem.h"
#include "systems/ForcePodSystem.h"
#include "systems/PlayerWeaponSystem.h"
#include "systems/CollisionSystem.h"
#include "systems/BoundaryCleanupSystem.h"
#include "systems/ParallaxSystem.h"
#include "systems/GameRenderSystem.h"
#include "systems/AudioSystem.h"
#include "systems/NetworkSyncSystem.h"

namespace Game {

    GameApp::GameApp()
        : _running(false), _frameRate(60.0f), _fixedDeltaTime(1.0f / 60.0f)
    {
#ifdef WIN32
        _physics2DModuleLoader.loadFile("../modules/physics2D/Physics2D_Module.dll");
        _environmentModuleLoader.loadFile("../modules/environment2D/Environment2D_Module.dll");
#else
        _physics2DModuleLoader.loadFile("../modules/physics2D/Physics2D_Module.so");
        _environmentModuleLoader.loadFile("../modules/environment2D/Environment2D_Module.so");
#endif
    }

    void GameApp::init()
    {
        _registry.registerComponent<RType::Transform2D>();
        _registry.registerComponent<RType::Sprite>();
        _registry.registerComponent<RType::Controllable>();
        _registry.registerComponent<NetworkEntity>();
        _registry.registerComponent<Player>();
        _registry.registerComponent<Enemy>();
        _registry.registerComponent<Projectile>();
        _registry.registerComponent<Health>();
        _registry.registerComponent<Hitbox>();
        _registry.registerComponent<ParallaxBackground>();
        _registry.registerComponent<ForcePod>();

        setupResources();
        setupKeyBindings();
        setupSystems();
        spawnInitialEntities();
    }

    void GameApp::setupResources()
    {
        _resourceManager.addAsset("player_ship", "assets/sprites/r_type_player.png", ResourceManager::AssetType::Texture);
        _resourceManager.addAsset("force_pod", "assets/sprites/force_pod.png", ResourceManager::AssetType::Texture);
        _resourceManager.addAsset("enemy_bydo_scout", "assets/sprites/bydo_scout.png", ResourceManager::AssetType::Texture);
        _resourceManager.addAsset("laser_bullet", "assets/sprites/laser_bullet.png", ResourceManager::AssetType::Texture);
        _resourceManager.addAsset("wave_cannon", "assets/sprites/wave_cannon.png", ResourceManager::AssetType::Texture);
        _resourceManager.addAsset("bg_space_stars", "assets/backgrounds/space_stars.png", ResourceManager::AssetType::Texture);
        _resourceManager.addAsset("bg_nebula", "assets/backgrounds/nebula.png", ResourceManager::AssetType::Texture);
        _resourceManager.addAsset("sfx_laser", "assets/audio/laser.wav", ResourceManager::AssetType::Sound);
        _resourceManager.addAsset("sfx_explosion", "assets/audio/explosion.wav", ResourceManager::AssetType::Sound);
        _resourceManager.addAsset("bgm_stage1", "assets/audio/stage1.ogg", ResourceManager::AssetType::Music);
    }

    void GameApp::setupKeyBindings()
    {
        _inputManager.bind(Action::MoveUp, static_cast<int>(sf::Keyboard::Key::Up));
        _inputManager.bind(Action::MoveUp, static_cast<int>(sf::Keyboard::Key::W));
        _inputManager.bind(Action::MoveDown, static_cast<int>(sf::Keyboard::Key::Down));
        _inputManager.bind(Action::MoveDown, static_cast<int>(sf::Keyboard::Key::S));
        _inputManager.bind(Action::MoveLeft, static_cast<int>(sf::Keyboard::Key::Left));
        _inputManager.bind(Action::MoveLeft, static_cast<int>(sf::Keyboard::Key::A));
        _inputManager.bind(Action::MoveRight, static_cast<int>(sf::Keyboard::Key::Right));
        _inputManager.bind(Action::MoveRight, static_cast<int>(sf::Keyboard::Key::D));
        _inputManager.bind(Action::Shoot, static_cast<int>(sf::Keyboard::Key::Space));
    }

    void GameApp::setupSystems()
    {
        constexpr float playerSpeed = 350.0f;

        // Engine Controllable system (local client prediction)
        RType::addControllableSystem(_registry, playerSpeed, _fixedDeltaTime);

        // Modules systems
        _physics2DModuleLoader.getInstance().registerModule(_registry);
        _environmentModuleLoader.getInstance().registerModule(_registry);

        // Game systems
        PlayerWeaponSystem::addSystem(_registry, _inputManager, _fixedDeltaTime);
        EnemyAISystem::addSystem(_registry, _fixedDeltaTime);
        ForcePodSystem::addSystem(_registry);
        CollisionSystem::addSystem(_registry);
        BoundaryCleanupSystem::addSystem(_registry, 1920.0f, 1080.0f);
    }

    void GameApp::spawnInitialEntities()
    {
        // Parallax backgrounds
        auto bgStars = _registry.spawnEntity();
        _registry.addComponent<RType::Transform2D>(bgStars, RType::Transform2D({0.0f, 0.0f}));
        _registry.addComponent<ParallaxBackground>(bgStars, ParallaxBackground(30.0f));

        auto bgNebula = _registry.spawnEntity();
        _registry.addComponent<RType::Transform2D>(bgNebula, RType::Transform2D({0.0f, 0.0f}));
        _registry.addComponent<ParallaxBackground>(bgNebula, ParallaxBackground(80.0f));
    }

    void GameApp::processEvents()
    {
        if (!_window) {
            return;
        }

        while (const auto event = _window->pollEvent()) {
            ImGui::SFML::ProcessEvent(*_window, *event);
            handleEvent(*event);
        }
    }

    void GameApp::handleEvent(const sf::Event &event)
    {
        if (event.is<sf::Event::Closed>()) {
            _running = false;
            _window->close();
        } else if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            handleKeyPressed(*keyPressed);
        } else if (const auto *keyReleased = event.getIf<sf::Event::KeyReleased>()) {
            handleKeyReleased(*keyReleased);
        }
    }

    void GameApp::handleKeyPressed(const sf::Event::KeyPressed &keyPressed)
    {
        if (keyPressed.code == sf::Keyboard::Key::Escape) {
            if (_screenState == ScreenState::Game) {
                _connectionDialog.toggleVisible();
            }
        } else if (!ImGui::GetIO().WantCaptureKeyboard) {
            _inputManager.setKeyState(static_cast<int>(keyPressed.code), true);
        }
    }

    void GameApp::handleKeyReleased(const sf::Event::KeyReleased &keyReleased)
    {
        if (!ImGui::GetIO().WantCaptureKeyboard) {
            _inputManager.setKeyState(static_cast<int>(keyReleased.code), false);
        }
    }

    void GameApp::update(float dt)
    {
        if (_window && _window->isOpen()) {
            ImGui::SFML::Update(*_window, sf::seconds(dt));
            auto winSize = _window->getSize();
            if (_screenState == ScreenState::Connection) {
                _connectionDialog.renderScreen(_networkClient, static_cast<float>(winSize.x), static_cast<float>(winSize.y));
            } else {
                _connectionDialog.renderInGameOverlay(_networkClient, static_cast<float>(winSize.x), static_cast<float>(winSize.y));
            }
        }

        if (_screenState == ScreenState::Connection) {
            _networkClient.update(dt);
            if (_networkClient.isConnected()) {
                _screenState = ScreenState::Game;
                _connectionDialog.setVisible(false);
            }

            // Animate background starfield while on connection screen
            auto &transforms = _registry.getComponents<RType::Transform2D>();
            auto &parallax = _registry.getComponents<ParallaxBackground>();
            for (auto [transform, bg] : RType::Zipper(transforms, parallax)) {
                transform.position.x -= bg.scrollSpeed * dt;
                if (transform.position.x <= -1920.0f) {
                    transform.position.x += 1920.0f;
                }
            }
            return;
        }

        // Active In-Game Screen
        if (!_networkClient.isConnected()) {
            // Disconnected -> reset ECS network replica and return to connection lobby
            _networkSyncSystem.reset(_registry);
            _screenState = ScreenState::Connection;
            _connectionDialog.setVisible(true);
            return;
        }

        // Update action mask ONLY on the local controllable player entity
        auto &controllables = _registry.getComponents<RType::Controllable>();
        auto &netEntities = _registry.getComponents<NetworkEntity>();
        std::uint8_t currentMask = _inputManager.actionMask();

        for (std::size_t i = 0; i < controllables.size(); ++i) {
            if (controllables[i].has_value()) {
                if (i < netEntities.size() && netEntities[i].has_value() && netEntities[i]->isLocalPlayer) {
                    controllables[i]->actionMask = currentMask;
                } else {
                    controllables[i]->actionMask = 0;
                }
            }
        }

        _registry.runSystems();
        _networkSyncSystem.syncWithServer(_registry, _inputManager, _resourceManager, _networkClient);
    }

    void GameApp::render()
    {
        if (_window && _window->isOpen()) {
            GameRenderSystem::renderFrame(_registry, _resourceManager, *_window);
            ImGui::SFML::Render(*_window);
            _window->display();
        }
    }

    int GameApp::run()
    {
        std::cout << "=== R-Type 2D Shooter Client ===" << std::endl;
        init();

        _window = std::make_unique<sf::RenderWindow>(
            sf::VideoMode({1920, 1080}),
            "R-Type Client",
            sf::Style::Titlebar | sf::Style::Close
        );
        _window->setFramerateLimit(static_cast<unsigned int>(_frameRate));

        if (!ImGui::SFML::Init(*_window)) {
            std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
        } else {
            ImGuiIO &io = ImGui::GetIO();
            if (io.Fonts->Fonts.empty()) {
                auto *font = io.Fonts->AddFontDefault();
                font->Scale = 2;
            }
        }

        std::cout << "Client initialized successfully. Use ImGui dialog to connect to server." << std::endl;

        sf::Clock clock;
        _running = true;
        while (_running && _window->isOpen()) {
            float dt = clock.restart().asSeconds();
            processEvents();
            update(dt);
            render();
        }

        ImGui::SFML::Shutdown(*_window);
        return 0;
    }

    ScreenState GameApp::getScreenState() const
    {
        return _screenState;
    }

    void GameApp::setScreenState(ScreenState state)
    {
        _screenState = state;
    }

    RType::Registry &GameApp::getRegistry()
    {
        return _registry;
    }

    InputManager &GameApp::getInputManager()
    {
        return _inputManager;
    }

    ResourceManager &GameApp::getResourceManager()
    {
        return _resourceManager;
    }

    NetworkClient &GameApp::getNetworkClient()
    {
        return _networkClient;
    }

    sf::RenderWindow &GameApp::getWindow()
    {
        return *_window;
    }

} // namespace Game

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Reusable game application base
*/

#include "game/GameApp.h"

#include <SFML/Window/Keyboard.hpp>
#include <chrono>
#include <iostream>

#include "components/Controllable.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "systems/ControllableSystem.h"

namespace Game {

GameApp::GameApp()
{
#ifdef WIN32
    _physics2DModuleLoader.loadFile(ENGINE_MODULE_DIR "/Physics2D_Module.dll");
    _environmentModuleLoader.loadFile(
        ENGINE_MODULE_DIR "/Environment2D_Module.dll");
#else
    _physics2DModuleLoader.loadFile(ENGINE_MODULE_DIR "/Physics2D_Module.so");
    _environmentModuleLoader.loadFile(
        ENGINE_MODULE_DIR "/Environment2D_Module.so");
#endif
}

void GameApp::init()
{
    _registry.registerComponent<RType::Transform2D>();
    _registry.registerComponent<RType::Sprite>();
    _registry.registerComponent<RType::Controllable>();
    registerComponents();
    setupResources();
    setupKeyBindings();
    setupSystems();
    spawnInitialEntities();
}

void GameApp::setupSystems()
{
    _physics2DModuleLoader.getInstance().registerModule(_registry);
    _environmentModuleLoader.getInstance().registerModule(_registry);
    RType::addControllableSystem(_registry, 300.0f, _fixedDeltaTime,
        {0, 1, 2, 3});
}

void GameApp::setupKeyBindings()
{
    constexpr RType::ActionId moveUp = 0;
    constexpr RType::ActionId moveDown = 1;
    constexpr RType::ActionId moveLeft = 2;
    constexpr RType::ActionId moveRight = 3;
    constexpr RType::ActionId shoot = 4;
    _inputManager.defineAction("MoveUp", moveUp);
    _inputManager.defineAction("MoveDown", moveDown);
    _inputManager.defineAction("MoveLeft", moveLeft);
    _inputManager.defineAction("MoveRight", moveRight);
    _inputManager.defineAction("Shoot", shoot);
    _inputManager.bind(moveUp, static_cast<int>(sf::Keyboard::Key::Up));
    _inputManager.bind(moveUp, static_cast<int>(sf::Keyboard::Key::W));
    _inputManager.bind(moveDown, static_cast<int>(sf::Keyboard::Key::Down));
    _inputManager.bind(moveDown, static_cast<int>(sf::Keyboard::Key::S));
    _inputManager.bind(moveLeft, static_cast<int>(sf::Keyboard::Key::Left));
    _inputManager.bind(moveLeft, static_cast<int>(sf::Keyboard::Key::A));
    _inputManager.bind(moveRight, static_cast<int>(sf::Keyboard::Key::Right));
    _inputManager.bind(moveRight, static_cast<int>(sf::Keyboard::Key::D));
    _inputManager.bind(shoot, static_cast<int>(sf::Keyboard::Key::Space));
}

void GameApp::processEvents()
{
    if (!_window)
        return;
    while (const auto event = _window->pollEvent())
        handleEvent(*event);
}

void GameApp::handleEvent(const sf::Event &event)
{
    if (event.is<sf::Event::Closed>()) {
        close();
    } else if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
        _inputManager.setKeyState(static_cast<int>(key->code), true);
    } else if (const auto *key = event.getIf<sf::Event::KeyReleased>()) {
        _inputManager.setKeyState(static_cast<int>(key->code), false);
    }
}

void GameApp::close()
{
    _running = false;
    if (_window)
        _window->close();
}

void GameApp::update(float dt)
{
    updateGame(dt);
}

void GameApp::render()
{
    if (_window && _window->isOpen()) {
        renderGame();
        _window->display();
    }
}

int GameApp::run()
{
    init();
    _window = std::make_unique<sf::RenderWindow>(sf::VideoMode({1920, 1080}),
        "Game", sf::Style::Titlebar | sf::Style::Close);
    _window->setFramerateLimit(static_cast<unsigned int>(_frameRate));
    onWindowCreated();
    sf::Clock clock;
    _running = true;
    while (_running && _window->isOpen()) {
        const float dt = clock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
    onShutdown();
    return 0;
}

RType::Registry &GameApp::getRegistry() { return _registry; }
InputManager &GameApp::getInputManager() { return _inputManager; }
ResourceManager &GameApp::getResourceManager() { return _resourceManager; }
sf::RenderWindow &GameApp::getWindow() const { return *_window; }

} // namespace Game

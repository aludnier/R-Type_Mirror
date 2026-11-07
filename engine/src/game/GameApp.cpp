/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Reusable game application base
*/

#include "game/GameApp.h"

#include <SFML/Window/Keyboard.hpp>
#include "components/Controllable.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/SparseArray.h"
#include "systems/ControllableSystem.h"

namespace RType::Game {

GameApp::GameApp(const Configuration &configuration) :
    _configuration(configuration)
{
}

void GameApp::init()
{
    _registry.registerComponent<Transform2D>();
    _registry.registerComponent<Sprite>();
    _registry.registerComponent<Controllable>();
    registerComponents();
    setupKeyBindings();
    setupSystems();
    setupResources();
    spawnInitialEntities();
}

void GameApp::setupSystems()
{
    _pluginManager.loadConfig(
        _configuration, _registry, _clientAssetManager.loaderRegistry());
    addControllableSystem(
        _registry, _configuration.game().playerSpeed, _fixedDeltaTime, {});
}

void GameApp::setupKeyBindings()
{
    constexpr ActionId moveUp = 0;
    constexpr ActionId moveDown = 1;
    constexpr ActionId moveLeft = 2;
    constexpr ActionId moveRight = 3;
    constexpr ActionId shoot = 4;
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
    } else if (const auto *keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        _inputManager.setKeyState(static_cast<int>(keyPressed->code), true);
    } else if (const auto *keyReleased =
                   event.getIf<sf::Event::KeyReleased>()) {
        _inputManager.setKeyState(static_cast<int>(keyReleased->code), false);
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
        _configuration.game().name,
        sf::Style::Titlebar | sf::Style::Close);
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

Registry &GameApp::getRegistry()
{
    return _registry;
}

InputManager &GameApp::getInputManager()
{
    return _inputManager;
}

AssetManager &GameApp::getAssetManager()
{
    return _clientAssetManager.metadata();
}

ClientAssetManager &GameApp::getClientAssetManager()
{
    return _clientAssetManager;
}

sf::RenderWindow &GameApp::getWindow() const
{
    return *_window;
}

} // namespace RType::Game

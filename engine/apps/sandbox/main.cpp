/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** main.cpp
*/

#include <algorithm>
#include <exception>
#include <iostream>

#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "library/Keys.h"
#include "library/RessourceManager.h"
#include "platform/Application.h"
#include "render/SceneRenderer.h"

namespace {
constexpr RType::ActionId Brighter = 0;
constexpr RType::ActionId Darker = 1;
constexpr RType::ActionId Quit = 2;

class Sandbox : public RType::Application {
public:
    Sandbox() :
        Application({.title = "Sandbox"})
    {
        input().bind(Brighter, RType::Key::Up);
        input().bind(Darker, RType::Key::Down);
        input().bind(Quit, RType::Key::Escape);

        const auto image = _assets.addAsset("test", SANDBOX_ASSETS "/test.png");

        _registry.registerComponent<RType::Transform2D>();
        _registry.registerComponent<RType::Sprite>();
        _spinning = _registry.spawnEntity();
        _registry.addComponent(
            _spinning, RType::Transform2D {{0.f, 0.f}, {3.f, 3.f}});
        _registry.addComponent(_spinning, RType::Sprite {image, 1});

        const auto behind = _registry.spawnEntity();

        _registry.addComponent(
            behind, RType::Transform2D {{-120.f, 60.f}, {4.f, 4.f}});
        _registry.addComponent(behind, RType::Sprite {image, 0});
    }

protected:
    void update(float dt) override
    {
        if (input().isActionDown(Quit))
            window().close();
        if (input().isActionDown(Brighter))
            _brightness = std::min(_brightness + dt, 1.f);
        if (input().isActionDown(Darker))
            _brightness = std::max(_brightness - dt, 0.f);
        _registry.getComponents<RType::Transform2D>()[_spinning]->rotation +=
            dt;
    }

    void render() override
    {
        window().clear({0.1f, 0.2f * _brightness, _brightness, 1.f});
        _sceneRenderer.render(_registry, window(), _camera);
    }

private:
    float _brightness = 0.5f;
    RType::Registry _registry;
    ResourceManager _assets;
    RType::SceneRenderer _sceneRenderer {_assets};
    RType::Camera2D _camera;
    RType::Entity _spinning = RType::Registry::entityFromIndex(0);
};
} // namespace

int main()
{
    try {
        Sandbox sandbox;

        sandbox.run();
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}

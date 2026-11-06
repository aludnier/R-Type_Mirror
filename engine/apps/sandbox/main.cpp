/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** main.cpp
*/

#include <algorithm>
#include <exception>
#include <iostream>

#include "library/Keys.h"
#include "platform/Application.h"
#include "render/Renderer2D.h"
#include "render/Texture.h"

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
    }

    void render() override
    {
        window().clear({0.1f, 0.2f * _brightness, _brightness, 1.f});
        _renderer.drawTexture(_texture, {-0.5f, 0.5f, 1.f, 1.f});
    }

private:
    float _brightness = 0.5f;
    RType::Renderer2D _renderer;
    RType::Texture _texture {SANDBOX_ASSETS "/test.png"};
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

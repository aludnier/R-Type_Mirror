/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** main.cpp
*/

#include <algorithm>
#include <exception>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "platform/Application.h"

namespace {
constexpr RType::ActionId Brighter = 0;
constexpr RType::ActionId Darker = 1;
constexpr RType::ActionId Quit = 2;

class Sandbox : public RType::Application {
public:
    Sandbox() : Application({.title = "Sandbox"})
    {
        input().bind(Brighter, GLFW_KEY_UP);
        input().bind(Darker, GLFW_KEY_DOWN);
        input().bind(Quit, GLFW_KEY_ESCAPE);
        std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";
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
        glClearColor(0.1f, 0.2f * _brightness, _brightness, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

private:
    float _brightness = 0.5f;
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

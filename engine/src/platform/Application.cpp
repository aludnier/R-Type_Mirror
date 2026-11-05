/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Application.cpp
*/

#include "Application.h"

namespace RType {
Application::Application(const WindowConfig &config) : _window(config)
{
    _window.setKeyCallback([this](int key, bool pressed) {
        _input.setKeyState(key, pressed);
    });
}

void Application::run()
{
    double previous = _window.time();

    while (_window.isOpen()) {
        const double now = _window.time();
        const int steps = _fixedStep.advance(now - previous);

        previous = now;
        _window.pollEvents();
        for (int i = 0; i < steps; ++i)
            update(static_cast<float>(_fixedStep.step()));
        render();
        _window.swapBuffers();
    }
}
} // namespace RType

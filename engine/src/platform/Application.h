/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Application.h
*/

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include "library/InputManager.h"
#include "platform/Window.h"
#include "time/FixedStep.h"

namespace RType {
/**
 * @brief Main loop of a client program: inherit from it, then override
 *        update() for the game logic and render() for the drawing.
 */
class Application {
public:
    explicit Application(const WindowConfig &config);
    virtual ~Application() = default;

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    /** @brief Runs until the window is closed. */
    void run();

protected:
    /** @brief Called at a fixed rate. @param dt Seconds, always the same. */
    virtual void update(float dt) = 0;
    /** @brief Called once per frame. */
    virtual void render() = 0;

    Window &window()
    {
        return _window;
    }

    InputManager &input()
    {
        return _input;
    }

private:
    Window _window;
    InputManager _input;
    FixedStep _fixedStep {1.0 / 60.0};
};
} // namespace RType

#endif /* !APPLICATION_H_ */

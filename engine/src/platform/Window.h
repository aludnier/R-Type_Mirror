/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Window.h
*/

#ifndef WINDOW_H_
#define WINDOW_H_

#include <functional>
#include <string>

struct GLFWwindow;

namespace RType {
struct Color {
    float r = 0.f;
    float g = 0.f;
    float b = 0.f;
    float a = 1.f;
};

struct WindowConfig {
    std::string title = "Engine";
    int width = 1280;
    int height = 720;
    bool vsync = true;
};

/** @brief An operating system window with an OpenGL 3.3 core context. */
class Window {
public:
    using KeyCallback = std::function<void(int key, bool pressed)>;

    /** @throws std::runtime_error if the window or context cannot be made. */
    explicit Window(const WindowConfig &config);
    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    [[nodiscard]] bool isOpen() const;
    void close();

    /** @brief Fills the window with a color. */
    void clear(const Color &color);

    /** @brief Handles the pending window and keyboard events. */
    static void pollEvents();
    /** @brief Shows what was drawn during this frame. */
    void swapBuffers();

    /** @brief Sets the function called when a key is pressed or released. */
    void setKeyCallback(KeyCallback callback);

    /** @return Seconds since the window was created. */
    [[nodiscard]] static double time();

private:
    static void onKey(
        GLFWwindow *handle, int key, int scancode, int action, int mods);
    static void onResize(GLFWwindow *handle, int width, int height);

    GLFWwindow *_handle = nullptr;
    KeyCallback _keyCallback;
};
} // namespace RType

#endif /* !WINDOW_H_ */

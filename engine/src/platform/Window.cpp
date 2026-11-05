/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Window.cpp
*/

#include "Window.h"
#include "library/Keys.h"

#include <stdexcept>
#include <utility>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace RType {
static_assert(Key::Space == GLFW_KEY_SPACE);
static_assert(Key::Num0 == GLFW_KEY_0);
static_assert(Key::Num9 == GLFW_KEY_9);
static_assert(Key::A == GLFW_KEY_A);
static_assert(Key::D == GLFW_KEY_D);
static_assert(Key::S == GLFW_KEY_S);
static_assert(Key::W == GLFW_KEY_W);
static_assert(Key::Z == GLFW_KEY_Z);
static_assert(Key::Escape == GLFW_KEY_ESCAPE);
static_assert(Key::Enter == GLFW_KEY_ENTER);
static_assert(Key::Tab == GLFW_KEY_TAB);
static_assert(Key::Backspace == GLFW_KEY_BACKSPACE);
static_assert(Key::Delete == GLFW_KEY_DELETE);
static_assert(Key::Right == GLFW_KEY_RIGHT);
static_assert(Key::Left == GLFW_KEY_LEFT);
static_assert(Key::Down == GLFW_KEY_DOWN);
static_assert(Key::Up == GLFW_KEY_UP);
static_assert(Key::F1 == GLFW_KEY_F1);
static_assert(Key::F12 == GLFW_KEY_F12);
static_assert(Key::LeftShift == GLFW_KEY_LEFT_SHIFT);
static_assert(Key::LeftControl == GLFW_KEY_LEFT_CONTROL);
static_assert(Key::LeftAlt == GLFW_KEY_LEFT_ALT);

Window::Window(const WindowConfig &config)
{
    if (glfwInit() != GLFW_TRUE)
        throw std::runtime_error("cannot initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    _handle = glfwCreateWindow(
        config.width, config.height, config.title.c_str(), nullptr, nullptr);
    if (_handle == nullptr) {
        glfwTerminate();
        throw std::runtime_error("cannot create the window");
    }

    glfwMakeContextCurrent(_handle);
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) ==
        0) {
        glfwDestroyWindow(_handle);
        glfwTerminate();
        throw std::runtime_error("cannot load the OpenGL functions");
    }

    glfwSwapInterval(config.vsync ? 1 : 0);
    glfwSetWindowUserPointer(_handle, this);
    glfwSetKeyCallback(_handle, &Window::onKey);
    glfwSetFramebufferSizeCallback(_handle, &Window::onResize);
}

Window::~Window()
{
    glfwDestroyWindow(_handle);
    glfwTerminate();
}

bool Window::isOpen() const
{
    return glfwWindowShouldClose(_handle) == GLFW_FALSE;
}

void Window::close()
{
    glfwSetWindowShouldClose(_handle, GLFW_TRUE);
}

void Window::pollEvents()
{
    glfwPollEvents();
}

void Window::swapBuffers()
{
    glfwSwapBuffers(_handle);
}

void Window::clear(const Color &color)
{
    if (glfwGetCurrentContext() != _handle)
        glfwMakeContextCurrent(_handle);
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Window::setKeyCallback(KeyCallback callback)
{
    _keyCallback = std::move(callback);
}

double Window::time()
{
    return glfwGetTime();
}

void Window::onKey(
    GLFWwindow *handle, int key, int /*scancode*/, int action, int /*mods*/)
{
    auto *window = static_cast<Window *>(glfwGetWindowUserPointer(handle));

    if (window->_keyCallback && action != GLFW_REPEAT)
        window->_keyCallback(key, action == GLFW_PRESS);
}

void Window::onResize(GLFWwindow * /*handle*/, int width, int height)
{
    glViewport(0, 0, width, height);
}
} // namespace RType

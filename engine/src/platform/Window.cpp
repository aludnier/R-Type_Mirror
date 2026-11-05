/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Window.cpp
*/

#include "Window.h"

#include <stdexcept>
#include <utility>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace RType {
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
    if (gladLoadGLLoader(
            reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0) {
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

void Window::setKeyCallback(KeyCallback callback)
{
    _keyCallback = std::move(callback);
}

double Window::time() const
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

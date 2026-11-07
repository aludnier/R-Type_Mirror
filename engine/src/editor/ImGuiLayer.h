/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ImGuiLayer.h
*/

#ifndef IMGUILAYER_H_
#define IMGUILAYER_H_

#include "platform/Window.h"

namespace RType::Editor {
/**
 * @brief Runs ImGui (docking, GLFW and OpenGL 3 backends) in a window.
 *
 * The GLFW callbacks of the window keep working: ImGui installs its own and
 * calls the previous ones first, so a key reaches the window, then ImGui.
 *
 * @note Create it after the window and destroy it before.
 */
class ImGuiLayer {
public:
    explicit ImGuiLayer(Window &window);
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer &) = delete;
    ImGuiLayer &operator=(const ImGuiLayer &) = delete;

    /** @brief Starts an ImGui frame: call it before any ImGui widget. */
    static void beginFrame();
    /** @brief Draws the ImGui frame in the current framebuffer. */
    static void endFrame();
};
} // namespace RType::Editor

#endif /* !IMGUILAYER_H_ */

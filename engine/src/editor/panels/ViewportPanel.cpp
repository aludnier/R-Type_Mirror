/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ViewportPanel.cpp
*/

#include "ViewportPanel.h"

#include <imgui.h>

namespace RType::Editor {
void ViewportPanel::draw(EditorContext &context)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::Begin(TITLE);
    ImGui::PopStyleVar();

    const ImVec2 size = ImGui::GetContentRegionAvail();

    if (context.sceneRenderer != nullptr && context.framebuffer != nullptr &&
        size.x > 0.f && size.y > 0.f) {
        Framebuffer &framebuffer = *context.framebuffer;

        framebuffer.resize(static_cast<int>(size.x), static_cast<int>(size.y));
        framebuffer.clear({0.f, 0.f, 0.f, 1.f});
        context.sceneRenderer->render(context.registry, framebuffer, _camera);
        // OpenGL images start at the bottom, ImGui at the top: flip it.
        ImGui::Image(static_cast<ImTextureID>(framebuffer.textureId()),
            size,
            ImVec2(0.f, 1.f),
            ImVec2(1.f, 0.f));
    }
    ImGui::End();
}
} // namespace RType::Editor

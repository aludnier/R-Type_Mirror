/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ViewportPanel.cpp
*/

#include "ViewportPanel.h"

#include <imgui.h>

namespace RType::Editor {
void ViewportPanel::draw(EditorContext & /*context*/)
{
    ImGui::Begin(TITLE);
    ImGui::End();
}
} // namespace RType::Editor

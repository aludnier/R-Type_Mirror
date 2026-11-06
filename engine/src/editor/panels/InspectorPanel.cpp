/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** InspectorPanel.cpp
*/

#include "InspectorPanel.h"

#include <imgui.h>

namespace RType::Editor {
void InspectorPanel::draw(EditorContext & /*context*/)
{
    ImGui::Begin(_title);
    ImGui::End();
}
} // namespace RType::Editor

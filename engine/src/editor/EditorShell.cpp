/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** EditorShell.cpp
*/

#include "EditorShell.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace {
void buildDefaultLayout(ImGuiID dockspace, const ImGuiViewport *viewport)
{
    using RType::Editor::HierarchyPanel;
    using RType::Editor::InspectorPanel;
    using RType::Editor::ViewportPanel;

    ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace, viewport->WorkSize);
    ImGuiID center = dockspace;
    const ImGuiID left = ImGui::DockBuilderSplitNode(
        center, ImGuiDir_Left, 0.2f, nullptr, &center);
    const ImGuiID right = ImGui::DockBuilderSplitNode(
        center, ImGuiDir_Right, 0.25f, nullptr, &center);
    ImGui::DockBuilderDockWindow(HierarchyPanel::TITLE, left);
    ImGui::DockBuilderDockWindow(InspectorPanel::TITLE, right);
    ImGui::DockBuilderDockWindow(ViewportPanel::TITLE, center);
    ImGui::DockBuilderFinish(dockspace);
}
} // namespace

namespace RType::Editor {
void EditorShell::draw(EditorContext &context)
{
    drawMenuBar(context);

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    const ImGuiID dockspace = ImGui::GetID("EditorDockSpace");
    // No node yet: first run, imgui.ini did not restore a layout.
    if (ImGui::DockBuilderGetNode(dockspace) == nullptr)
        buildDefaultLayout(dockspace, viewport);
    ImGui::DockSpaceOverViewport(dockspace, viewport);

    _hierarchy.draw(context);
    _viewport.draw(context);
    _inspector.draw(context);
}

void EditorShell::drawMenuBar(EditorContext &context)
{
    if (ImGui::BeginMainMenuBar()) {
        _fileMenu.drawMenu(context);
        // No play mode yet.
        ImGui::BeginDisabled();
        ImGui::MenuItem("Play");
        ImGui::MenuItem("Stop");
        ImGui::EndDisabled();
        ImGui::Separator();
        ImGui::TextDisabled("%s",
            context.scenePath ? context.scenePath->filename().string().c_str()
                              : "untitled");
        ImGui::EndMainMenuBar();
    }
    _fileMenu.drawPopups(context);
}
} // namespace RType::Editor

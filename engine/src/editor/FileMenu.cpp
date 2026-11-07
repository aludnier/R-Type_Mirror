/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** FileMenu.cpp
*/

#include "FileMenu.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "editor/SceneFile.h"

namespace {
constexpr const char *OPEN_POPUP = "Open scene";
constexpr const char *SAVE_POPUP = "Save scene as";
} // namespace

namespace RType::Editor {
void FileMenu::drawMenu(EditorContext &context)
{
    const bool enabled = !context.readOnly;
    const bool canSave = enabled && context.assets != nullptr;

    if (enabled) {
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_N))
            SceneFile::newScene(context);
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_O))
            request(Dialog::Open, context);
        if (canSave && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_S))
            save(context);
    }
    if (!ImGui::BeginMenu("File", enabled))
        return;
    if (ImGui::MenuItem("New", "Ctrl+N"))
        SceneFile::newScene(context);
    if (ImGui::MenuItem("Open...", "Ctrl+O"))
        request(Dialog::Open, context);
    if (ImGui::MenuItem("Save", "Ctrl+S", false, canSave))
        save(context);
    if (ImGui::MenuItem("Save As...", nullptr, false, canSave))
        request(Dialog::SaveAs, context);
    if (!canSave && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        ImGui::SetTooltip("Saving needs an AssetManager to name the assets");
    ImGui::EndMenu();
}

void FileMenu::drawPopups(EditorContext &context)
{
    // Opened here: popups opened inside the menu would close with it.
    if (_requested == Dialog::Open)
        ImGui::OpenPopup(OPEN_POPUP);
    if (_requested == Dialog::SaveAs)
        ImGui::OpenPopup(SAVE_POPUP);
    _requested = Dialog::None;

    drawPathPopup(OPEN_POPUP, "Open", [&](const std::filesystem::path &file) {
        return SceneFile::openScene(context, file);
    });
    drawPathPopup(SAVE_POPUP, "Save", [&](const std::filesystem::path &file) {
        return SceneFile::saveScene(context, file);
    });
}

void FileMenu::save(EditorContext &context)
{
    if (!context.scenePath) {
        request(Dialog::SaveAs, context);
        return;
    }
    if (auto error = SceneFile::saveScene(context, *context.scenePath)) {
        _error = *error;
        request(Dialog::SaveAs, context);
    }
}

void FileMenu::request(Dialog dialog, const EditorContext &context)
{
    _requested = dialog;
    _error.clear();
    if (context.scenePath)
        _path = context.scenePath->string();
}

void FileMenu::drawPathPopup(
    const char *title, const char *button, const Action &action)
{
    if (!ImGui::BeginPopupModal(
            title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;
    if (ImGui::IsWindowAppearing())
        ImGui::SetKeyboardFocusHere();
    bool confirmed =
        ImGui::InputText("Path", &_path, ImGuiInputTextFlags_EnterReturnsTrue);
    if (!_error.empty())
        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s", _error.c_str());
    confirmed |= ImGui::Button(button);
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
        ImGui::CloseCurrentPopup();
    if (confirmed) {
        if (auto error = action(_path))
            _error = *error;
        else
            ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}
} // namespace RType::Editor

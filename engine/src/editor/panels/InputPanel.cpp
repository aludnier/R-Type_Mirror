/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** InputPanel.cpp
*/

#include "InputPanel.h"

#include <algorithm>
#include <exception>
#include <utility>
#include <vector>

#include <imgui.h>
#include <imgui_stdlib.h>

#include "library/Keys.h"

namespace {
constexpr const char *ADD_POPUP = "Add action";
constexpr const char *RENAME_POPUP = "Rename action";
constexpr const char *SAVE_POPUP = "Save input config as";

/** @brief Names of the keys that are not a single printable character. */
constexpr std::pair<int, const char *> KEY_NAMES[] = {
    {RType::Key::Space, "Space"},
    {RType::Key::World1, "World 1"},
    {RType::Key::World2, "World 2"},
    {RType::Key::Escape, "Escape"},
    {RType::Key::Enter, "Enter"},
    {RType::Key::Tab, "Tab"},
    {RType::Key::Backspace, "Backspace"},
    {RType::Key::Insert, "Insert"},
    {RType::Key::Delete, "Delete"},
    {RType::Key::Right, "Right"},
    {RType::Key::Left, "Left"},
    {RType::Key::Down, "Down"},
    {RType::Key::Up, "Up"},
    {RType::Key::PageUp, "Page Up"},
    {RType::Key::PageDown, "Page Down"},
    {RType::Key::Home, "Home"},
    {RType::Key::End, "End"},
    {RType::Key::CapsLock, "Caps Lock"},
    {RType::Key::ScrollLock, "Scroll Lock"},
    {RType::Key::NumLock, "Num Lock"},
    {RType::Key::PrintScreen, "Print Screen"},
    {RType::Key::Pause, "Pause"},
    {RType::Key::KeypadDecimal, "Keypad ."},
    {RType::Key::KeypadDivide, "Keypad /"},
    {RType::Key::KeypadMultiply, "Keypad *"},
    {RType::Key::KeypadSubtract, "Keypad -"},
    {RType::Key::KeypadAdd, "Keypad +"},
    {RType::Key::KeypadEnter, "Keypad Enter"},
    {RType::Key::KeypadEqual, "Keypad ="},
    {RType::Key::LeftShift, "Left Shift"},
    {RType::Key::LeftControl, "Left Ctrl"},
    {RType::Key::LeftAlt, "Left Alt"},
    {RType::Key::LeftSuper, "Left Super"},
    {RType::Key::RightShift, "Right Shift"},
    {RType::Key::RightControl, "Right Ctrl"},
    {RType::Key::RightAlt, "Right Alt"},
    {RType::Key::RightSuper, "Right Super"},
    {RType::Key::Menu, "Menu"},
};

std::string keyName(int key)
{
    for (const auto &[code, name] : KEY_NAMES)
        if (code == key)
            return name;
    // GLFW uses the ASCII code of printable keys, in upper case.
    if (key > RType::Key::Space && key <= RType::Key::GraveAccent) {
        const char printable = static_cast<char>(key);
        return {&printable, 1};
    }
    if (key >= RType::Key::F1 && key <= RType::Key::F25)
        return "F" + std::to_string(key - RType::Key::F1 + 1);
    if (key >= RType::Key::Keypad0 && key <= RType::Key::Keypad9)
        return "Keypad " + std::to_string(key - RType::Key::Keypad0);
    return "Key " + std::to_string(key);
}
} // namespace

namespace RType::Editor {
void InputPanel::draw(EditorContext &context)
{
    ImGui::Begin(TITLE);
    if (context.input == nullptr) {
        ImGui::TextDisabled("No input manager");
        ImGui::End();
        return;
    }
    if (context.readOnly)
        _capturing.reset();
    captureInput(context);
    ImGui::BeginDisabled(context.readOnly);
    drawToolbar(context);
    ImGui::Separator();
    drawActions(context);
    ImGui::EndDisabled();
    drawPopups(context);
    ImGui::End();
}

void InputPanel::drawToolbar(EditorContext &context)
{
    if (ImGui::Button("Add action"))
        request(Popup::AddAction, "");
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        if (!context.inputPath)
            request(Popup::SaveAs, "input.json");
        else if (auto error = save(context, *context.inputPath)) {
            // After request(), which clears the error.
            request(Popup::SaveAs, context.inputPath->string());
            _error = *error;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As..."))
        request(Popup::SaveAs,
            context.inputPath ? context.inputPath->string() : "input.json");
    ImGui::SameLine();
    ImGui::TextDisabled("%s",
        context.inputPath ? context.inputPath->filename().string().c_str()
                          : "unsaved");
}

void InputPanel::drawActions(EditorContext &context)
{
    InputManager &input = *context.input;
    std::optional<std::string> toRemove;

    if (input.actions().empty())
        ImGui::TextDisabled("No action");
    for (const auto &[name, action] : input.actions()) {
        ImGui::PushID(name.c_str());
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(name.c_str());
        if (ImGui::BeginPopupContextItem("actionMenu")) {
            if (ImGui::MenuItem("Rename")) {
                _renaming = name;
                request(Popup::RenameAction, name);
            }
            if (ImGui::MenuItem("Remove"))
                toRemove = name;
            ImGui::EndPopup();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("(bit %u)", static_cast<unsigned>(action));

        ImGui::Indent();
        // Copied: unbinding changes the list.
        const std::vector<std::string> bound = input.bindings(action);
        for (const std::string &id : bound) {
            ImGui::PushID(id.c_str());
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(inputLabel(id).c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("x"))
                input.unbind(action, std::string_view(id));
            ImGui::PopID();
        }
        if (_capturing == name) {
            ImGui::TextColored(ImVec4(1.f, 0.85f, 0.2f, 1.f), "Press a key...");
            ImGui::SameLine();
            if (ImGui::SmallButton("Cancel"))
                _capturing.reset();
        } else if (ImGui::SmallButton("Bind...")) {
            _capturing = name;
            _pressedBefore = input.pressedInputs();
        }
        ImGui::Unindent();
        ImGui::PopID();
    }
    // Changed after the loop: actions() must not change while iterating.
    if (toRemove) {
        if (_capturing == toRemove)
            _capturing.reset();
        input.removeAction(*toRemove);
    }
}

void InputPanel::captureInput(EditorContext &context)
{
    if (!_capturing)
        return;
    InputManager &input = *context.input;
    const auto action = input.actions().find(*_capturing);

    if (action == input.actions().end()) {
        _capturing.reset();
        return;
    }
    // A key released since Bind was pressed can be captured again.
    std::erase_if(_pressedBefore, [&](const std::string &id) {
        return !input.pressedInputs().contains(id);
    });
    if (auto id = newInput(_pressedBefore, input.pressedInputs())) {
        input.bind(action->second, *id);
        _capturing.reset();
    }
}

void InputPanel::request(Popup popup, std::string text)
{
    _requested = popup;
    _text = std::move(text);
    _error.clear();
}

void InputPanel::drawPopups(EditorContext &context)
{
    // Opened here: a popup opened inside a context menu closes with it.
    if (_requested == Popup::AddAction)
        ImGui::OpenPopup(ADD_POPUP);
    if (_requested == Popup::RenameAction)
        ImGui::OpenPopup(RENAME_POPUP);
    if (_requested == Popup::SaveAs)
        ImGui::OpenPopup(SAVE_POPUP);
    _requested = Popup::None;

    const std::pair<const char *, const char *> popups[] = {
        {ADD_POPUP, "Add"}, {RENAME_POPUP, "Rename"}, {SAVE_POPUP, "Save"}};
    for (const auto &[title, button] : popups) {
        if (!ImGui::BeginPopupModal(
                title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            continue;
        if (ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        bool confirmed = ImGui::InputText(title == SAVE_POPUP ? "Path" : "Name",
            &_text,
            ImGuiInputTextFlags_EnterReturnsTrue);
        if (!_error.empty())
            ImGui::TextColored(
                ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s", _error.c_str());
        confirmed |= ImGui::Button(button);
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        if (confirmed) {
            std::optional<std::string> error;
            if (title == ADD_POPUP)
                error = addAction(context, _text);
            else if (title == RENAME_POPUP)
                error = renameAction(context, _renaming, _text);
            else
                error = save(context, _text);
            if (error)
                _error = *error;
            else
                ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

std::optional<std::string> InputPanel::addAction(
    EditorContext &context, const std::string &name)
{
    if (context.input == nullptr)
        return "no input manager";
    if (name.empty())
        return "the name is empty";
    if (context.input->actions().contains(name))
        return "action '" + name + "' already exists";
    try {
        context.input->addAction(name);
    } catch (const std::exception &error) {
        return error.what();
    }
    return std::nullopt;
}

std::optional<std::string> InputPanel::renameAction(
    EditorContext &context, const std::string &from, const std::string &to)
{
    if (context.input == nullptr)
        return "no input manager";
    if (to.empty())
        return "the name is empty";
    try {
        context.input->renameAction(from, to);
    } catch (const std::exception &error) {
        return error.what();
    }
    return std::nullopt;
}

std::optional<std::string> InputPanel::save(
    EditorContext &context, const std::filesystem::path &file)
{
    if (context.input == nullptr)
        return "no input manager";
    if (file.empty())
        return "the path is empty";
    try {
        context.input->saveActions(file);
    } catch (const std::exception &error) {
        return error.what();
    }
    context.inputPath = file;
    return std::nullopt;
}

std::optional<std::string> InputPanel::newInput(
    const std::unordered_set<std::string> &before,
    const std::unordered_set<std::string> &now)
{
    std::optional<std::string> first;

    for (const std::string &id : now)
        if (!before.contains(id) && (!first || id < *first))
            first = id;
    return first;
}

std::string InputPanel::inputLabel(const std::string &input)
{
    if (const auto key = InputManager::keyCodeOf(input))
        return keyName(*key);
    return input;
}
} // namespace RType::Editor

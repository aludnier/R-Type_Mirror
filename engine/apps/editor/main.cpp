/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** main.cpp
*/

#include <exception>
#include <iostream>

#include "ecs/Registry.h"
#include "editor/EditorContext.h"
#include "editor/EditorShell.h"
#include "editor/ImGuiLayer.h"
#include "platform/Application.h"

namespace {
class Editor : public RType::Application {
public:
    Editor() :
        Application({.title = "Editor"}), _imgui(window())
    {
    }

protected:
    void update(float /*dt*/) override
    {
    }

    void render() override
    {
        window().clear({0.12f, 0.12f, 0.14f, 1.f});
        RType::Editor::ImGuiLayer::beginFrame();
        _shell.draw(_context);
        RType::Editor::ImGuiLayer::endFrame();
    }

private:
    RType::Editor::ImGuiLayer _imgui;
    RType::Registry _registry;
    RType::Editor::EditorContext _context {.registry = _registry};
    RType::Editor::EditorShell _shell {};
};
} // namespace

int main()
{
    try {
        Editor editor;

        editor.run();
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}

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
#include "library/AssetManager.h"
#include "platform/Application.h"
#include "reflection/Reflection.h"

namespace {
class Editor : public RType::Application {
public:
    Editor() :
        Application({.title = "Editor"}), _imgui(window())
    {
        RType::reflectCoreComponents(_reflection);
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
    RType::Reflection _reflection;
    AssetManager _assets;
    RType::Editor::EditorContext _context {
        .registry = _registry, .reflection = _reflection, .assets = &_assets};
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

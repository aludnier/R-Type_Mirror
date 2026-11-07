/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** main.cpp
*/

#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>

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
    /** @param assetConfig Asset config file to load, may be empty. */
    explicit Editor(const std::filesystem::path &assetConfig) :
        Application({.title = "Editor"}), _imgui(window())
    {
        RType::reflectCoreComponents(_reflection);
        if (assetConfig.empty())
            return;
        try {
            _assets.loadConfigFile(assetConfig);
        } catch (const std::exception &error) {
            std::cerr << "warning: cannot load the assets: " << error.what()
                      << "\n";
        }
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

int main(int argc, char **argv)
{
    if (argc > 2 ||
        (argc == 2 &&
            (std::string_view(argv[1]) == "-h" ||
                std::string_view(argv[1]) == "--help"))) {
        std::cout << "USAGE: " << argv[0] << " [assets.json]\n"
                  << "    assets.json  Asset config, needed to show and save "
                     "sprites\n";
        return argc == 2 ? 0 : 84;
    }
    try {
        Editor editor(argc == 2 ? argv[1] : "");

        editor.run();
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}

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
#include "render/Framebuffer.h"
#include "render/SceneRenderer.h"

namespace {
/** @brief Runs @p load unless @p file is empty, warns if it fails. */
template<typename Load>
void loadOptional(const std::filesystem::path &file, Load load)
{
    if (file.empty())
        return;
    try {
        load(file);
    } catch (const std::exception &error) {
        std::cerr << "warning: cannot load " << file.string() << ": "
                  << error.what() << "\n";
    }
}

class Editor : public RType::Application {
public:
    /**
     * @param assetConfig Asset config file to load, may be empty.
     * @param inputConfig Actions and keys of the game, may be empty.
     */
    Editor(const std::filesystem::path &assetConfig,
        const std::filesystem::path &inputConfig) :
        Application({.title = "Editor"}), _imgui(window())
    {
        RType::reflectCoreComponents(_reflection);
        loadOptional(assetConfig,
            [this](const auto &file) { _assets.loadConfigFile(file); });
        loadOptional(inputConfig,
            [this](const auto &file) { input().loadActions(file); });
    }

protected:
    void update(float dt) override
    {
        _shell.update(_context, dt);
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
    RType::SceneRenderer _sceneRenderer {_assets};
    RType::Framebuffer _framebuffer {1, 1};
    RType::Editor::EditorContext _context {.registry = _registry,
        .reflection = _reflection,
        .assets = &_assets,
        .input = &input(),
        .sceneRenderer = &_sceneRenderer,
        .framebuffer = &_framebuffer};
    RType::Editor::EditorShell _shell {};
};
} // namespace

int main(int argc, char **argv)
{
    if (argc > 3 ||
        (argc == 2 &&
            (std::string_view(argv[1]) == "-h" ||
                std::string_view(argv[1]) == "--help"))) {
        std::cout << "USAGE: " << argv[0] << " [assets.json [input.json]]\n"
                  << "    assets.json  Asset config, needed to show and save "
                     "sprites\n"
                  << "    input.json   Actions and keys, for scripts in Play "
                     "mode\n";
        return argc == 2 ? 0 : 84;
    }
    try {
        Editor editor(argc >= 2 ? argv[1] : "", argc == 3 ? argv[2] : "");

        editor.run();
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}

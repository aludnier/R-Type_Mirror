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
#include "library/PluginManager.h"
#include "platform/Application.h"
#include "reflection/Reflection.h"
#include "render/Framebuffer.h"
#include "render/SceneRenderer.h"

#include "Physics2DBindings.h"

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

/** @brief The game config of the build, used when none is given. */
std::filesystem::path defaultGameConfig()
{
    const std::filesystem::path file = ENGINE_MODULE_DIR "/../config/game.json";
    std::error_code error;
    return std::filesystem::exists(file, error) ? file
                                                : std::filesystem::path {};
}

class Editor : public RType::Application {
public:
    /**
     * @param assetConfig Asset config file to load, may be empty.
     * @param inputConfig Actions and keys of the game, may be empty.
     * @param gameConfig  Game config whose "plugins" are loaded, may be empty.
     */
    Editor(const std::filesystem::path &assetConfig,
        const std::filesystem::path &inputConfig,
        const std::filesystem::path &gameConfig) :
        Application({.title = "Editor"}), _imgui(window())
    {
        RType::reflectCoreComponents(_reflection);
        loadOptional(
            gameConfig, [this](const auto &file) { loadModules(file); });
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
    /**
     * @brief Loads the modules of the game, so their components show in the
     *        inspector and their systems run in Play mode.
     */
    void loadModules(const std::filesystem::path &gameConfig)
    {
        _plugins.loadConfig(gameConfig, _registry);
        _plugins.reflectModules(_reflection);
        if (_plugins.isLoaded(RType::Physics2D::MODULE_NAME))
            _context.scriptBindings.emplace_back(
                RType::Physics2D::bindScripting);
    }

    // First: the registry and the reflection hold code of the modules.
    RType::PluginManager _plugins;
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
    if (argc > 4 ||
        (argc == 2 &&
            (std::string_view(argv[1]) == "-h" ||
                std::string_view(argv[1]) == "--help"))) {
        std::cout << "USAGE: " << argv[0]
                  << " [assets.json [input.json [game.json]]]\n"
                  << "    assets.json  Asset config, needed to show and save "
                     "sprites\n"
                  << "    input.json   Actions and keys, for scripts in Play "
                     "mode\n"
                  << "    game.json    Game config: its \"plugins\" are "
                     "loaded (default: the\n"
                  << "                 build's config/game.json)\n";
        return argc == 2 ? 0 : 84;
    }
    try {
        Editor editor(argc >= 2 ? argv[1] : "",
            argc >= 3 ? argv[2] : "",
            argc >= 4 ? std::filesystem::path(argv[3]) : defaultGameConfig());

        editor.run();
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}

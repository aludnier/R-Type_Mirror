#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string_view>

#include "Physics2DBindings.h"
#include "components/Controllable.h"
#include "ecs/Registry.h"
#include "library/AssetManager.h"
#include "library/PluginManager.h"
#include "platform/Application.h"
#include "reflection/Reflection.h"
#include "render/Camera2D.h"
#include "render/SceneRenderer.h"
#include "scene/Scene.h"
#include "scripting/ScriptEngine.h"

namespace {
class TowerDefense final : public RType::Application {
public:
    TowerDefense(const std::filesystem::path &scene,
        const std::filesystem::path &assets,
        const std::filesystem::path &input,
        const std::filesystem::path &gameConfig) :
        Application({.title = "Tower Defense",
            .width = 1280,
            .height = 720,
            .vsync = true}),
        _sceneRenderer(_assets)
    {
        RType::reflectCoreComponents(_reflection);

        const RType::Configuration configuration(gameConfig);
        _plugins.loadConfig(configuration, _registry);
        _plugins.reflectModules(_reflection);
        for (const auto &component : _reflection.components())
            component.ensureRegistered(_registry);

        _assets.loadConfigFile(assets);
        this->input().loadActions(input);

        _scripts = std::make_unique<RType::ScriptEngine>(_registry);
        if (_plugins.isLoaded(RType::Physics2D::MODULE_NAME))
            RType::Physics2D::bindScripting(*_scripts);
        for (const auto &[name, action] : this->input().actions())
            _scripts->defineAction(name, action);

        RType::Scene::loadFromFile(_registry, _reflection, scene, _assets);
    }

protected:
    void update(float dt) override
    {
        const auto actions = input().actionMask();
        for (auto &controllable : _registry.getComponents<RType::Controllable>())
            if (controllable)
                controllable->actionMask = actions;

        _scripts->setDeltaTime(dt);
        _scripts->update(_registry.getComponents<RType::ScriptComponent>());
        _registry.runSystems();
    }

    void render() override
    {
        window().clear({0.12f, 0.12f, 0.14f, 1.f});
        _sceneRenderer.render(_registry, window(), _camera);
    }

private:
    RType::PluginManager _plugins;
    RType::Registry _registry;
    RType::Reflection _reflection;
    AssetManager _assets;
    RType::SceneRenderer _sceneRenderer;
    std::unique_ptr<RType::ScriptEngine> _scripts;
    RType::Camera2D _camera;
};

std::filesystem::path defaultGameConfig()
{
    return std::filesystem::path(ENGINE_MODULE_DIR) / "../config/game.json";
}

void printUsage(std::string_view program)
{
    std::cout << "USAGE: " << program
              << " [scene.json [assets.json [input.json [game.json]]]]\n"
              << "Defaults to the Tower Defense files in towerDefense/.\n";
}
} // namespace

int main(int argc, char **argv)
{
    if (argc == 2 &&
        (std::string_view(argv[1]) == "-h" ||
            std::string_view(argv[1]) == "--help")) {
        printUsage(argv[0]);
        return 0;
    }
    if (argc > 5) {
        printUsage(argv[0]);
        return 2;
    }

    try {
        std::filesystem::current_path(TOWER_DEFENSE_ROOT);
        const std::filesystem::path project = "towerDefense";
        TowerDefense game(argc >= 2 ? argv[1] : project / "scene.json",
            argc >= 3 ? argv[2] : project / "config/assets.json",
            argc >= 4 ? argv[3] : project / "config/input.json",
            argc >= 5 ? argv[4] : defaultGameConfig());
        game.run();
    } catch (const std::exception &error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
    return 0;
}

#include "TowerDefense.h"

#include "Physics2DBindings.h"
#include "components/Controllable.h"
#include "scene/Scene.h"

namespace TowerDefenseGame {
TowerDefense::TowerDefense(const std::filesystem::path &scene,
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

void TowerDefense::update(float dt)
{
    const auto actions = input().actionMask();
    for (auto &controllable : _registry.getComponents<RType::Controllable>())
        if (controllable)
            controllable->actionMask = actions;

    _scripts->setDeltaTime(dt);
    _scripts->update(_registry.getComponents<RType::ScriptComponent>());
    _registry.runSystems();
}

void TowerDefense::render()
{
    window().clear({0.12f, 0.12f, 0.14f, 1.f});
    _sceneRenderer.render(_registry, window(), _camera);
}
} // namespace TowerDefenseGame

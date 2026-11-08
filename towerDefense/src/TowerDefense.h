#ifndef TOWER_DEFENSE_H
#define TOWER_DEFENSE_H

#include <filesystem>
#include <memory>

#include "ecs/Registry.h"
#include "library/AssetManager.h"
#include "library/PluginManager.h"
#include "platform/Application.h"
#include "reflection/Reflection.h"
#include "render/Camera2D.h"
#include "render/SceneRenderer.h"
#include "scripting/ScriptEngine.h"

namespace TowerDefenseGame {
class TowerDefense final : public RType::Application {
public:
    TowerDefense(const std::filesystem::path &scene,
        const std::filesystem::path &assets,
        const std::filesystem::path &input,
        const std::filesystem::path &gameConfig);

protected:
    void update(float dt) override;
    void render() override;

private:
    RType::PluginManager _plugins;
    RType::Registry _registry;
    RType::Reflection _reflection;
    AssetManager _assets;
    RType::SceneRenderer _sceneRenderer;
    std::unique_ptr<RType::ScriptEngine> _scripts;
    RType::Camera2D _camera;
};
} // namespace TowerDefenseGame

#endif

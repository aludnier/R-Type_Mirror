/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** PlayMode.cpp
*/

#include "PlayMode.h"

#include "components/Controllable.h"
#include "ecs/SparseArray.h"
#include "scene/Scene.h"
#include "scripting/ScriptComponent.h"

namespace RType::Editor {
void PlayMode::play(EditorContext &context)
{
    if (isPlaying() || context.assets == nullptr)
        return;
    // Scripts can use any component, but the editor registers them on demand.
    for (const ComponentInfo &info : context.reflection.components())
        info.ensureRegistered(context.registry);
    _snapshot =
        Scene::toJson(context.registry, context.reflection, *context.assets);
    _scripts.emplace(context.registry);
    if (context.input != nullptr)
        for (const auto &[name, action] : context.input->actions())
            _scripts->defineAction(name, action);
    context.readOnly = true;
}

void PlayMode::update(EditorContext &context, float dt, ActionMask actions)
{
    if (!isPlaying())
        return;
    for (auto &controllable : context.registry.getComponents<Controllable>())
        if (controllable)
            controllable->actionMask = actions;
    _scripts->setDeltaTime(dt);
    _scripts->update(context.registry.getComponents<ScriptComponent>());
    context.registry.runSystems();
}

void PlayMode::stop(EditorContext &context)
{
    if (!isPlaying())
        return;
    // First: the engine holds Lua references to the entities.
    _scripts.reset();
    Scene::clear(context.registry);
    Scene::fromJson(
        context.registry, context.reflection, _snapshot, *context.assets);
    // The restored entities have new ids.
    context.clearSelection();
    context.readOnly = false;
}

bool PlayMode::isPlaying() const
{
    return _scripts.has_value();
}
} // namespace RType::Editor

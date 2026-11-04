/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ScriptSystem.cpp
*/

#include "ScriptSystem.h"
#include "ScriptComponent.h"
#include "ecs/SparseArray.h"

void RType::addScriptSystem(Registry &registry, ScriptEngine &engine)
{
    registry.addSystem<ScriptComponent>(
        [&engine](
            SparseArray<ScriptComponent> &scripts) { engine.update(scripts); });
}

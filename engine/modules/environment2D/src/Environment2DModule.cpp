/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Environment2DModule.cpp
*/

#include "Environment2DModule.h"
#include "components/ParallaxBackground.h"
#include "ecs/SparseArray.h"
#include "reflection/Reflection.h"
#include "systems/ParallaxSystem.h"

namespace {
void registerComponents(RType::Registry &registry)
{
    registry.registerComponent<RType::Environment::ParallaxBackground>();
}

void registerSystems(RType::Registry &registry)
{
    RType::Environment::addParallaxSystem(registry, 1.0f / 60.0f);
}

RType::ModuleStatus registerEnvironmentModule(RType::Registry &registry)
{
    registerComponents(registry);
    registerSystems(registry);
    return RType::ModuleStatus::Success;
}

RType::ModuleStatus reflectEnvironmentModule(RType::Reflection &reflection)
{
    using RType::Environment::ParallaxBackground;
    reflection.reflect<ParallaxBackground>("ParallaxBackground")
        .field("scrollSpeed", &ParallaxBackground::scrollSpeed)
        .field("resetX", &ParallaxBackground::resetX)
        .field("startX", &ParallaxBackground::startX);
    return RType::ModuleStatus::Success;
}
} // namespace

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint()
{
    static const RType::ModuleDescriptor descriptor {
        .name = "environment2d",
        .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
        .dependencies = {},
        .registerModule = registerEnvironmentModule,
        .registerReflection = reflectEnvironmentModule,
    };
    return &descriptor;
}

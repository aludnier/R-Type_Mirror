#include "Module.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "TestDependencyComponent.h"
#include "TestPluginComponent.h"

#include <stdexcept>

namespace {
RType::ModuleStatus registerTestModule(RType::Registry &registry)
{
    try {
        registry.getComponents<TestDependencyComponent>();
    } catch (const std::out_of_range &) {
        return RType::ModuleStatus::InvalidRegistry;
    }
    registry.registerComponent<TestPluginComponent>();
    registry.addSystem<TestPluginComponent>([](auto &values) {
        for (auto &value : values) {
            if (value)
                ++value->value;
        }
    });
    return RType::ModuleStatus::Success;
}

const RType::ModuleDescriptor moduleDescriptor{
    .name = "test_module",
    .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
    .dependencies = {{"test_dependency_module",
        {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR}}},
    .registerModule = registerTestModule
};
}

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint()
{
    return &moduleDescriptor;
}

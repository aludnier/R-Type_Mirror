#include "Module.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "TestDependencyComponent.h"

namespace {
RType::ModuleStatus registerDependencyModule(RType::Registry &registry)
{
    registry.registerComponent<TestDependencyComponent>();
    return RType::ModuleStatus::Success;
}

const RType::ModuleDescriptor moduleDescriptor{
    .name = "test_dependency_module",
    .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
    .dependencies = {},
    .registerModule = registerDependencyModule
};
}

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint()
{
    return &moduleDescriptor;
}

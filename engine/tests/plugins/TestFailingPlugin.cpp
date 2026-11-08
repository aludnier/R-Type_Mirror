#include "Module.h"
#include "TestFailingComponent.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"

#include <stdexcept>

namespace {
RType::ModuleStatus registerFailingModule(RType::Registry &registry)
{
    registry.registerComponent<TestFailingComponent>();
    throw std::runtime_error("test plugin initialization failed");
}

const RType::ModuleDescriptor moduleDescriptor {.name = "test_failing_module",
    .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
    .dependencies = {{"test_dependency_module",
        {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR}}},
    .registerModule = registerFailingModule};
} // namespace

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint()
{
    return &moduleDescriptor;
}

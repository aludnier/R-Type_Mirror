#include "library/PluginManager.h"
#include "TestDependencyComponent.h"

namespace {
    void initialize(RType::Registry *registry)
    {
        registry->registerComponent<TestDependencyComponent>();
    }

    void shutdown(RType::Registry *registry)
    {
        registry->unregisterComponent<TestDependencyComponent>();
    }

    const Engine::Plugin::Descriptor descriptor{
        .abiVersion = Engine::Plugin::ABI_VERSION,
        .name = "test_dependency",
        .components = nullptr,
        .componentCount = 0,
        .systems = nullptr,
        .systemCount = 0,
        .dependencies = nullptr,
        .dependencyCount = 0,
        .initialize = initialize,
        .shutdown = shutdown
    };
}

ENGINE_PLUGIN_EXPORT const Engine::Plugin::Descriptor *
engine_plugin_descriptor()
{
    return &descriptor;
}

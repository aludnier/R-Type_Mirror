#include "library/PluginManager.h"
#include "TestFailingComponent.h"

#include <stdexcept>

namespace {
    constexpr const char *components[] = {"TestFailingComponent"};
    constexpr const char *dependencies[] = {"test_dependency"};

    void initialize(RType::Registry *registry)
    {
        registry->registerComponent<TestFailingComponent>();
        throw std::runtime_error("test plugin initialization failed");
    }

    void shutdown(RType::Registry *registry)
    {
    }

    const Engine::Plugin::Descriptor descriptor{
        .abiVersion = Engine::Plugin::ABI_VERSION,
        .name = "test_failing_plugin",
        .components = components,
        .componentCount = 1,
        .systems = nullptr,
        .systemCount = 0,
        .dependencies = dependencies,
        .dependencyCount = 1,
        .initialize = initialize,
        .shutdown = shutdown
    };
}

ENGINE_PLUGIN_EXPORT const Engine::Plugin::Descriptor *
engine_plugin_descriptor()
{
    return &descriptor;
}

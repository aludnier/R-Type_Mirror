/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** example_module.cpp
*/

#include "example/example_api.h"
#include "ecs/SparseArray.h"

namespace {
    RType::Example::ModuleStatus registerExampleModule(RType::Registry &registry) {

        registry.registerComponent<RType::Example::ExampleComponent>();
        registry.addSystem<RType::Example::ExampleComponent>([](auto &values) {
            for (auto &value: values) {
                if (value.has_value()) {
                    ++value->value;
                }
            }
        });
        return RType::Example::ModuleStatus::Success;
    }
}

extern "C" EXAMPLE_MODULE_EXPORT const RType::Example::ModuleDescriptor *EXAMPLE_MODULE_CALL
RTypeLibEntryPoint() {
    static const RType::Example::ModuleDescriptor descriptor{
        .name = "example",
        .apiVersion = {RType::Example::MODULE_API_MAJOR, RType::Example::MODULE_API_MINOR},
        .dependencies = {},
        .registerModule = registerExampleModule,
    };
    return &descriptor;
}

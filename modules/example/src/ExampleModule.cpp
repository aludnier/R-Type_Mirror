/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** example_module.cpp
*/

#include "ExampleModule.h"
#include "ecs/SparseArray.h"

namespace {
    RType::ModuleStatus registerExampleModule(RType::Registry &registry) {

        registry.registerComponent<RType::Example::ExampleComponent>();
        registry.addSystem<RType::Example::ExampleComponent>([](auto &values) {
            for (auto &value: values) {
                if (value.has_value()) {
                    ++value->value;
                }
            }
        });
        return RType::ModuleStatus::Success;
    }
}

extern "C" MODULE_EXPORT const RType::ModuleDescriptor *MODULE_CALL
RTypeLibEntryPoint() {
    static const RType::ModuleDescriptor descriptor{
        .name = "example",
        .apiVersion = {RType::MODULE_API_MAJOR, RType::MODULE_API_MINOR},
        .dependencies = {},
        .registerModule = registerExampleModule,
    };
    return &descriptor;
}

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** example_api.h
*/


#ifndef EXAMPLE_API_H
    #define EXAMPLE_API_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "ecs/Registry.h"

namespace RType::Example {
    inline constexpr std::uint32_t MODULE_API_MAJOR = 1;
    inline constexpr std::uint32_t MODULE_API_MINOR = 0;

    enum class ModuleStatus {
        Success,
        InvalidRegistry,
    };

    struct ModuleApiVersion {
        std::uint32_t major;
        std::uint32_t minor;
    };

    struct ModuleDependency {
        std::string name;
        ModuleApiVersion compatible_api;
    };

    struct ExampleComponent {
        std::int32_t value = 0;
    };

    using RegisterModuleFn = std::function<ModuleStatus(Registry &)>;

    struct ModuleDescriptor {
        std::string name;
        ModuleApiVersion apiVersion;
        std::vector<ModuleDependency> dependencies;
        RegisterModuleFn registerModule;
    };
}

#if defined(_WIN32)
    #define EXAMPLE_MODULE_EXPORT __declspec(dllexport)
    #define EXAMPLE_MODULE_CALL __cdecl
#else
    #define EXAMPLE_MODULE_EXPORT __attribute__((visibility("default")))
    #define EXAMPLE_MODULE_CALL
#endif

extern "C" EXAMPLE_MODULE_EXPORT const RType::Example::ModuleDescriptor *EXAMPLE_MODULE_CALL
RTypeLibEntryPoint();

#endif // EXAMPLE_API_H

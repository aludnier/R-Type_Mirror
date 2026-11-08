/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Module.h
*/

#ifndef MODULE_H
#define MODULE_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "ecs/Registry.h"
#include "library/AssetLoaderRegistry.h"

#if defined(_WIN32)
#define MODULE_EXPORT __declspec(dllexport)
#define MODULE_CALL __cdecl
#else
#define MODULE_EXPORT __attribute__((visibility("default")))
#define MODULE_CALL
#endif

namespace RType {
class Reflection;

inline constexpr std::uint32_t MODULE_API_MAJOR = 1;
inline constexpr std::uint32_t MODULE_API_MINOR = 1;

enum class ModuleStatus {
    Success,
    InvalidRegistry,
};

struct ModuleApiVersion {
    std::uint32_t major {};
    std::uint32_t minor {};
};

struct ModuleDependency {
    std::string name {};
    ModuleApiVersion compatible_api {};
};

using RegisterModuleFn = std::function<ModuleStatus(Registry &)>;
using RegisterAssetTypesFn = std::function<ModuleStatus(AssetLoaderRegistry &)>;
/**
 * @brief Describes the module's components to the editor, so the inspector
 *        can show them and scenes can save them. Optional (API 1.1).
 */
using RegisterReflectionFn = std::function<ModuleStatus(Reflection &)>;

struct ModuleDescriptor {
    std::string name {};
    ModuleApiVersion apiVersion {};
    std::vector<ModuleDependency> dependencies {};
    RegisterModuleFn registerModule {};
    RegisterAssetTypesFn registerAssetTypes {};
    RegisterReflectionFn registerReflection {};
};
} // namespace RType

#endif // MODULE_H

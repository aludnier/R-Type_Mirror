/*
** EPITECH PROJECT, 2026
** Rtype
** File description:
** pluginManager
*/

#ifndef PLUGINMANAGER_H_
#define PLUGINMANAGER_H_

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "DLLoader.hpp"
#include "Module.h"
#include "ecs/Registry.h"

namespace RType {
class PluginManager {
public:
    /**
     * @brief Constructs an empty manager with no module loaded.
     */
    explicit PluginManager() = default;

    ~PluginManager() = default;

    PluginManager(const PluginManager &) = delete;
    PluginManager &operator=(const PluginManager &) = delete;

    /**
     * @brief Loads the requested modules and their transitive dependencies.
     *
     * Scans @p directory for shared libraries exporting `RTypeLibEntryPoint`,
     * uses the requested modules as roots of the dependency graph, then
     * registers the modules in dependency order.
     *
     * @param directory   Directory containing the module libraries.
     * @param moduleNames Names (ModuleDescriptor::name) of the requested
     * modules.
     * @param registry    Registry passed to each registerModule callback.
     */
    void loadRequiredModules(const std::filesystem::path &directory,
        const std::vector<std::string> &moduleNames, RType::Registry &registry);

    /**
     * @brief Loads a module library and the dependencies found next to it.
     * @param path     Path to the root module's shared library.
     * @param registry Registry passed to each registerModule callback.
     */
    void loadModule(
        const std::filesystem::path &path, RType::Registry &registry);

    /**
     * @brief Checks whether a module with this name is loaded.
     * @param name Module name.
     * @return true if loaded.
     */
    [[nodiscard]] bool isLoaded(const std::string &name) const;

private:
    using Loader = RType::DLLoader<RType::ModuleDescriptor>;
    using NameIndex = std::unordered_map<std::string, std::size_t>;

    /** @brief A module library, either read-only (candidate) or registered. */
    struct Module {
        Module(RType::PluginManager::Loader lib, const RType::ModuleDescriptor *descriptor) {
            library = std::move(lib);
            name =  descriptor->name;
            apiVersion = descriptor->apiVersion;
            dependencies = descriptor->dependencies;
            registerModule = descriptor->registerModule;
        }

        Loader library;
        std::string name;
        RType::ModuleApiVersion apiVersion {};
        std::vector<RType::ModuleDependency> dependencies;
        RType::RegisterModuleFn registerModule;
    };

    /** @brief Result of scanning a directory. The three members share indices.
     */
    struct Scan {
        std::vector<Module> modules;
        std::vector<std::filesystem::path> paths;
        NameIndex index;
    };

    /** @brief Node state used by the depth-first dependency traversal. */
    enum class VisitState : char { Unvisited, Visiting, Done };

    /** @brief Opens a library as persistent (code stays mapped). */
    static Loader openLibrary(const std::filesystem::path &path);

    /** @brief Tells whether a path has the platform's shared library extension.
     */
    static bool isSharedLibrary(const std::filesystem::path &path);

    static void visitModule(std::size_t index, const std::vector<Module> &candidates, const NameIndex &nameIndex,
        const std::unordered_map<std::string, RType::ModuleApiVersion> &loadedVersions,
        std::vector<VisitState> &state, std::vector<std::size_t> &order);

    /**
     * @brief Lists the shared libraries directly inside a directory.
     * @throws std::runtime_error If @p directory is not a directory.
     */
    static std::vector<std::filesystem::path> listLibraries(
        const std::filesystem::path &directory);

    /**
     * @brief Opens every library of a directory and keeps the ones exporting
     *        `RTypeLibEntryPoint`.
     * @throws std::runtime_error On load failure, invalid descriptor or
     *         duplicate module name.
     */
    static Scan scanDirectory(const std::filesystem::path &directory);

    /**
     * @brief Computes the registration order from several roots.
     *
     * Dependencies always come before their dependents. Dependencies that are
     * already loaded are only checked for API compatibility.
     *
     * @return Indices into @p candidates, in registration order.
     * @throws std::runtime_error On missing or incompatible dependency, or
     *         dependency cycle.
     */
    [[nodiscard]] std::vector<std::size_t> resolveLoadOrder(
        const std::vector<Module> &candidates, const NameIndex &nameIndex,
        const std::vector<std::size_t> &roots) const;

    /**
     * @brief Registers the modules in order and keeps them as loaded.
     */
    void registerModules(std::vector<Module> &candidates,
        const std::vector<std::size_t> &order, RType::Registry &registry);

    /**
     * @brief Forgets every module loaded after @p firstNew.
     * @param firstNew Number of modules that must stay loaded.
     */
    void rollback(std::size_t firstNew) noexcept;

    std::vector<Loader> _retiredLibraries;
    std::vector<Module> _loadedModules;
    std::optional<std::reference_wrapper<RType::Registry>> _registry;
    bool _configured = false;
};

} // namespace RType

#endif

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
#include "AssetLoaderRegistry.h"
#include "DLLoader.hpp"
#include "Module.h"
#include "configuration/Configuration.hpp"
#include "ecs/Registry.h"

namespace RType {
class PluginManager {
public:
    /**
     * @brief Constructs an empty manager with no module loaded.
     */
    explicit PluginManager() = default;

    /**
     * @brief Constructs a manager and loads its configured modules.
     * @param configFile JSON file with a "directory" and "modules" entry.
     * @param registry Registry passed to each module registration callback.
     */
    PluginManager(const std::filesystem::path &configFile, Registry &registry);

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
     * @param assets      Loader registry passed to each module.
     */
    void loadRequiredModules(const std::filesystem::path &directory,
        const std::vector<std::string> &moduleNames, Registry &registry);
    void loadRequiredModules(const std::filesystem::path &directory,
        const std::vector<std::string> &moduleNames, Registry &registry,
        AssetLoaderRegistry &assets);

    /**
     * @brief Loads module requirements from a JSON config file.
     * @param configFile JSON file with a "directory" and "modules" entry.
     * @param registry Registry passed to each module registration callback.
     */
    void loadConfig(
        const std::filesystem::path &configFile, Registry &registry);
    void loadConfig(const Configuration &config, Registry &registry);
    void loadConfig(const Configuration &config, Registry &registry,
        AssetLoaderRegistry &assets);

    /**
     * @brief Loads a module library and the dependencies found next to it.
     * @param path     Path to the root module's shared library.
     * @param registry Registry passed to each registerModule callback.
     * @param assets   Loader registry passed to each module.
     */
    void loadModule(const std::filesystem::path &path, Registry &registry);
    void loadModule(const std::filesystem::path &path, Registry &registry,
        AssetLoaderRegistry &assets);

    /**
     * @brief Checks whether a module with this name is loaded.
     * @param name Module name.
     * @return true if loaded.
     */
    [[nodiscard]] bool isLoaded(const std::string &name) const;

    /**
     * @brief Adds the components of every loaded module to @p reflection,
     *        for the modules that describe them (registerReflection).
     *        Call it once, after loading the modules.
     * @throws std::runtime_error if a module fails to describe them.
     */
    void reflectModules(Reflection &reflection) const;

private:
    using Loader = DLLoader<ModuleDescriptor>;
    using NameIndex = std::unordered_map<std::string, std::size_t>;

    /** @brief A module library, either read-only (candidate) or registered. */
    struct Module {
        Module(Loader lib, const ModuleDescriptor *descriptor)
        {
            library = std::move(lib);
            name = descriptor->name;
            apiVersion = descriptor->apiVersion;
            dependencies = descriptor->dependencies;
            registerModule = descriptor->registerModule;
            registerAssetTypes = descriptor->registerAssetTypes;
            registerReflection = descriptor->registerReflection;
        }

        Loader library;
        std::string name;
        ModuleApiVersion apiVersion {};
        std::vector<ModuleDependency> dependencies;
        RegisterModuleFn registerModule;
        RegisterAssetTypesFn registerAssetTypes;
        RegisterReflectionFn registerReflection;
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

    void visitModule(std::size_t index, const std::vector<Module> &candidates,
        std::vector<VisitState> &state, std::vector<std::size_t> &order) const;

    [[nodiscard]] const Module *findLoaded(const std::string &name) const;

    /**
     * @brief Registers the modules in order and keeps them as loaded.
     */
    void registerModules(std::vector<Module> &candidates,
        const std::vector<std::size_t> &order, Registry &registry);
    void registerModule(Module module, Registry &registry);
    void loadModuleFromPath(
        const std::filesystem::path &path, Registry &registry);

    /**
     * @brief Forgets every module loaded after @p firstNew.
     * @param firstNew Number of modules that must stay loaded.
     */
    void rollback(std::size_t firstNew) noexcept;

    std::vector<Loader> _retiredLibraries;
    std::vector<Module> _loadedModules;
    std::optional<std::reference_wrapper<Registry>> _registry;
    bool _configured = false;
    AssetLoaderRegistry *_assets = nullptr;
    AssetLoaderRegistry _defaultAssets;
};

} // namespace RType

#endif

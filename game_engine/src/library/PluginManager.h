/*
** EPITECH PROJECT, 2026
** Rtype
** File description:
** pluginManager
*/

#ifndef PLUGINMANAGER_H_
    #define PLUGINMANAGER_H_

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "DLLoader.hpp"

#include "ecs/Registry.h"
#include "Module.h"

#ifdef _WIN32
#define ENGINE_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define ENGINE_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace RType::Plugin {
    constexpr std::uint32_t ABI_VERSION = 2;
    constexpr const char *DESCRIPTOR_SYMBOL = "engine_plugin_descriptor";

    using InitializeFunction = void (*)(RType::Registry *);
    using ShutdownFunction = void (*)(RType::Registry *);

    struct Descriptor {
        std::uint32_t abiVersion;
        const char *name;
        const char *const *components;
        std::size_t componentCount;
        const char *const *systems;
        std::size_t systemCount;
        const char *const *dependencies;
        std::size_t dependencyCount;
        InitializeFunction initialize;
        ShutdownFunction shutdown;
    };

    using DescriptorFunction = const Descriptor *(*)();
}

namespace RType
{
class PluginManager
{
public:
    /**
     * @brief Constructs an empty plugin manager with no plugin or module loaded.
     */
    explicit PluginManager() = default;

    /**
     * @brief Shuts down every loaded plugin in reverse load order and
     *        releases their libraries.
     * @note Loaded modules have no shutdown callback and are simply
     *       released with their libraries, in reverse load order.
     *       Retired libraries are released last, also in reverse order.
     */
    ~PluginManager();

    PluginManager(const PluginManager &) = delete;
    PluginManager &operator=(const PluginManager &) = delete;

    /**
     * @brief Loads the plugins needed to provide the requested components
     *        and systems, together with their transitive dependencies.
     *
     * Scans @p directory for shared libraries, reads their descriptors,
     * finds which plugin owns each requested component/system, resolves
     * the dependency order, then initializes the plugins in that order.
     *
     * @param directory  Directory containing the plugin shared libraries.
     * @param components Names of the components that must be provided.
     * @param systems    Names of the systems that must be provided.
     * @param registry   Registry passed to each plugin's initialize callback.
     * @throws std::logic_error   If called more than once, or if the manager
     *         is already bound to another registry.
     * @throws std::runtime_error If the directory does not exist, a library
     *         cannot be loaded or has an invalid descriptor/ABI, two plugins
     *         share a name or provide the same component/system, a required
     *         component/system or dependency is missing, or a dependency
     *         cycle is detected.
     * @note The call is marked as configured even when both lists are empty
     *       (in which case nothing is loaded).
     * @note If a plugin's initialize callback throws, every plugin loaded
     *       by this call is shut down in reverse order before rethrowing.
     *       The shutdown callback is also called on the plugin whose
     *       initialize threw, so it must tolerate a partially initialized
     *       state.
     */
    void loadRequiredPlugins(const std::filesystem::path &directory,
        const std::vector<std::string> &components,
        const std::vector<std::string> &systems, RType::Registry &registry);

    /**
     * @brief Loads a module library and all the module dependencies found
     *        next to it.
     *
     * Every shared library in the same directory as @p path that exports
     * `RTypeLibEntryPoint` is considered as a candidate. Dependencies of
     * the root module are resolved (API compatibility is checked) and
     * registered before the module itself.
     *
     * @param path     Path to the root module's shared library.
     * @param registry Registry passed to each module's registerModule callback.
     * @throws std::logic_error   If the manager is already bound to another
     *         registry, or if the root module is already loaded.
     * @throws std::runtime_error If a descriptor is invalid or has an
     *         incompatible API, a module name is duplicated, the root library
     *         has no entry point, a dependency is missing or incompatible,
     *         a cycle is detected, or a module fails to initialize.
     * @note Modules already registered before a failure are kept loaded;
     *       no rollback is performed. The module that failed is removed
     *       from the loaded list (so a retry is possible) but its library
     *       stays mapped until the manager is destroyed.
     */
    void loadModule(const std::filesystem::path &path,
        RType::Registry &registry);

    /**
     * @brief Unloads a single plugin by name.
     * @param name Name of the plugin to unload.
     * @return true if the plugin was shut down and unloaded, false if no
     *         plugin has this name.
     * @throws std::logic_error   If @p name refers to a module (modules
     *         cannot be unloaded individually).
     * @throws std::runtime_error If another loaded plugin depends on it.
     * @note The plugin's library is not closed immediately: it is retired
     *       and stays mapped until the manager is destroyed, because the
     *       registry may still reference its code.
     */
    bool unload(const std::string &name);

    /**
     * @brief Shuts down and unloads all plugins in reverse load order.
     * @throws std::logic_error If at least one module is loaded.
     */
    void unloadAll();

    /**
     * @brief Checks whether a plugin or a module with this name is loaded.
     * @param name Name of the plugin or module.
     * @return true if a loaded plugin or module matches, false otherwise.
     */
    bool isLoaded(const std::string &name) const;

private:
   using Loader = RType::DLLoader<RType::ModuleDescriptor>;

    /** @brief Maps a name to the index of a candidate in a vector. */
    using OwnerMap = std::unordered_map<std::string, std::size_t>;

    /** @brief A plugin library that has been read but not yet initialized. */
    struct Candidate {
        Loader library;
        std::string name;
        std::vector<std::string> components;
        std::vector<std::string> systems;
        std::vector<std::string> dependencies;
        RType::Plugin::InitializeFunction initialize = nullptr;
        RType::Plugin::ShutdownFunction shutdown = nullptr;
    };

    /** @brief A plugin that has been initialized and is currently active. */
    struct LoadedPlugin {
        Loader library;
        std::string name;
        std::vector<std::string> dependencies;
        RType::Plugin::ShutdownFunction shutdown;
    };

    /** @brief A module that has been registered and is currently active. */
    struct LoadedModule {
        Loader library;
        std::string name;
        RType::ModuleApiVersion apiVersion{};
        std::vector<RType::ModuleDependency> dependencies;
        RType::RegisterModuleFn registerModule;
    };

    /** @brief A module library that has been read but not yet registered. */
    struct ModuleCandidate {
        Loader library;
        std::string name;
        RType::ModuleApiVersion apiVersion{};
        std::vector<RType::ModuleDependency> dependencies;
        RType::RegisterModuleFn registerModule;
    };

    /** @brief Node state used by the depth-first dependency traversal. */
    enum class VisitState : char {
        Unvisited,
        Visiting,
        Done
    };

    static std::vector<ModuleCandidate> loadModuleCandidates(const std::filesystem::path &rootPath,
        OwnerMap &nameIndex, std::optional<std::size_t> &rootIndex);
    std::vector<std::size_t> resolveModuleLoadOrder(
        const std::vector<ModuleCandidate> &candidates,
        const OwnerMap &nameIndex, std::size_t rootIndex) const;
    void registerModules(std::vector<ModuleCandidate> &candidates,
        const std::vector<std::size_t> &order, RType::Registry &registry);

   static Loader openLibrary(const std::filesystem::path &path);

   /**
     * @brief Tells whether a path has the platform's shared library
     *        extension (`.dll` on Windows, `.so` elsewhere).
     * @param path Path to test.
     * @return true if the extension matches.
     */
    static bool isSharedLibrary(const std::filesystem::path &path);

    /**
     * @brief Lists the shared libraries directly inside a directory.
     * @param directory Directory to scan (not recursive).
     * @return Sorted list of library paths.
     * @throws std::runtime_error If @p directory is not a directory.
     */
    static std::vector<std::filesystem::path> listLibraries(const std::filesystem::path &directory);

    /**
     * @brief Retrieves and validates the plugin descriptor of a library.
     * @param library Opened library.
     * @param path    Loader path, used for error messages.
     * @return Pointer to the descriptor owned by the library.
     * @throws std::runtime_error If the descriptor symbol is missing or the
     *         descriptor is invalid or has an incompatible ABI.
     */
    static const RType::Plugin::Descriptor *readDescriptor(const Loader &library, const std::filesystem::path &path);

    /**
     * @brief Checks a descriptor's ABI version, name and mandatory
     *        initialize callback.
     * @param descriptor Descriptor to check (may be nullptr).
     * @return true if the descriptor is usable.
     */
    static bool isValidDescriptor(const RType::Plugin::Descriptor *descriptor);

    /**
     * @brief Copies a C array of names into a vector of strings.
     * @param names      C array of null-terminated strings (may be null
     *                   only if @p count is 0).
     * @param count      Number of entries.
     * @param pluginName Owning plugin, used for error messages.
     * @param kind       Kind of name ("component", "system", "dependency"),
     *                   used for error messages.
     * @return The copied names.
     * @throws std::runtime_error If the array is null with a non-zero count,
     *         or if any entry is null or empty.
     */
    static std::vector<std::string> copyNames(const char *const *names, std::size_t count,
        std::string_view pluginName, std::string_view kind);

    /**
     * @brief Opens a library and builds a Candidate from its descriptor.
     * @param path Path to the plugin library.
     * @return The candidate, owning the opened library.
     * @throws std::runtime_error If the library or its descriptor is invalid.
     */
    static Candidate loadCandidate(const std::filesystem::path &path);

    /**
     * @brief Wrapper around loadCandidate() meant to allow skipping
     *        unloadable libraries.
     * @param path Path to the plugin library.
     * @return The candidate, or std::nullopt if the library is skipped.
     * @note Currently every error is propagated, so std::nullopt is never
     *       returned.
     */
    static std::optional<Candidate> tryLoadCandidate(const std::filesystem::path &path);

    /**
     * @brief Loads every plugin library of a directory as a candidate.
     * @param directory Directory to scan.
     * @return All candidates, in sorted path order.
     * @throws std::runtime_error If a library is invalid or two plugins
     *         share the same name.
     */
    static std::vector<Candidate> loadCandidates(const std::filesystem::path &directory);

    /**
     * @brief Registers the owner of each name in an OwnerMap.
     * @param owners      Map to fill.
     * @param names       Names provided by the plugin.
     * @param pluginIndex Index of the providing candidate.
     * @param kind        Kind of name, used for error messages.
     * @param pluginName  Plugin name, used for error messages.
     * @throws std::runtime_error If a name already has an owner.
     */
    static void addOwners(OwnerMap &owners, const std::vector<std::string> &names, std::size_t pluginIndex, std::string_view kind,
        std::string_view pluginName);

    /**
     * @brief Builds a name -> candidate index map for components or systems.
     * @param candidates All candidates.
     * @param names      Pointer to the Candidate member to index
     *                   (`&Candidate::components` or `&Candidate::systems`).
     * @param kind       Kind of name, used for error messages.
     * @return The ownership map.
     * @throws std::runtime_error If two candidates provide the same name.
     */
    static OwnerMap buildOwners(const std::vector<Candidate> &candidates, std::vector<std::string> Candidate::*names, std::string_view kind);

    /**
     * @brief Builds a plugin name -> candidate index map.
     * @param candidates All candidates.
     * @return The name index.
     */
    static OwnerMap buildNameIndex(const std::vector<Candidate> &candidates);

    /**
     * @brief Finds the candidate that provides each required name.
     * @param owners   Ownership map to search.
     * @param required Names that must be provided.
     * @param kind     Kind of name, used for error messages.
     * @return Candidate indices, in the same order as @p required.
     * @throws std::runtime_error If no plugin provides a required name.
     */
    static std::vector<std::size_t> findOwners(const OwnerMap &owners, const std::vector<std::string> &required, std::string_view kind);

    /**
     * @brief Computes the order in which candidates must be initialized.
     *
     * Uses a depth-first traversal from the owners of the required
     * components and systems, so dependencies always come before their
     * dependents and unrelated plugins are left out.
     *
     * @param candidates All candidates.
     * @param components Required component names.
     * @param systems    Required system names.
     * @return Candidate indices in initialization order.
     * @throws std::runtime_error If a requirement or dependency is missing,
     *         or if a dependency cycle is detected.
     */
    static std::vector<std::size_t> resolveLoadOrder(const std::vector<Candidate> &candidates, const std::vector<std::string> &components,
        const std::vector<std::string> &systems);

    /**
     * @brief Initializes the candidates following the given order and
     *        stores them as loaded plugins.
     * @param candidates Candidates to take libraries from (moved out).
     * @param order      Initialization order from resolveLoadOrder().
     * @param registry   Registry passed to each initialize callback.
     * @note Storage for the loaded and retired plugins is reserved before
     *       any callback runs, so the loading loop and the rollback do not
     *       allocate.
     * @note If an initialize callback throws, all plugins added by this
     *       call are shut down in reverse order, then the exception is
     *       rethrown.
     */
    void initializeInOrder(std::vector<Candidate> &candidates,
        const std::vector<std::size_t> &order, RType::Registry &registry);

    /**
     * @brief Calls a plugin's shutdown callback, if it has one.
     * @param plugin Plugin to shut down.
     * @note Does nothing if the plugin has no shutdown callback or no
     *       registry is set. Any exception thrown by the callback is
     *       caught and reported on std::cerr.
     */
    void shutdownPlugin(LoadedPlugin &plugin) noexcept;

    std::vector<Loader> _retiredLibraries;
    std::vector<LoadedPlugin> _loadedPlugins;
    std::vector<LoadedModule> _loadedModules;
    std::optional<std::reference_wrapper<RType::Registry>> _registry;
    bool _configured = false;
};

}


#endif

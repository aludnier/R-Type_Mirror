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

#include "ecs/Registry.h" // TODO: move Registry out of the RType namespace

/*
 * ABI NOTE
 * --------
 * The plugin boundary uses C++ types (Registry). Host and plugins MUST be
 * built with the same compiler, standard library, build type and (on Windows)
 * the same C runtime. The descriptor itself is plain C data so it can be read
 * safely before any other check.
 *
 * LIFETIME NOTE
 * -------------
 * The Registry passed to loadRequiredPlugins() must outlive the PluginManager
 * (declare the registry BEFORE the manager). The manager calls each plugin's
 * `shutdown` hook before unloading its library, so the plugin can remove every
 * component/system whose code lives in the library.
 */

#ifdef _WIN32
#define ENGINE_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define ENGINE_PLUGIN_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace Engine::Plugin {
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
        const char *const *dependencies; // names of plugins required first
        std::size_t dependencyCount;
        InitializeFunction initialize;   // mandatory
        ShutdownFunction shutdown;       // optional (may be nullptr)
    };

    using DescriptorFunction = const Descriptor *(*)();
} // namespace Engine::Plugin

class PluginManager
{
public:
    using WarningHandler = std::function<void(const std::string &)>;

    // strict = true : any invalid library in the directory is an error.
    // strict = false: invalid libraries are skipped with a warning.
    explicit PluginManager(bool strict = true);
    ~PluginManager();

    PluginManager(const PluginManager &) = delete;
    PluginManager &operator=(const PluginManager &) = delete;

    void setWarningHandler(WarningHandler handler);

    // Loads the plugins providing the required components/systems, plus
    // their dependencies (initialized in dependency order). Callable once.
    void loadRequiredPlugins(const std::filesystem::path &directory,
        const std::vector<std::string> &components,
        const std::vector<std::string> &systems, RType::Registry &registry);

    // Unloads one plugin. Throws if another loaded plugin depends on it.
    // Returns false if no plugin has this name.
    bool unload(const std::string &name);

    // Unloads everything, in reverse load order.
    void unloadAll();

    bool isLoaded(const std::string &name) const;

private:
    // RAII owner of a native library handle (dlopen / LoadLibrary).
    class Library
    {
    public:
        Library() = default;
        explicit Library(void *handle);
        Library(const Library &) = delete;
        Library &operator=(const Library &) = delete;
        Library(Library &&other) noexcept;
        Library &operator=(Library &&other) noexcept;
        ~Library();

        void *get() const;
        void *release();

    private:
        void reset(void *handle = nullptr);

        void *_handle = nullptr;
    };

    using NameList = std::vector<std::string>;
    using OwnerMap = std::unordered_map<std::string, std::size_t>;

    // A plugin found on disk, validated but not yet initialized.
    struct Candidate {
        Library library;
        std::string name;
        NameList components;
        NameList systems;
        NameList dependencies;
        Engine::Plugin::InitializeFunction initialize = nullptr;
        Engine::Plugin::ShutdownFunction shutdown = nullptr;
    };

    struct LoadedPlugin {
        Library library;
        std::string name;
        NameList dependencies;
        Engine::Plugin::ShutdownFunction shutdown;
    };

    enum class VisitState : char { Unvisited, Visiting, Done };

    // Platform layer
    static void closeLibrary(void *handle);
    static void *openLibrary(const std::filesystem::path &path);
    static void *findSymbol(void *handle, const char *name);
    static bool isSharedLibrary(const std::filesystem::path &path);

    // Discovery and validation
    std::vector<std::filesystem::path> listLibraries(
        const std::filesystem::path &directory) const;
    static bool isInsideDirectory(const std::filesystem::path &path,
        const std::filesystem::path &root);
    static const Engine::Plugin::Descriptor *readDescriptor(
        const Library &library, const std::filesystem::path &path);
    static bool isValidDescriptor(
        const Engine::Plugin::Descriptor *descriptor);
    static NameList copyNames(const char *const *names, std::size_t count,
        std::string_view pluginName, std::string_view kind);
    static Candidate loadCandidate(const std::filesystem::path &path);
    std::optional<Candidate> tryLoadCandidate(
        const std::filesystem::path &path) const;
    std::vector<Candidate> loadCandidates(
        const std::filesystem::path &directory) const;

    // Selection and ordering
    static void addOwners(OwnerMap &owners, const NameList &names,
        std::size_t pluginIndex, std::string_view kind,
        std::string_view pluginName);
    static OwnerMap buildOwners(const std::vector<Candidate> &candidates,
        NameList Candidate::*names, std::string_view kind);
    static OwnerMap buildNameIndex(const std::vector<Candidate> &candidates);
    static std::vector<std::size_t> findOwners(const OwnerMap &owners,
        const NameList &required, std::string_view kind);
    static void visit(const std::vector<Candidate> &candidates,
        const OwnerMap &nameIndex, std::size_t index,
        std::vector<VisitState> &state, std::vector<std::size_t> &order);
    static std::vector<std::size_t> resolveLoadOrder(
        const std::vector<Candidate> &candidates, const NameList &components,
        const NameList &systems);

    // Activation / deactivation
    void initializeInOrder(std::vector<Candidate> &candidates,
        const std::vector<std::size_t> &order, RType::Registry &registry);
    void ensureNoDependents(const std::string &name) const;
    void shutdownPlugin(LoadedPlugin &plugin) noexcept;
    void warn(const std::string &message) const;

    std::vector<LoadedPlugin> _loadedPlugins;
    RType::Registry *_registry = nullptr;
    WarningHandler _warningHandler;
    bool _strict;
    bool _configured = false;
};

#endif  

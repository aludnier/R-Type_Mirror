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

#include "ecs/Registry.h"

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
}

class PluginManager
{
public:
    explicit PluginManager();
    ~PluginManager();

    PluginManager(const PluginManager &) = delete;
    PluginManager &operator=(const PluginManager &) = delete;

    void loadRequiredPlugins(const std::filesystem::path &directory,
        const std::vector<std::string> &components,
        const std::vector<std::string> &systems, RType::Registry &registry);
    bool unload(const std::string &name);
    void unloadAll();
    bool isLoaded(const std::string &name) const;

private:
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

    using OwnerMap = std::unordered_map<std::string, std::size_t>;

    struct Candidate {
        Library library;
        std::string name;
        std::vector<std::string> components;
        std::vector<std::string> systems;
        std::vector<std::string> dependencies;
        Engine::Plugin::InitializeFunction initialize = nullptr;
        Engine::Plugin::ShutdownFunction shutdown = nullptr;
    };

    struct LoadedPlugin {
        Library library;
        std::string name;
        std::vector<std::string> dependencies;
        Engine::Plugin::ShutdownFunction shutdown;
    };

    enum class VisitState : char {
        Unvisited,
        Visiting,
        Done
    };

    static void closeLibrary(void *handle);
    static void *openLibrary(const std::filesystem::path &path);
    static void *findSymbol(void *handle, const char *name);
    static bool isSharedLibrary(const std::filesystem::path &path);

    std::vector<std::filesystem::path> listLibraries(const std::filesystem::path &directory) const;
    static const Engine::Plugin::Descriptor *readDescriptor(const Library &library, const std::filesystem::path &path);
    static bool isValidDescriptor(const Engine::Plugin::Descriptor *descriptor);
    static std::vector<std::string> copyNames(const char *const *names, std::size_t count,
        std::string_view pluginName, std::string_view kind);
    static Candidate loadCandidate(const std::filesystem::path &path);
    std::optional<Candidate> tryLoadCandidate(const std::filesystem::path &path) const;
    std::vector<Candidate> loadCandidates(const std::filesystem::path &directory) const;

    // Selection and ordering
    static void addOwners(OwnerMap &owners, const std::vector<std::string> &names, std::size_t pluginIndex, std::string_view kind,
        std::string_view pluginName);
    static OwnerMap buildOwners(const std::vector<Candidate> &candidates, std::vector<std::string> Candidate::*names, std::string_view kind);
    static OwnerMap buildNameIndex(const std::vector<Candidate> &candidates);
    static std::vector<std::size_t> findOwners(const OwnerMap &owners, const std::vector<std::string> &required, std::string_view kind);
    static std::vector<std::size_t> resolveLoadOrder(const std::vector<Candidate> &candidates, const std::vector<std::string> &components,
        const std::vector<std::string> &systems);

    void initializeInOrder(std::vector<Candidate> &candidates,
        const std::vector<std::size_t> &order, RType::Registry &registry);
    void shutdownPlugin(LoadedPlugin &plugin) noexcept;

    std::vector<LoadedPlugin> _loadedPlugins;
    RType::Registry *_registry = nullptr;
    bool _configured = false;
};

#endif  

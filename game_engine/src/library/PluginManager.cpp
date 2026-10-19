#include "PluginManager.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

using namespace Engine::Plugin;

// ---------------------------------------------------------------------------
// Library (RAII handle)
// ---------------------------------------------------------------------------

PluginManager::Library::Library(void *handle) :
    _handle(handle)
{
}

PluginManager::Library::Library(Library &&other) noexcept :
    _handle(other.release())
{
}

PluginManager::Library &PluginManager::Library::operator=(
    Library &&other) noexcept
{
    if (this != &other)
        reset(other.release());
    return *this;
}

PluginManager::Library::~Library()
{
    reset();
}

void *PluginManager::Library::get() const
{
    return _handle;
}

void *PluginManager::Library::release()
{
    return std::exchange(_handle, nullptr);
}

void PluginManager::Library::reset(void *handle)
{
    if (_handle != nullptr)
        closeLibrary(_handle);
    _handle = handle;
}

// ---------------------------------------------------------------------------
// Platform layer
// ---------------------------------------------------------------------------

void PluginManager::closeLibrary(void *handle)
{
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

void *PluginManager::openLibrary(const std::filesystem::path &path)
{
#ifdef _WIN32
    auto handle = LoadLibraryW(path.c_str());
    if (handle == nullptr)
        throw std::runtime_error("Could not load plugin library: " +
            path.string() + " (Windows error " +
            std::to_string(GetLastError()) + ")");
    return handle;
#else
    auto handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        const char *error = dlerror();
        throw std::runtime_error("Could not load plugin library: " +
            path.string() +
            (error == nullptr ? "" : " (" + std::string(error) + ")"));
    }
    return handle;
#endif
}

void *PluginManager::findSymbol(void *handle, const char *name)
{
#ifdef _WIN32
    return reinterpret_cast<void *>(
        GetProcAddress(static_cast<HMODULE>(handle), name));
#else
    return dlsym(handle, name);
#endif
}

bool PluginManager::isSharedLibrary(const std::filesystem::path &path)
{
#ifdef _WIN32
    return path.extension() == ".dll";
#else
    return path.extension() == ".so";
#endif
}

// ---------------------------------------------------------------------------
// Discovery and validation
// ---------------------------------------------------------------------------


std::vector<std::filesystem::path> PluginManager::listLibraries(const std::filesystem::path &directory) const
{
    if (!std::filesystem::is_directory(directory))
        throw std::runtime_error(
            "Plugin directory does not exist: " + directory.string());

    const auto root = std::filesystem::canonical(directory);
    std::vector<std::filesystem::path> paths;
    for (const auto &entry : std::filesystem::directory_iterator(root)) {
        if (!entry.is_regular_file() || !isSharedLibrary(entry.path()))
            continue;
        paths.push_back(entry.path());
    }
    std::sort(paths.begin(), paths.end());
    return paths;
}

bool PluginManager::isValidDescriptor(const Descriptor *descriptor)
{
    return descriptor != nullptr && descriptor->abiVersion == ABI_VERSION &&
        descriptor->name != nullptr && descriptor->name[0] != '\0' &&
        descriptor->initialize != nullptr;
}

const Descriptor *PluginManager::readDescriptor(
    const Library &library, const std::filesystem::path &path)
{
    auto descriptorFunction = reinterpret_cast<DescriptorFunction>(
        findSymbol(library.get(), DESCRIPTOR_SYMBOL));
    if (descriptorFunction == nullptr)
        throw std::runtime_error("Plugin library is missing " +
            std::string(DESCRIPTOR_SYMBOL) + ": " + path.string());

    const Descriptor *descriptor = descriptorFunction();
    if (!isValidDescriptor(descriptor))
        throw std::runtime_error(
            "Plugin has an invalid descriptor or incompatible ABI: " +
            path.string());
    return descriptor;
}

PluginManager::Candidate PluginManager::loadCandidate(const std::filesystem::path &path)
{
    Library library(openLibrary(path));
    const Descriptor *descriptor = readDescriptor(library, path);

    Candidate candidate;
    candidate.name = descriptor->name;
    candidate.components = copyNames(descriptor->components,
        descriptor->componentCount, candidate.name, "component");
    candidate.systems = copyNames(descriptor->systems,
        descriptor->systemCount, candidate.name, "system");
    candidate.dependencies = copyNames(descriptor->dependencies,
        descriptor->dependencyCount, candidate.name, "dependency");
    candidate.initialize = descriptor->initialize;
    candidate.shutdown = descriptor->shutdown;
    candidate.library = std::move(library);
    return candidate;
}

std::optional<PluginManager::Candidate> PluginManager::tryLoadCandidate(const std::filesystem::path &path) const
{
    try {
        return loadCandidate(path);
    } catch (const std::runtime_error &error) {
        throw;
    }
}

std::vector<PluginManager::Candidate> PluginManager::loadCandidates(const std::filesystem::path &directory) const
{
    const auto paths = listLibraries(directory);
    std::vector<Candidate> candidates;
    std::unordered_set<std::string> pluginNames;

    candidates.reserve(paths.size());
    for (const auto &path : paths) {
        auto candidate = tryLoadCandidate(path);
        if (!candidate)
            continue;
        if (!pluginNames.insert(candidate->name).second)
            throw std::runtime_error(
                "Duplicate plugin name: " + candidate->name);
        candidates.push_back(std::move(*candidate));
    }
    return candidates;
}

// ---------------------------------------------------------------------------
// Selection and ordering
// ---------------------------------------------------------------------------

void PluginManager::addOwners(OwnerMap &owners, const std::vector<std::string> &names,
    std::size_t pluginIndex, std::string_view kind,
    std::string_view pluginName)
{
    for (const auto &name : names) {
        if (!owners.emplace(name, pluginIndex).second)
            throw std::runtime_error("Plugin '" + std::string(pluginName) +
                "' and another plugin both provide " + std::string(kind) +
                " '" + name + "'");
    }
}

PluginManager::OwnerMap PluginManager::buildOwners(const std::vector<Candidate> &candidates, std::vector<std::string> Candidate::*names,
    std::string_view kind)
{
    OwnerMap owners;

    for (std::size_t i = 0; i < candidates.size(); ++i)
        addOwners(owners, candidates[i].*names, i, kind, candidates[i].name);
    return owners;
}

PluginManager::OwnerMap PluginManager::buildNameIndex(const std::vector<Candidate> &candidates)
{
    OwnerMap index;

    for (std::size_t i = 0; i < candidates.size(); ++i)
        index.emplace(candidates[i].name, i);
    return index;
}

std::vector<std::size_t> PluginManager::findOwners(const OwnerMap &owners,
    const std::vector<std::string> &required, std::string_view kind)
{
    std::vector<std::size_t> result;

    for (const auto &name : required) {
        auto owner = owners.find(name);
        if (owner == owners.end())
            throw std::runtime_error("No plugin provides required " +
                std::string(kind) + " '" + name + "'");
        result.push_back(owner->second);
    }
    return result;
}

std::vector<std::size_t> PluginManager::resolveLoadOrder(const std::vector<Candidate> &candidates,
    const std::vector<std::string> &components, const std::vector<std::string> &systems)
{
    const auto nameIndex = buildNameIndex(candidates);
    auto roots = findOwners(
        buildOwners(candidates, &Candidate::components, "component"),
        components, "component");
    const auto systemRoots = findOwners(
        buildOwners(candidates, &Candidate::systems, "system"), systems,
        "system");
    std::vector<VisitState> state(candidates.size(), VisitState::Unvisited);
    std::vector<std::size_t> order;

    roots.insert(roots.end(), systemRoots.begin(), systemRoots.end());
    return order;
}

// ---------------------------------------------------------------------------
// Activation / deactivation
// ---------------------------------------------------------------------------

void PluginManager::initializeInOrder(std::vector<Candidate> &candidates,
    const std::vector<std::size_t> &order, RType::Registry &registry)
{
    _registry = &registry;
    const auto firstNewPlugin = _loadedPlugins.size();
    try {
        for (const auto index : order) {
            auto &candidate = candidates[index];
            const auto initialize = candidate.initialize;
            _loadedPlugins.push_back({std::move(candidate.library),
                candidate.name,
                candidate.dependencies,
                candidate.shutdown});
            initialize(&registry);
        }
    } catch (...) {
        while (_loadedPlugins.size() > firstNewPlugin) {
            shutdownPlugin(_loadedPlugins.back());
            _loadedPlugins.pop_back();
        }
        if (_loadedPlugins.empty())
            _registry = nullptr;
        throw;
    }
}

void PluginManager::shutdownPlugin(LoadedPlugin &plugin) noexcept
{
    if (plugin.shutdown == nullptr || _registry == nullptr)
        return;
    try {
        plugin.shutdown(_registry);
    } catch (const std::exception &error) {
        throw ;
    }
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

PluginManager::PluginManager()
{
}

PluginManager::~PluginManager()
{
    unloadAll();
}

bool PluginManager::isLoaded(const std::string &name) const
{
    for (auto &loaded : _loadedPlugins) {
        if (loaded.name == name) {
            return true;
        }
    }
    return false;
}

bool PluginManager::unload(const std::string &name)
{
    auto found = std::find_if(_loadedPlugins.begin(),
        _loadedPlugins.end(),
        [&name](const LoadedPlugin &plugin) { return plugin.name == name; });

    if (found == _loadedPlugins.end())
        return false;
    shutdownPlugin(*found);
    _loadedPlugins.erase(found);
    return true;
}

void PluginManager::unloadAll()
{
    while (!_loadedPlugins.empty()) {
        shutdownPlugin(_loadedPlugins.back());
        _loadedPlugins.pop_back();
    }
}

void PluginManager::loadRequiredPlugins(const std::filesystem::path &directory, const std::vector<std::string> &components,
    const std::vector<std::string> &systems, RType::Registry &registry)
{
    if (_configured)
        throw std::logic_error(
            "Plugin requirements have already been configured");
    _configured = true;

    if (components.empty() && systems.empty())
        return;

    auto candidates = loadCandidates(directory);
    const auto order = resolveLoadOrder(candidates, components, systems);
    initializeInOrder(candidates, order, registry);
}

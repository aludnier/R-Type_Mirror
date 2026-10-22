#include "PluginManager.h"
#include "Module.h"

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
    HMODULE pinned = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN |
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        reinterpret_cast<LPCWSTR>(handle), &pinned);
    return handle;
#else
    auto handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
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

std::vector<std::string> PluginManager::copyNames(const char *const *names,
    std::size_t count, std::string_view pluginName, std::string_view kind)
{
    if (count == 0)
        return {};
    if (names == nullptr)
        throw std::runtime_error("Plugin '" + std::string(pluginName) +
            "' has a null " + std::string(kind) + " list");

    std::vector<std::string> result;
    result.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        if (names[i] == nullptr || names[i][0] == '\0')
            throw std::runtime_error("Plugin '" + std::string(pluginName) +
                "' has an invalid " + std::string(kind) + " name");
        result.emplace_back(names[i]);
    }
    return result;
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
    return loadCandidate(path);
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

    const auto visit = [&](const auto &self, std::size_t index) -> void {
        if (state[index] == VisitState::Done)
            return;
        if (state[index] == VisitState::Visiting)
            throw std::runtime_error(
                "Plugin dependency cycle detected at '" +
                candidates[index].name + "'");

        state[index] = VisitState::Visiting;
        for (const auto &dependency : candidates[index].dependencies) {
            const auto found = nameIndex.find(dependency);
            if (found == nameIndex.end())
                throw std::runtime_error("Plugin '" + candidates[index].name +
                    "' requires missing dependency '" + dependency + "'");
            self(self, found->second);
        }
        state[index] = VisitState::Done;
        order.push_back(index);
    };

    for (const auto root : roots)
        visit(visit, root);

    return order;
}

void PluginManager::initializeInOrder(std::vector<Candidate> &candidates,
    const std::vector<std::size_t> &order, RType::Registry &registry)
{
    _loadedPlugins.reserve(_loadedPlugins.size() + order.size());
    _retiredLibraries.reserve(_retiredLibraries.size() + order.size());

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
            auto &plugin = _loadedPlugins.back();
            shutdownPlugin(plugin);
            // Do not dlclose here: the exception being propagated (and any
            // leftover registry callback) may still reference this code.
            _retiredLibraries.push_back(std::move(plugin.library));
            _loadedPlugins.pop_back();
        }
        if (_loadedPlugins.empty() && _loadedModules.empty())
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
        std::cerr << "Plugin '" << plugin.name
                  << "' threw during shutdown: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "Plugin '" << plugin.name
                  << "' threw during shutdown\n";
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
    while (!_loadedPlugins.empty()) {
        shutdownPlugin(_loadedPlugins.back());
        _loadedPlugins.pop_back();
    }
    while (!_loadedModules.empty())
        _loadedModules.pop_back();
    while (!_retiredLibraries.empty())
        _retiredLibraries.pop_back();
}

bool PluginManager::isLoaded(const std::string &name) const
{
    for (const auto &loaded : _loadedPlugins) {
        if (loaded.name == name) {
            return true;
        }
    }
    for (const auto &loaded : _loadedModules) {
        if (loaded.name == name)
            return true;
    }
    return false;
}

bool PluginManager::unload(const std::string &name)
{
    const auto module = std::find_if(_loadedModules.begin(),
        _loadedModules.end(),
        [&name](const LoadedModule &loaded) { return loaded.name == name; });
    if (module != _loadedModules.end())
        throw std::logic_error(
            "Cannot unload module '" + name +
            "' because its descriptor has no shutdown callback");

    auto found = std::find_if(_loadedPlugins.begin(),
        _loadedPlugins.end(),
        [&name](const LoadedPlugin &plugin) { return plugin.name == name; });

    if (found == _loadedPlugins.end())
        return false;
    const auto dependent = std::find_if(_loadedPlugins.begin(),
        _loadedPlugins.end(),
        [&name](const LoadedPlugin &plugin) {
            return plugin.name != name &&
                std::find(plugin.dependencies.begin(), plugin.dependencies.end(),
                    name) != plugin.dependencies.end();
        });
    if (dependent != _loadedPlugins.end())
        throw std::runtime_error("Cannot unload plugin '" + name +
            "' because plugin '" + dependent->name + "' depends on it");

    _retiredLibraries.reserve(_retiredLibraries.size() + 1);
    shutdownPlugin(*found);
    _retiredLibraries.push_back(std::move(found->library));
    _loadedPlugins.erase(found);
    return true;
}

void PluginManager::unloadAll()
{
    if (!_loadedModules.empty())
        throw std::logic_error(
            "Cannot unload modules without shutdown callbacks");
    _retiredLibraries.reserve(_retiredLibraries.size() + _loadedPlugins.size());
    while (!_loadedPlugins.empty()) {
        shutdownPlugin(_loadedPlugins.back());
        _retiredLibraries.push_back(std::move(_loadedPlugins.back().library));
        _loadedPlugins.pop_back();
    }
}

void PluginManager::loadModule(const std::filesystem::path &path,
    RType::Registry &registry)
{
    if (_registry != nullptr && _registry != &registry)
        throw std::logic_error(
            "PluginManager cannot load into multiple registries");

    const auto rootPath = std::filesystem::canonical(path);
    const auto paths = listLibraries(rootPath.parent_path());
    std::vector<ModuleCandidate> candidates;
    std::unordered_map<std::string, std::size_t> nameIndex;
    std::optional<std::size_t> rootIndex;

    using ModuleEntryPoint = const RType::ModuleDescriptor *(*)();
    for (const auto &candidatePath : paths) {
        Library library(openLibrary(candidatePath));
        auto entryPoint = reinterpret_cast<ModuleEntryPoint>(
            findSymbol(library.get(), "RTypeLibEntryPoint"));
        if (entryPoint == nullptr)
            continue;

        const auto *descriptor = entryPoint();
        if (descriptor == nullptr || descriptor->name.empty() ||
            descriptor->apiVersion.major != RType::MODULE_API_MAJOR ||
            !descriptor->registerModule)
            throw std::runtime_error(
                "Module has an invalid descriptor or incompatible API: " +
                candidatePath.string());

        ModuleCandidate candidate;
        candidate.name = descriptor->name;
        candidate.apiVersion = descriptor->apiVersion;
        candidate.dependencies = descriptor->dependencies;
        candidate.registerModule = descriptor->registerModule;
        candidate.library = std::move(library);

        if (!nameIndex.emplace(candidate.name, candidates.size()).second)
            throw std::runtime_error(
                "Duplicate module name: " + candidate.name);
        if (std::filesystem::canonical(candidatePath) == rootPath)
            rootIndex = candidates.size();
        candidates.push_back(std::move(candidate));
    }

    if (!rootIndex)
        throw std::runtime_error(
            "Module is missing RTypeLibEntryPoint: " + path.string());
    if (isLoaded(candidates[*rootIndex].name))
        throw std::logic_error(
            "Module is already loaded: " + candidates[*rootIndex].name);

    const auto isApiCompatible = [](RType::ModuleApiVersion actual,
                                     RType::ModuleApiVersion required) {
        return actual.major == required.major &&
            actual.minor >= required.minor;
    };
    std::unordered_map<std::string, RType::ModuleApiVersion> loadedVersions;
    for (const auto &module : _loadedModules)
        loadedVersions.emplace(module.name, module.apiVersion);

    std::vector<VisitState> state(candidates.size(), VisitState::Unvisited);
    std::vector<std::size_t> order;
    const auto visit = [&](const auto &self, std::size_t index) -> void {
        auto &candidate = candidates[index];
        if (state[index] == VisitState::Done)
            return;
        if (state[index] == VisitState::Visiting)
            throw std::runtime_error(
                "Module dependency cycle detected at '" + candidate.name +
                "'");

        state[index] = VisitState::Visiting;
        for (const auto &dependency : candidate.dependencies) {
            if (const auto loaded = loadedVersions.find(dependency.name);
                loaded != loadedVersions.end()) {
                if (!isApiCompatible(loaded->second, dependency.compatible_api))
                    throw std::runtime_error(
                        "Module '" + candidate.name +
                        "' requires an incompatible version of dependency '" +
                        dependency.name + "'");
                continue;
            }

            const auto found = nameIndex.find(dependency.name);
            if (found == nameIndex.end())
                throw std::runtime_error("Module '" + candidate.name +
                    "' requires missing dependency '" + dependency.name +
                    "'");
            const auto &dependencyCandidate = candidates[found->second];
            if (!isApiCompatible(dependencyCandidate.apiVersion,
                    dependency.compatible_api))
                throw std::runtime_error("Module '" + candidate.name +
                    "' requires an incompatible version of dependency '" +
                    dependency.name + "'");
            self(self, found->second);
        }
        state[index] = VisitState::Done;
        order.push_back(index);
    };
    visit(visit, *rootIndex);

    _loadedModules.reserve(_loadedModules.size() + order.size());
    _retiredLibraries.reserve(_retiredLibraries.size() + 1);

    _registry = &registry;
    for (const auto index : order) {
        auto &candidate = candidates[index];
        _loadedModules.push_back({
            std::move(candidate.library),
            candidate.name,
            candidate.apiVersion,
            candidate.dependencies,
            std::move(candidate.registerModule)
        });
        auto &module = _loadedModules.back();
        const auto status = module.registerModule(registry);
        if (status != RType::ModuleStatus::Success) {
            const auto failedName = module.name;
            _retiredLibraries.push_back(std::move(module.library));
            _loadedModules.pop_back();
            if (_loadedPlugins.empty() && _loadedModules.empty())
                _registry = nullptr;
            throw std::runtime_error(
                "Module '" + failedName + "' failed to initialize");
        }
        loadedVersions.emplace(module.name, module.apiVersion);
    }
}

void PluginManager::loadRequiredPlugins(const std::filesystem::path &directory, const std::vector<std::string> &components,
    const std::vector<std::string> &systems, RType::Registry &registry)
{
    if (_configured)
        throw std::logic_error(
            "Plugin requirements have already been configured");
    if (_registry != nullptr && _registry != &registry)
        throw std::logic_error(
            "PluginManager cannot load into multiple registries");
    _configured = true;

    if (components.empty() && systems.empty())
        return;

    auto candidates = loadCandidates(directory);
    const auto order = resolveLoadOrder(candidates, components, systems);
    initializeInOrder(candidates, order, registry);
}

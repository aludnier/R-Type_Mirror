#include "PluginManager.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {
using ModuleEntryPoint = const RType::ModuleDescriptor *(*) ();

bool isApiCompatible(
    RType::ModuleApiVersion actual, RType::ModuleApiVersion required)
{
    return actual.major == required.major && actual.minor >= required.minor;
}

void requireCompatible(const std::string &moduleName,
    const RType::ModuleDependency &dependency, RType::ModuleApiVersion actual)
{
    if (!isApiCompatible(actual, dependency.compatible_api))
        throw std::runtime_error("Module '" + moduleName +
            "' requires an incompatible version of dependency '" +
            dependency.name + "'");
}

void validateDescriptor(const RType::ModuleDescriptor *descriptor,
    const std::filesystem::path &path)
{
    if (descriptor == nullptr || descriptor->name.empty() ||
        descriptor->apiVersion.major != RType::MODULE_API_MAJOR ||
        !descriptor->registerModule)
        throw std::runtime_error(
            "Module has an invalid descriptor or incompatible API: " +
            path.string());
}
} // namespace

RType::PluginManager::Loader RType::PluginManager::openLibrary(
    const std::filesystem::path &path)
{
    Loader library;

    try {
        library.loadFile(path.string(), true);
    } catch (const Loader::LibraryLoadException &error) {
        throw std::runtime_error("Could not load module library: " +
            path.string() + " (" + error.what() + ")");
    }
    return library;
}

bool RType::PluginManager::isSharedLibrary(const std::filesystem::path &path)
{
#ifdef _WIN32
    return path.extension() == ".dll";
#else
    return path.extension() == ".so";
#endif
}

std::vector<std::filesystem::path> RType::PluginManager::listLibraries(
    const std::filesystem::path &directory)
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

RType::PluginManager::Scan RType::PluginManager::scanDirectory(
    const std::filesystem::path &directory)
{
    Scan scan;

    for (const auto &path : listLibraries(directory)) {
        auto library = openLibrary(path);
        const auto entryPoint =
            library.getFunction<ModuleEntryPoint>("RTypeLibEntryPoint");
        if (entryPoint == nullptr)
            continue;

        const auto *descriptor = entryPoint();
        validateDescriptor(descriptor, path);
        if (!scan.index.emplace(descriptor->name, scan.modules.size()).second)
            throw std::runtime_error(
                "Duplicate module name: " + descriptor->name);

        Module module;
        module.name = descriptor->name;
        module.apiVersion = descriptor->apiVersion;
        module.dependencies = descriptor->dependencies;
        module.registerModule = descriptor->registerModule;
        module.library = std::move(library);

        scan.paths.push_back(std::filesystem::canonical(path));
        scan.modules.push_back(std::move(module));
    }
    return scan;
}

std::vector<std::size_t> RType::PluginManager::resolveLoadOrder(
    const std::vector<Module> &candidates, const NameIndex &nameIndex,
    const std::vector<std::size_t> &roots) const
{
    std::unordered_map<std::string, RType::ModuleApiVersion> loadedVersions;
    for (const auto &module : _loadedModules)
        loadedVersions.emplace(module.name, module.apiVersion);

    std::vector<VisitState> state(candidates.size(), VisitState::Unvisited);
    std::vector<std::size_t> order;

    const auto visit = [&](const auto &self, std::size_t index) -> void {
        const auto &candidate = candidates[index];

        if (state[index] == VisitState::Done)
            return;
        if (state[index] == VisitState::Visiting)
            throw std::runtime_error(
                "Module dependency cycle detected at '" + candidate.name + "'");
        state[index] = VisitState::Visiting;
        for (const auto &dependency : candidate.dependencies) {
            if (const auto loaded = loadedVersions.find(dependency.name);
                loaded != loadedVersions.end()) {
                requireCompatible(candidate.name, dependency, loaded->second);
                continue;
            }
            const auto found = nameIndex.find(dependency.name);
            if (found == nameIndex.end())
                throw std::runtime_error("Module '" + candidate.name +
                    "' requires missing dependency '" + dependency.name + "'");
            requireCompatible(candidate.name,
                dependency,
                candidates[found->second].apiVersion);
            self(self, found->second);
        }
        state[index] = VisitState::Done;
        order.push_back(index);
    };

    for (const auto root : roots)
        visit(visit, root);
    return order;
}

void RType::PluginManager::rollback(std::size_t firstNew) noexcept
{
    while (_loadedModules.size() > firstNew) {
        _retiredLibraries.push_back(std::move(_loadedModules.back().library));
        _loadedModules.pop_back();
    }
    if (_loadedModules.empty())
        _registry.reset();
}

void RType::PluginManager::registerModules(std::vector<Module> &candidates,
    const std::vector<std::size_t> &order, RType::Registry &registry)
{
    _loadedModules.reserve(_loadedModules.size() + order.size());
    _retiredLibraries.reserve(_retiredLibraries.size() + order.size());

    _registry = std::ref(registry);
    const auto firstNew = _loadedModules.size();
    try {
        for (const auto index : order) {
            _loadedModules.push_back(std::move(candidates[index]));
            auto &module = _loadedModules.back();
            if (module.registerModule(registry) != RType::ModuleStatus::Success)
                throw std::runtime_error(
                    "Module '" + module.name + "' failed to initialize");
        }
    } catch (...) {
        rollback(firstNew);
        throw;
    }
}

bool RType::PluginManager::isLoaded(const std::string &name) const
{
    return std::ranges::any_of(_loadedModules,
        [&name](const auto &module) { return module.name == name; });
}

void RType::PluginManager::loadRequiredModules(
    const std::filesystem::path &directory,
    const std::vector<std::string> &moduleNames, RType::Registry &registry)
{
    if (_configured)
        throw std::logic_error(
            "Plugin requirements have already been configured");
    if (_registry && &_registry->get() != &registry)
        throw std::logic_error(
            "PluginManager cannot load into multiple registries");
    _configured = true;

    if (moduleNames.empty())
        return;

    auto scan = scanDirectory(directory);
    std::vector<std::size_t> roots;
    for (const auto &name : moduleNames) {
        const auto found = scan.index.find(name);
        if (found == scan.index.end())
            throw std::runtime_error("No module named '" + name +
                "' found in " + directory.string());
        if (!isLoaded(name))
            roots.push_back(found->second);
    }
    registerModules(scan.modules,
        resolveLoadOrder(scan.modules, scan.index, roots),
        registry);
}

void RType::PluginManager::loadModule(
    const std::filesystem::path &path, RType::Registry &registry)
{
    if (_registry && &_registry->get() != &registry)
        throw std::logic_error(
            "PluginManager cannot load into multiple registries");

    const auto rootPath = std::filesystem::canonical(path);
    auto scan = scanDirectory(rootPath.parent_path());

    const auto rootIt =
        std::find(scan.paths.begin(), scan.paths.end(), rootPath);
    if (rootIt == scan.paths.end())
        throw std::runtime_error(
            "Module is missing RTypeLibEntryPoint: " + path.string());
    const auto rootIndex =
        static_cast<std::size_t>(rootIt - scan.paths.begin());
    if (isLoaded(scan.modules[rootIndex].name))
        throw std::logic_error(
            "Module is already loaded: " + scan.modules[rootIndex].name);

    registerModules(scan.modules,
        resolveLoadOrder(scan.modules, scan.index, {rootIndex}),
        registry);
}

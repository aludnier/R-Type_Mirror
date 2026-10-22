#include "PluginManager.h"

#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace
{
bool isApiCompatible(RType::ModuleApiVersion actual,
    RType::ModuleApiVersion required)
{
    return actual.major == required.major &&
        actual.minor >= required.minor;
}
}

std::vector<RType::PluginManager::ModuleCandidate>
RType::PluginManager::loadModuleCandidates(
    const std::filesystem::path &rootPath, OwnerMap &nameIndex,
    std::optional<std::size_t> &rootIndex) const
{
    const auto paths = listLibraries(rootPath.parent_path());
    std::vector<ModuleCandidate> candidates;

    using ModuleEntryPoint = const RType::ModuleDescriptor *(*)();
    for (const auto &candidatePath : paths) {
        Library library(openLibrary(candidatePath));
        const auto entryPoint = reinterpret_cast<ModuleEntryPoint>(
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

    return candidates;
}

std::vector<std::size_t> RType::PluginManager::resolveModuleLoadOrder(
    const std::vector<ModuleCandidate> &candidates,
    const OwnerMap &nameIndex, std::size_t rootIndex) const
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
    visit(visit, rootIndex);
    return order;
}

void RType::PluginManager::registerModules(
    std::vector<ModuleCandidate> &candidates,
    const std::vector<std::size_t> &order, RType::Registry &registry)
{
    _loadedModules.reserve(_loadedModules.size() + order.size());
    _retiredLibraries.reserve(_retiredLibraries.size() + 1);

    _registry = std::ref(registry);
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
                _registry.reset();
            throw std::runtime_error(
                "Module '" + failedName + "' failed to initialize");
        }
    }
}

void RType::PluginManager::loadModule(const std::filesystem::path &path,
    RType::Registry &registry)
{
    if (_registry && &_registry->get() != &registry)
        throw std::logic_error(
            "PluginManager cannot load into multiple registries");

    const auto rootPath = std::filesystem::canonical(path);
    OwnerMap nameIndex;
    std::optional<std::size_t> rootIndex;
    auto candidates = loadModuleCandidates(rootPath, nameIndex, rootIndex);

    if (!rootIndex)
        throw std::runtime_error(
            "Module is missing RTypeLibEntryPoint: " + path.string());
    if (isLoaded(candidates[*rootIndex].name))
        throw std::logic_error(
            "Module is already loaded: " + candidates[*rootIndex].name);

    const auto order = resolveModuleLoadOrder(candidates, nameIndex, *rootIndex);
    registerModules(candidates, order, registry);
}

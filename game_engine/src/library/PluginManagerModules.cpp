#include "PluginManager.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using NameIndex = std::unordered_map<std::string, std::size_t>;
using VersionMap = std::unordered_map<std::string, RType::ModuleApiVersion>;
using ModuleEntryPoint = const RType::ModuleDescriptor *(*)();

bool isApiCompatible(RType::ModuleApiVersion actual,
    RType::ModuleApiVersion required)
{
    return actual.major == required.major &&
        actual.minor >= required.minor;
}

void requireCompatible(const std::string &moduleName,
    const RType::ModuleDependency &dependency,
    RType::ModuleApiVersion actual)
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

template <typename Candidate, typename Library>
Candidate makeCandidate(const RType::ModuleDescriptor &descriptor,
    Library &&library)
{
    Candidate candidate;

    candidate.name = descriptor.name;
    candidate.apiVersion = descriptor.apiVersion;
    candidate.dependencies = descriptor.dependencies;
    candidate.registerModule = descriptor.registerModule;
    candidate.library = std::forward<Library>(library);
    return candidate;
}

void indexName(NameIndex &nameIndex, const std::string &name,
    std::size_t index)
{
    if (!nameIndex.emplace(name, index).second)
        throw std::runtime_error("Duplicate module name: " + name);
}

template <typename Modules>
VersionMap collectVersions(const Modules &modules)
{
    VersionMap versions;

    for (const auto &module : modules)
        versions.emplace(module.name, module.apiVersion);
    return versions;
}

template <typename Loaded, typename Candidate>
Loaded toLoadedModule(Candidate &candidate)
{
    return Loaded{
        std::move(candidate.library),
        candidate.name,
        candidate.apiVersion,
        candidate.dependencies,
        std::move(candidate.registerModule)
    };
}

template <typename Candidate>
std::optional<std::size_t> resolveDependency(const Candidate &candidate,
    const RType::ModuleDependency &dependency,
    const std::vector<Candidate> &candidates, const NameIndex &nameIndex,
    const VersionMap &loadedVersions)
{
    if (const auto loaded = loadedVersions.find(dependency.name);
        loaded != loadedVersions.end()) {
        requireCompatible(candidate.name, dependency, loaded->second);
        return std::nullopt;
    }

    const auto found = nameIndex.find(dependency.name);
    if (found == nameIndex.end())
        throw std::runtime_error("Module '" + candidate.name +
            "' requires missing dependency '" + dependency.name + "'");
    requireCompatible(candidate.name, dependency,
        candidates[found->second].apiVersion);
    return found->second;
}
}

std::vector<RType::PluginManager::ModuleCandidate>
RType::PluginManager::loadModuleCandidates(
    const std::filesystem::path &rootPath, OwnerMap &nameIndex,
    std::optional<std::size_t> &rootIndex)
{
    const auto paths = listLibraries(rootPath.parent_path());
    std::vector<ModuleCandidate> candidates;

    for (const auto &candidatePath : paths) {
        Library library(openLibrary(candidatePath));
        const auto entryPoint = reinterpret_cast<ModuleEntryPoint>(
            findSymbol(library.get(), "RTypeLibEntryPoint"));
        if (entryPoint == nullptr)
            continue;

        const auto *descriptor = entryPoint();
        validateDescriptor(descriptor, candidatePath);
        auto candidate = makeCandidate<ModuleCandidate>(
            *descriptor, std::move(library));

        indexName(nameIndex, candidate.name, candidates.size());
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
    const auto loadedVersions = collectVersions(_loadedModules);
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
            const auto next = resolveDependency(
                candidate, dependency, candidates, nameIndex, loadedVersions);
            if (next)
                self(self, *next);
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
        _loadedModules.push_back(
            toLoadedModule<LoadedModule>(candidates[index]));
        auto &module = _loadedModules.back();
        if (module.registerModule(registry) == RType::ModuleStatus::Success)
            continue;

        const auto failedName = module.name;
        _retiredLibraries.push_back(std::move(module.library));
        _loadedModules.pop_back();
        if (_loadedPlugins.empty() && _loadedModules.empty())
            _registry.reset();
        throw std::runtime_error(
            "Module '" + failedName + "' failed to initialize");
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

    const auto order = resolveModuleLoadOrder(
        candidates, nameIndex, *rootIndex);
    registerModules(candidates, order, registry);
}

#include "PluginManager.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

void RType::PluginManager::addOwners(OwnerMap &owners, const std::vector<std::string> &names,
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

RType::PluginManager::OwnerMap RType::PluginManager::buildOwners(const std::vector<Candidate> &candidates, std::vector<std::string> Candidate::*names,
    std::string_view kind)
{
    OwnerMap owners;

    for (std::size_t i = 0; i < candidates.size(); ++i)
        addOwners(owners, candidates[i].*names, i, kind, candidates[i].name);
    return owners;
}

RType::PluginManager::OwnerMap RType::PluginManager::buildNameIndex(const std::vector<Candidate> &candidates)
{
    OwnerMap index;

    for (std::size_t i = 0; i < candidates.size(); ++i)
        index.emplace(candidates[i].name, i);
    return index;
}

std::vector<std::size_t> RType::PluginManager::findOwners(const OwnerMap &owners,
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

std::vector<std::size_t> RType::PluginManager::resolveLoadOrder(const std::vector<Candidate> &candidates,
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

void RType::PluginManager::initializeInOrder(std::vector<Candidate> &candidates,
    const std::vector<std::size_t> &order, RType::Registry &registry)
{
    _loadedPlugins.reserve(_loadedPlugins.size() + order.size());
    _retiredLibraries.reserve(_retiredLibraries.size() + order.size());

    _registry = std::ref(registry);
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
            _retiredLibraries.push_back(std::move(plugin.library));
            _loadedPlugins.pop_back();
        }
        if (_loadedPlugins.empty() && _loadedModules.empty())
            _registry.reset();
        throw;
    }
}

void RType::PluginManager::shutdownPlugin(LoadedPlugin &plugin) noexcept
{
    if (plugin.shutdown == nullptr || !_registry)
        return;
    try {
        plugin.shutdown(&_registry->get());
    } catch (const std::exception &error) {
        std::cerr << "Plugin '" << plugin.name
                  << "' threw during shutdown: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "Plugin '" << plugin.name
                  << "' threw during shutdown\n";
    }
}

RType::PluginManager::PluginManager()
{
}

RType::PluginManager::~PluginManager()
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

bool RType::PluginManager::isLoaded(const std::string &name) const
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

bool RType::PluginManager::unload(const std::string &name)
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

void RType::PluginManager::unloadAll()
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

void RType::PluginManager::loadRequiredPlugins(const std::filesystem::path &directory, const std::vector<std::string> &components,
    const std::vector<std::string> &systems, RType::Registry &registry)
{
    if (_configured)
        throw std::logic_error(
            "Plugin requirements have already been configured");
    if (_registry && &_registry->get() != &registry)
        throw std::logic_error(
            "PluginManager cannot load into multiple registries");
    _configured = true;

    if (components.empty() && systems.empty())
        return;

    auto candidates = loadCandidates(directory);
    const auto order = resolveLoadOrder(candidates, components, systems);
    initializeInOrder(candidates, order, registry);
}

#include "PluginManager.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <utility>

RType::PluginManager::Loader RType::PluginManager::openLibrary(
    const std::filesystem::path &path)
{
    Loader library;

    try {
        // Persistent: the code must stay mapped even after the Loader is
        // destroyed, because the registry may still reference it.
        library.loadFile(path.string(), true);
    } catch (const Loader::LibraryLoadException &error) {
        throw std::runtime_error("Could not load plugin library: " +
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

bool RType::PluginManager::isValidDescriptor(const RType::Plugin::Descriptor *descriptor)
{
    return descriptor != nullptr && descriptor->abiVersion == RType::Plugin::ABI_VERSION &&
        descriptor->name != nullptr && descriptor->name[0] != '\0' &&
        descriptor->initialize != nullptr;
}

const RType::Plugin::Descriptor *RType::PluginManager::readDescriptor(
    const Loader &library, const std::filesystem::path &path)
{
    const auto descriptorFunction =
        library.getFunction<RType::Plugin::DescriptorFunction>(
            RType::Plugin::DESCRIPTOR_SYMBOL);
    if (descriptorFunction == nullptr)
        throw std::runtime_error("Plugin library is missing " +
            std::string(RType::Plugin::DESCRIPTOR_SYMBOL) + ": " + path.string());

    const RType::Plugin::Descriptor *descriptor = descriptorFunction();
    if (!isValidDescriptor(descriptor))
        throw std::runtime_error(
            "Plugin has an invalid descriptor or incompatible ABI: " +
            path.string());
    return descriptor;
}

std::vector<std::string> RType::PluginManager::copyNames(const char *const *names,
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

RType::PluginManager::Candidate RType::PluginManager::loadCandidate(
    const std::filesystem::path &path)
{
    auto library = openLibrary(path);
    const RType::Plugin::Descriptor *descriptor = readDescriptor(library, path);

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

std::optional<RType::PluginManager::Candidate> RType::PluginManager::tryLoadCandidate(
    const std::filesystem::path &path)
{
    return loadCandidate(path);
}

std::vector<RType::PluginManager::Candidate> RType::PluginManager::loadCandidates(
    const std::filesystem::path &directory)
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

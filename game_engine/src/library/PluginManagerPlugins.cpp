#include "PluginManager.h"

#include <algorithm>
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

std::vector<std::filesystem::path> PluginManager::listLibraries(
    const std::filesystem::path &directory) const
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
    const Loader &library, const std::filesystem::path &path)
{
    const auto descriptorFunction = reinterpret_cast<DescriptorFunction>(
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

PluginManager::Candidate PluginManager::loadCandidate(
    const std::filesystem::path &path)
{
    Loader library(openLibrary(path));
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

std::optional<PluginManager::Candidate> PluginManager::tryLoadCandidate(
    const std::filesystem::path &path) const
{
    return loadCandidate(path);
}

std::vector<PluginManager::Candidate> PluginManager::loadCandidates(
    const std::filesystem::path &directory) const
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

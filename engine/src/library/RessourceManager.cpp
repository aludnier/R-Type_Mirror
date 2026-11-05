/*
** EPITECH PROJECT, 2026
** RType
** File description:
** resource manager
*/

#include <map>
#include <stdexcept>
#include "NlohmanReader.h"
#include "RessourceManager.h"

namespace {
struct AssetConfig {
    std::string path;
    ResourceManager::AssetType type = ResourceManager::AssetType::Texture;
};

struct Config {
    std::map<std::string, AssetConfig> assets;
};

void from_json(const nlohmann::json &j, AssetConfig &a)
{
    using T = ResourceManager::AssetType;
    static const std::map<std::string, T> types = {{"Texture", T::Texture},
        {"Sound", T::Sound},
        {"Music", T::Music},
        {"Font", T::Font}};

    j.at("path").get_to(a.path);
    const auto name = j.value("type", std::string("Texture"));
    const auto it = types.find(name);
    if (it == types.end())
        throw std::invalid_argument("unknown asset type '" + name + "'");
    a.type = it->second;
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, assets)

} // namespace

ResourceManager::ResourceManager(const std::filesystem::path &configFile)
{
    loadConfigFile(configFile);
}

void ResourceManager::loadConfigFile(const std::filesystem::path &configFile)
{
    if (!_reader.readFile(configFile)) {
        throw std::runtime_error(
            "[RessourceManager] cannot read " + configFile.string());
    }
    const auto config = _reader.convertJson<Config>();

    for (const auto &[name, asset] : config.assets)
        addAsset(name, asset.path, asset.type);
}

std::uint32_t ResourceManager::addAsset(
    const std::string &name, const std::string &path, AssetType type)
{
    auto it = _assets.find(name);
    if (it != _assets.end()) {
        it->second.path = path;
        it->second.type = type;
        return it->second.id;
    }
    const std::uint32_t id = _nextId++;
    _assets.emplace(name, Asset {id, type, path});
    _namesById.emplace(id, name);
    return id;
}

void ResourceManager::removeAsset(const std::string &name)
{
    auto it = _assets.find(name);
    if (it == _assets.end()) {
        return;
    }
    _namesById.erase(it->second.id);
    _assets.erase(it);
}

bool ResourceManager::hasAsset(const std::string &name) const
{
    return _assets.contains(name);
}

const ResourceManager::Asset &ResourceManager::getAsset(
    const std::string &name) const
{
    auto it = _assets.find(name);
    if (it == _assets.end()) {
        throw std::out_of_range(
            "ResourceManager: unknown asset '" + name + "'");
    }
    return it->second;
}

std::optional<std::reference_wrapper<const ResourceManager::Asset>>
ResourceManager::getAssetById(std::uint32_t id) const
{
    auto nameIt = _namesById.find(id);
    if (nameIt == _namesById.end()) {
        return std::nullopt;
    }
    return std::cref(_assets.at(nameIt->second));
}

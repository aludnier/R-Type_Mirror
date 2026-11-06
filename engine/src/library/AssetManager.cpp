/*
** EPITECH PROJECT, 2026
** RType
** File description:
** asset metadata manager
*/

#include <map>
#include <stdexcept>
#include <utility>

#include "AssetManager.h"

namespace {
struct AssetConfig {
    std::string path;
    std::string type;
};

struct Config {
    std::map<std::string, AssetConfig> assets;
};

void from_json(const nlohmann::json &json, AssetConfig &asset)
{
    json.at("path").get_to(asset.path);
    if (!json.contains("type") || !json.at("type").is_string())
        throw std::invalid_argument("asset type must be a string");
    json.at("type").get_to(asset.type);
    if (asset.path.empty())
        throw std::invalid_argument("asset path cannot be empty");
    if (asset.type.empty())
        throw std::invalid_argument("asset type cannot be empty");
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, assets)
} // namespace

AssetManager::AssetManager(const std::filesystem::path &configFile)
{
    loadConfigFile(configFile);
}

void AssetManager::loadConfigFile(const std::filesystem::path &configFile)
{
    if (!_reader.readFile(configFile))
        throw std::runtime_error(
            "[AssetManager] cannot read " + configFile.string());

    const auto config = _reader.convertJson<Config>();
    for (const auto &name : config.assets | std::views::keys)
        if (name.empty())
            throw std::invalid_argument("asset name cannot be empty");

    for (const auto &[name, asset] : config.assets)
        addAsset(name, asset.path, asset.type);
}

std::uint32_t AssetManager::addAsset(
    const std::string &name, const std::string &path, std::string_view type)
{
    if (name.empty())
        throw std::invalid_argument("asset name cannot be empty");
    if (path.empty())
        throw std::invalid_argument("asset path cannot be empty");
    if (type.empty())
        throw std::invalid_argument("asset type cannot be empty");

    auto it = _assets.find(name);
    if (it != _assets.end()) {
        it->second.path = path;
        it->second.type = type;
        return it->second.id;
    }

    const std::uint32_t id = _nextId++;
    _assets.emplace(name, Asset {id, std::string(type), path});
    _namesById.emplace(id, name);
    return id;
}

void AssetManager::removeAsset(const std::string &name)
{
    const auto it = _assets.find(name);
    if (it == _assets.end())
        throw std::out_of_range("AssetManager: unknown asset '" + name + "'");
    _namesById.erase(it->second.id);
    _assets.erase(it);
}

bool AssetManager::hasAsset(const std::string &name) const
{
    return _assets.contains(name);
}

const AssetManager::Asset &AssetManager::getAsset(const std::string &name) const
{
    const auto it = _assets.find(name);
    if (it == _assets.end())
        throw std::out_of_range("AssetManager: unknown asset '" + name + "'");
    return it->second;
}

std::optional<std::reference_wrapper<const AssetManager::Asset>>
AssetManager::getAssetById(std::uint32_t id) const
{
    const auto nameIt = _namesById.find(id);
    if (nameIt == _namesById.end())
        return std::nullopt;
    return std::cref(_assets.at(nameIt->second));
}

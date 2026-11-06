/*
** EPITECH PROJECT, 2026
** RType
** File description:
** resource manager
*/

#include <map>
#include <stdexcept>

#include "AssetManager.h"
#include "NlohmanReader.h"

namespace {
struct AssetConfig {
    std::string path;
    AssetManager::AssetType type = AssetManager::AssetType::Texture;
};

struct Config {
    std::map<std::string, AssetConfig> assets;
};

void from_json(const nlohmann::json &j, AssetConfig &a)
{
    using T = AssetManager::AssetType;
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

AssetManager::AssetManager(const std::filesystem::path &configFile)
{
    loadConfigFile(configFile);
}

void AssetManager::loadConfigFile(const std::filesystem::path &configFile)
{
    if (!_reader.readFile(configFile)) {
        throw std::runtime_error(
            "[AssetManager] cannot read " + configFile.string());
    }
    const auto config = _reader.convertJson<Config>();

    for (const auto &[name, asset] : config.assets)
        addAsset(name, asset.path, asset.type);
}

std::uint32_t AssetManager::addAsset(
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

void AssetManager::removeAsset(const std::string &name)
{
    auto it = _assets.find(name);
    if (it == _assets.end()) {
        return;
    }
    _namesById.erase(it->second.id);
    _assets.erase(it);
}

bool AssetManager::hasAsset(const std::string &name) const
{
    return _assets.contains(name);
}

const AssetManager::Asset &AssetManager::getAsset(
    const std::string &name) const
{
    auto it = _assets.find(name);
    if (it == _assets.end()) {
        throw std::out_of_range(
            "AssetManager: unknown asset '" + name + "'");
    }
    return it->second;
}

std::optional<std::reference_wrapper<const AssetManager::Asset>>
AssetManager::getAssetById(std::uint32_t id) const
{
    auto nameIt = _namesById.find(id);
    if (nameIt == _namesById.end()) {
        return std::nullopt;
    }
    return std::cref(_assets.at(nameIt->second));
}

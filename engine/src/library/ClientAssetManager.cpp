#include "ClientAssetManager.h"

ClientAssetManager::ClientAssetManager() = default;

ClientAssetManager::ClientAssetManager(
    const std::filesystem::path &configFile) : _metadata(configFile)
{
}

void ClientAssetManager::preload(const std::string &name)
{
    const auto &asset = metadata(name);
    const auto cached = _cache.find(name);
    if (cached != _cache.end() && cached->second.path == asset.path &&
        cached->second.type == asset.type)
        return;
    _cache.insert_or_assign(name, CacheEntry {
        asset.path, asset.type, _loaders.load(asset.type, asset.path)});
}

void ClientAssetManager::unload(const std::string &name)
{
    if (!metadata().hasAsset(name))
        throw std::out_of_range(
            "ClientAssetManager: unknown asset '" + name + "'");
    _cache.erase(name);
}

void ClientAssetManager::clear()
{
    _cache.clear();
}

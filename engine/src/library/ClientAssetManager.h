/*
** EPITECH PROJECT, 2026
** RType
** File description:
** client asset loading and caching
*/

#ifndef CLIENTASSETMANAGER_H_
#define CLIENTASSETMANAGER_H_

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include "AssetLoaderRegistry.h"
#include "AssetManager.h"

class ClientAssetManager {
public:
    ClientAssetManager();
    explicit ClientAssetManager(const std::filesystem::path &configFile);

    template<LoadableAsset T>
    void registerType()
    {
        _loaders.registerType<T>();
    }

    template<LoadableAsset T>
    const T &load(const std::string &name)
    {
        const auto &asset = metadata(name);
        const std::string expected(AssetTraits<T>::jsonType);
        if (asset.type != expected)
            throw std::invalid_argument("ClientAssetManager: asset '" + name +
                "' has type '" + asset.type + "', requested '" + expected +
                "'");

        const auto cached = _cache.find(name);
        if (cached != _cache.end() && cached->second.path == asset.path &&
            cached->second.type == asset.type)
            return cached->second.value.get<T>();

        auto value = _loaders.load(asset.type, asset.path);
        _cache.insert_or_assign(
            name, CacheEntry {asset.path, asset.type, std::move(value)});
        return _cache.at(name).value.get<T>();
    }

    void preload(const std::string &name);
    void unload(const std::string &name);
    void clear();

    [[nodiscard]] AssetManager &metadata()
    {
        return _metadata;
    }

    [[nodiscard]] const AssetManager &metadata() const
    {
        return _metadata;
    }

    [[nodiscard]] AssetLoaderRegistry &loaderRegistry()
    {
        return _loaders;
    }

    [[nodiscard]] const AssetManager::Asset &getAsset(
        const std::string &name) const
    {
        return metadata().getAsset(name);
    }

private:
    struct CacheEntry {
        std::string path;
        std::string type;
        AssetLoaderRegistry::Holder value;
    };

    const AssetManager::Asset &metadata(const std::string &name) const
    {
        return _metadata.getAsset(name);
    }

    AssetManager _metadata;
    AssetLoaderRegistry _loaders;
    std::unordered_map<std::string, CacheEntry> _cache;
};

#endif

/*
** EPITECH PROJECT, 2026
** RType
** File description:
** client asset loading and caching
*/

#ifndef CLIENTASSETMANAGER_H_
#define CLIENTASSETMANAGER_H_

#include <SFML/Graphics.hpp>
#include <concepts>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include "AssetManager.h"

template <typename T>
struct AssetTraits {
    static constexpr std::string_view jsonType = T::JSON_TYPE;
};

template <typename T>
concept LoadableAsset = requires(const std::filesystem::path &path) {
    { T::load(path) } -> std::same_as<T>;
    { AssetTraits<T>::jsonType } -> std::convertible_to<std::string_view>;
};

struct SfmlTextureAsset {
    static constexpr std::string_view JSON_TYPE = "Texture";
    sf::Texture texture;

    static SfmlTextureAsset load(const std::filesystem::path &path)
    {
        sf::Texture texture;
        if (!texture.loadFromFile(path.string()))
            throw std::runtime_error(
                "ClientAssetManager: failed to load texture '" + path.string() +
                "'");
        return SfmlTextureAsset {std::move(texture)};
    }
};

class ClientAssetManager {
public:
    ClientAssetManager();
    explicit ClientAssetManager(const std::filesystem::path &configFile);

    template <LoadableAsset T>
    void registerType()
    {
        const std::string type(AssetTraits<T>::jsonType);
        if (type.empty())
            throw std::invalid_argument(
                "ClientAssetManager: empty asset type identifier");
        const auto [it, inserted] = _loaders.emplace(type, Loader {
            std::type_index(typeid(T)),
            [](const std::filesystem::path &path) {
                auto value = std::make_unique<T>(T::load(path));
                return Holder {
                    std::unique_ptr<void, Deleter> {
                        value.release(), [](void *resource) {
                            delete static_cast<T *>(resource);
                        }},
                    std::type_index(typeid(T))};
            }});
        if (!inserted && it->second.type != std::type_index(typeid(T)))
            throw std::invalid_argument(
                "ClientAssetManager: conflicting registration for type '" +
                type + "'");
        if (!inserted)
            throw std::invalid_argument(
                "ClientAssetManager: duplicate registration for type '" +
                type + "'");
    }

    template <LoadableAsset T>
    const T &load(const std::string &name)
    {
        const auto &asset = metadata(name);
        const std::string expected(AssetTraits<T>::jsonType);
        if (asset.type != expected)
            throw std::invalid_argument(
                "ClientAssetManager: asset '" + name + "' has type '" +
                asset.type + "', requested '" + expected + "'");
        const auto loader = findLoader(expected);
        const auto cached = _cache.find(name);
        if (cached != _cache.end() && cached->second.path == asset.path &&
            cached->second.type == asset.type &&
            cached->second.value.type == std::type_index(typeid(T)))
            return *static_cast<const T *>(cached->second.value.value.get());

        auto value = loader->load(asset.path);
        _cache.insert_or_assign(name,
            CacheEntry {asset.path, asset.type, std::move(value)});
        return *static_cast<const T *>(_cache.at(name).value.value.get());
    }

    void preload(const std::string &name);
    void unload(const std::string &name);
    void clear();

    [[nodiscard]] AssetManager &metadata() { return _metadata; }
    [[nodiscard]] const AssetManager &metadata() const { return _metadata; }
    [[nodiscard]] const AssetManager::Asset &getAsset(
        const std::string &name) const
    {
        return metadata().getAsset(name);
    }

private:
    using Deleter = std::function<void(void *)>;

    struct Holder {
        std::unique_ptr<void, Deleter> value;
        std::type_index type;
    };
    struct Loader {
        std::type_index type;
        std::function<Holder(const std::filesystem::path &)> load;
    };
    struct CacheEntry {
        std::string path;
        std::string type;
        Holder value;
    };

    const AssetManager::Asset &metadata(const std::string &name) const
    {
        return _metadata.getAsset(name);
    }
    const Loader *findLoader(const std::string &type) const;

    AssetManager _metadata;
    std::unordered_map<std::string, Loader> _loaders;
    std::unordered_map<std::string, CacheEntry> _cache;
};

#endif

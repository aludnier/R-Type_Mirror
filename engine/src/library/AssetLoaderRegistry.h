/*
** EPITECH PROJECT, 2026
** RType
** File description:
** extensible typed asset-loader registry
*/

#ifndef ASSETLOADERREGISTRY_H_
#define ASSETLOADERREGISTRY_H_

#include <any>
#include <concepts>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <utility>

template<typename T>
struct AssetTraits {
    static constexpr std::string_view jsonType = T::JSON_TYPE;
};

template<typename T>
concept LoadableAsset = requires(const std::filesystem::path &path) {
    { T::load(path) } -> std::same_as<T>;
    { AssetTraits<T>::jsonType } -> std::convertible_to<std::string_view>;
};

class AssetLoaderRegistry {
public:
    struct Holder {
        std::any value;

        template<typename T>
        [[nodiscard]]
        const T &get() const
        {
            const auto *typed = std::any_cast<T>(&value);
            if (typed == nullptr)
                throw std::invalid_argument(
                    "AssetLoaderRegistry: cached type mismatch");
            return *typed;
        }
    };

    template<LoadableAsset T>
    void registerType()
    {
        const std::string type(AssetTraits<T>::jsonType);
        if (type.empty())
            throw std::invalid_argument(
                "AssetLoaderRegistry: empty asset type identifier");

        const auto [it, inserted] = _loaders.emplace(type,
            Loader {std::type_index(typeid(T)),
                [](const std::filesystem::path &path) {
                    return Holder {T::load(path)};
                }});
        if (!inserted && it->second.type != std::type_index(typeid(T)))
            throw std::invalid_argument(
                "AssetLoaderRegistry: conflicting registration for type '" +
                type + "'");
        if (!inserted)
            throw std::invalid_argument(
                "AssetLoaderRegistry: duplicate registration for type '" +
                type + "'");
    }

    [[nodiscard]] Holder load(
        std::string_view type, const std::filesystem::path &path) const
    {
        const auto it = _loaders.find(std::string(type));
        if (it == _loaders.end())
            throw std::invalid_argument(
                "AssetLoaderRegistry: unknown asset type '" +
                std::string(type) + "'");
        return it->second.load(path);
    }

private:
    struct Loader {
        std::type_index type;
        std::function<Holder(const std::filesystem::path &)> load;
    };

    std::unordered_map<std::string, Loader> _loaders;
};

#endif

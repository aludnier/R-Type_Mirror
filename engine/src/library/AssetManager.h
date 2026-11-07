/*
** EPITECH PROJECT, 2026
** RType
** File description:
** asset metadata manager
*/

#ifndef ASSETMANAGER_H_
#define ASSETMANAGER_H_

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include "NlohmanReader.h"

class AssetManager {
public:
    class AssetType {
    public:
        inline static constexpr std::string_view Texture = "Texture";
        inline static constexpr std::string_view Sound = "Sound";
        inline static constexpr std::string_view Music = "Music";
        inline static constexpr std::string_view Font = "Font";
    };

    struct Asset {
        std::uint32_t id;
        std::string type;
        std::string path;
    };

    AssetManager() = default;
    explicit AssetManager(const std::filesystem::path &configFile);
    ~AssetManager() = default;

    void loadConfigFile(const std::filesystem::path &configFile);

    std::uint32_t addAsset(const std::string &name, const std::string &path,
        std::string_view type);

    void removeAsset(const std::string &name);
    bool hasAsset(const std::string &name) const;
    const Asset &getAsset(const std::string &name) const;
    std::optional<std::reference_wrapper<const Asset>> getAssetById(
        std::uint32_t id) const;

    /**
     * @brief Gets the name of an asset from its id.
     * @return The name, or std::nullopt if the id is unknown.
     */
    std::optional<std::string> getAssetName(std::uint32_t id) const;

private:
    RType::nlohamReader _reader;
    std::unordered_map<std::string, Asset> _assets;
    std::unordered_map<std::uint32_t, std::string> _namesById;
    std::uint32_t _nextId = 1;
};

#endif

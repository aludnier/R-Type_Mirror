/*
** EPITECH PROJECT, 2026
** RType
** File description:
** resource manager
*/

#ifndef RESOURCEMANAGER_H_
#define RESOURCEMANAGER_H_

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>

class ResourceManager {
public:
    enum class AssetType : std::uint8_t { Texture, Sound, Music, Font };

    struct Asset {
        std::uint32_t id;
        AssetType type;
        std::string path;
    };

    ResourceManager() = default;
    ~ResourceManager() = default;

    /**
     * @brief Registers an asset, or updates it if the name already exists.
     * @param name Unique name used to refer to the asset.
     * @param path File path of the resource.
     * @param type Kind of resource (defaults to Texture).
     * @return The id of the asset.
     * @note Re-adding an existing name updates its path and type but keeps its
     *       id, so components that already reference it stay valid.
     */
    std::uint32_t addAsset(const std::string &name, const std::string &path,
        AssetType type = AssetType::Texture);

    /**
     * @brief Removes an asset from the manager.
     * @param name Name of the asset to remove.
     * @note Does nothing if the name is unknown. Components still holding the
     *       removed id will no longer resolve through getAssetById().
     */
    void removeAsset(const std::string &name);

    /**
     * @brief Checks whether an asset is registered.
     * @param name Name of the asset.
     * @return true if the asset exists, false otherwise.
     */
    bool hasAsset(const std::string &name) const;

    /**
     * @brief Gets an asset from its name.
     * @param name Name of the asset.
     * @return Reference to the asset description.
     * @throws std::out_of_range If the name is unknown.
     * @note Use hasAsset() first if the asset may not exist.
     */
    const Asset &getAsset(const std::string &name) const;

    /**
     * @brief Gets an asset from its id.
     * @param id Id of the asset (as stored in components).
     * @return Optional reference to the asset, or std::nullopt if the id is
     * unknown.
     * @note The reference is invalidated if the asset is removed.
     */
    std::optional<std::reference_wrapper<const Asset>> getAssetById(
        std::uint32_t id) const;

private:
    std::unordered_map<std::string, Asset> _assets;
    std::unordered_map<std::uint32_t, std::string> _namesById;
    std::uint32_t _nextId = 1;
};

#endif

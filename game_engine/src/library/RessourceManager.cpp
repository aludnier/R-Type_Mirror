/*
** EPITECH PROJECT, 2026
** RType
** File description:
** resource manager
*/

#include "RessourceManager.h"
#include <stdexcept>

std::uint32_t ResourceManager::addAsset(const std::string &name, const std::string &path,
                                        AssetType type)
{
    auto it = _assets.find(name);
    if (it != _assets.end()) {
        it->second.path = path;
        it->second.type = type;
        return it->second.id;
    }
    const std::uint32_t id = _nextId++;
    _assets.emplace(name, Asset{id, type, path});
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
    return _assets.find(name) != _assets.end();
}

const ResourceManager::Asset &ResourceManager::getAsset(const std::string &name) const
{
    auto it = _assets.find(name);
    if (it == _assets.end()) {
        throw std::out_of_range("ResourceManager: unknown asset '" + name + "'");
    }
    return it->second;
}

std::optional<std::reference_wrapper<const ResourceManager::Asset>> ResourceManager::getAssetById(std::uint32_t id) const
{
    auto nameIt = _namesById.find(id);
    if (nameIt == _namesById.end()) {
        return std::nullopt;
    }
    return std::cref(_assets.at(nameIt->second));
}

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Registry.h
*/

#ifndef REGISTRY_H
#define REGISTRY_H

#include <algorithm>
#include <any>
#include <functional>
#include <queue>
#include <ranges>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Component.h"

namespace RType {
/**
 * @brief Central ECS container: owns entities, component storages and systems.
 */
class Registry {
public:
    using eraser_t = std::function<void(Registry &, Entity const &)>;

    struct RegistryEntry {
        std::any storage;
        eraser_t erase;
    };

    /**
     * @brief Registers a component type and creates its storage.
     * @tparam C Component type.
     * @return Reference to the newly created storage.
     * @note Calling this twice for the same type replaces the existing storage
     *       and discards all its data.
     */
    template<Component C>
    storage_t<C> &registerComponent()
    {
        auto key = std::type_index(typeid(C));
        _componentsStorage.insert_or_assign(key,
            RegistryEntry {.storage = std::any(storage_t<C>()),
                .erase = [](Registry &r, Entity const &e) {
                    r.getComponents<C>().erase(e);
                }});
        return this->getComponents<C>();
    }

    /** @brief Whether registerComponent<C>() was called. */
    template<Component C>
    [[nodiscard]] bool isRegistered() const
    {
        return _componentsStorage.contains(std::type_index(typeid(C)));
    }

    /**
     * @brief Gets the storage of a component type.
     * @tparam C Component type.
     * @throws std::out_of_range If @p C was not registered.
     */
    template<Component C>
    storage_t<C> &getComponents()
    {
        auto key = std::type_index(typeid(C));
        return std::any_cast<storage_t<C> &>(
            _componentsStorage.at(key).storage);
    }

    /**
     * @brief Creates a new entity and adds it to the live entities.
     */
    Entity spawnEntity()
    {
        Entity entity(_nextEntityId++);
        _entities.push_back(entity);
        return entity;
    }

    /**
     * @brief Live entities, in spawn order.
     * @note Killed entities leave this list as soon as killEntity() is called.
     */
    [[nodiscard]] const std::vector<Entity> &entities() const
    {
        return _entities;
    }

    [[nodiscard]] bool isAlive(Entity const &entity) const
    {
        return std::ranges::find(_entities, entity) != _entities.end();
    }

    static Entity entityFromIndex(entity_id_type id)
    {
        return Entity(id);
    }

    /**
     * @brief Schedules an entity for destruction.
     * @param entity Entity to destroy.
     * @note Destruction is deferred: components are removed at the end of
     *       runSystems(), so the entity stays valid during the current frame.
     *       It is removed from entities() right away.
     */
    void killEntity(Entity const &entity)
    {
        std::erase(_entities, entity);
        _entitiesToKill.push(entity);
    }

    template<Component C>
    storage_t<C>::reference_type addComponent(Entity const &to, C &&component)
    {
        return this->getComponents<C>().insert_at(
            to, std::forward<C>(component));
    }

    template<Component C, typename... Params>
    storage_t<C>::reference_type emplaceComponent(
        Entity const &to, Params &&...params)
    {
        return this->getComponents<C>().emplace(
            to, std::forward<Params>(params)...);
    }

    template<Component C>
    void removeComponent(Entity const &from)
    {
        this->getComponents<C>().erase(from);
    }

    /**
     * @brief Adds a system run on every call to runSystems().
     * @tparam components Component types whose storages are passed to @p f.
     * @param f Callable taking `storage_t<components> &...`.
     */
    template<Component... components, typename Function>
    void addSystem(Function &&f)
    {
        this->_systems.emplace_back(
            [f = std::forward<Function>(f)](
                Registry &r) mutable { f(r.getComponents<components>()...); });
    }

    /**
     * @brief Runs all systems in insertion order, then destroys pending
     * entities.
     */
    void runSystems()
    {
        for (auto &system : _systems) {
            system(*this);
        }
        this->killAllEntities();
    }

private:
    void killAllEntities()
    {
        while (!_entitiesToKill.empty()) {
            Entity entity = _entitiesToKill.front();
            _entitiesToKill.pop();
            for (auto &[storage, erase] :
                _componentsStorage | std::views::values) {
                erase(*this, entity);
            }
        }
    }

    entity_id_type _nextEntityId = 0;
    std::vector<Entity> _entities;
    std::queue<Entity> _entitiesToKill;

    std::unordered_map<std::type_index, RegistryEntry> _componentsStorage;
    std::vector<std::function<void(Registry &)>> _systems;
};
} // namespace RType

#endif // REGISTRY_H

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Component.h
*/

#ifndef COMPONENT_H
#define COMPONENT_H

#include <type_traits>
#include <optional>
#include <concepts>

#include "Entity.h"

namespace RType {
    template <class C>
    concept Component = std::is_object_v<C> && std::is_move_constructible_v<C> && std::is_destructible_v<C>;

    template <class Storage, class Component>
    concept ComponentStorage = std::is_copy_constructible_v<Storage> &&
        requires(Storage storage, const Storage cStorage, entity_id_type entity, Component component) {
            { storage.find(entity) } -> std::same_as<std::optional<Component>>;
            { storage.emplace(entity, std::move(component)) } -> std::same_as<Component &>;
            { storage.erase(entity) } -> std::same_as<void>;
            { cStorage.count() } -> std::convertible_to<size_t>;
            storage.forEach([](entity_id_type, Component &) {});
        };

    template <class C>
    concept HasCustomStorage = Component<C> && requires {typename C::storage; } && ComponentStorage<typename C::storage, C>;

    template <Component C>
    class SparseArray;

    template <Component C>
    struct storage_for {
        using type = SparseArray<C>;
    };

    template<Component C> requires HasCustomStorage<C>
    struct storage_for<C> {
        using type = C::storage;
    };

    template <Component C>
    using storage_t = storage_for<C>::type;
} // RType

#endif
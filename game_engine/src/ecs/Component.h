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
    /**
     * @brief Type trait concept identifying a valid component type.
     *
     * A Component is a plain object type that can be moved and destroyed.
     * Components are intended to be small, copyable/movable value-like objects
     * stored inside component storages (see ComponentStorage).
     *
     * Requirements:
     * - std::is_object_v<C> is true
     * - C is move-constructible
     * - C is destructible
     */
    concept Component = std::is_object_v<C> && std::is_move_constructible_v<C> && std::is_destructible_v<C>;

    /**
     * @brief Concept describing the interface of a component storage.
     *
     * A ComponentStorage is a container-like type used to store optional
     * components indexed by entity id. Storages are expected to expose a set
     * of nested types and operations used by the ECS registry and systems.
     *
     * Required nested types:
     * - value_type: must be std::optional<Component>
     * - reference_type: value_type &
     * - const_reference_type: const value_type &
     * - size_type: a size/index type
     * - iterator / const_iterator: iterator types compatible with begin()/end()
     *
     * Required special members:
     * - default constructible
     * - copy constructible / assignable
     * - nothrow move constructible / assignable where appropriate
     *
     * Required member functions (signatures shown informally):
     * - operator[](std::size_t) -> reference_type
     * - const operator[](std::size_t) -> const_reference_type
     * - begin(), end(), cbegin(), cend()
     * - size() -> size_type
     * - insert_at(pos, component) -> reference_type
     * - emplace_at(pos, ...) -> reference_type
     * - erase(pos) -> void
     * - get_index(const value_type&) -> size_type
     *
     * These names are used by the Registry and related utilities to access
     * and manipulate component data.
     */
    template<class S, class Component>
    concept ComponentStorage =
            requires
            {
                typename S::value_type;
                typename S::reference_type;
                typename S::const_reference_type;
                typename S::size_type;
                typename S::iterator;
                typename S::const_iterator;
            } &&
            std::same_as<typename S::value_type, std::optional<Component> > &&
            std::same_as<typename S::reference_type, typename S::value_type &> &&
            std::same_as<typename S::const_reference_type, const typename S::value_type &> &&

            // ---- special member functions ----
            std::default_initializable<S> &&
            std::is_copy_constructible_v<S> &&
            std::is_nothrow_move_constructible_v<S> &&
            std::is_nothrow_destructible_v<S> &&
            std::is_copy_assignable_v<S> &&
            std::is_nothrow_move_assignable_v<S> &&

            // ---- member functions ----
            requires(S s, const S cs, S other, const S cOther,
                     std::size_t idx,
                     typename S::size_type pos,
                     Component &&rvalue,
                     const Component &lvalue,
                     const typename S::value_type &opt)
            {
                // assignment operators return *this
                { s = cOther } -> std::same_as<S &>;
                { s = std::move(other) } -> std::same_as<S &>;

                // element access
                { s[idx] } -> std::same_as<typename S::reference_type>;
                { cs[idx] } -> std::same_as<typename S::const_reference_type>;

                // iterators
                { s.begin() } -> std::same_as<typename S::iterator>;
                { cs.begin() } -> std::same_as<typename S::const_iterator>;
                { cs.cbegin() } -> std::same_as<typename S::const_iterator>;
                { s.end() } -> std::same_as<typename S::iterator>;
                { cs.end() } -> std::same_as<typename S::const_iterator>;
                { cs.cend() } -> std::same_as<typename S::const_iterator>;

                // size
                { cs.size() } -> std::same_as<typename S::size_type>;

                // insertion
                { s.insert_at(pos, lvalue) } -> std::same_as<typename S::reference_type>;
                { s.insert_at(pos, std::move(rvalue)) } -> std::same_as<typename S::reference_type>;
                { s.emplace_at(pos, lvalue) } -> std::same_as<typename S::reference_type>;
                { s.emplace_at(pos, std::move(rvalue)) } -> std::same_as<typename S::reference_type>;

                // removal / lookup
                { s.erase(pos) } -> std::same_as<void>;
                { cs.getIndex(opt) } -> std::same_as<typename S::size_type>;
            };

    /**
     * @brief Detects components that provide a custom storage type.
     *
     * If a component type C defines a nested alias `using storage = ...`,
     * and that type satisfies ComponentStorage for C, then HasCustomStorage<C>
     * is true. This allows components to opt-in to custom storage
     * implementations instead of the default SparseArray.
     */
    template <class C>
    concept HasCustomStorage = Component<C> && requires {typename C::storage; } && ComponentStorage<typename C::storage, C>;

    /**
     * @brief Default component storage used by the ECS.
     *
     * A SparseArray stores components in a contiguous vector indexed by
     * entity id. Each element is an std::optional<C> to represent presence or
     * absence of a component on a given entity. The concrete implementation
     * is provided in `SparseArray.h`.
     */
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
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
                { cs.get_index(opt) } -> std::same_as<typename S::size_type>;
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
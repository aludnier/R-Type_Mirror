/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** SparseArray.h
*/
#ifndef SPARSEARRAY_H
#define SPARSEARRAY_H

#include <optional>
#include <vector>
#include <algorithm>
#include <stdexcept>

#include "Component.h"

namespace RType {
    /**
     * @brief Default sparse storage for components indexed by entity id.
     *
     * SparseArray stores an std::vector<std::optional<C>> where each index
     * corresponds to an entity id. An engaged optional means the entity has
     * that component; a disengaged optional means the component is absent.
     *
     * The storage exposes contiguous iteration over the underlying vector
     * (including disengaged entries). Helper methods such as insert_at,
     * emplace, erase and getIndex are provided to manage component presence.
     *
     * @tparam C Component type stored in the array.
     */
    template<Component C>
    class SparseArray {
    public:
        using value_type = std::optional<C>;
        using reference_type = value_type &;
        using const_reference_type = const value_type &;
        using container_t = std::vector<value_type>;
        using size_type = container_t::size_type;

        using iterator = container_t::iterator;
        using const_iterator = container_t::const_iterator;

        /** Default-construct an empty storage. */
        SparseArray() = default;

        SparseArray(const SparseArray &) = default;

        SparseArray(SparseArray &&) noexcept = default;

        ~SparseArray() = default;

        SparseArray &operator=(const SparseArray &) = default;

        SparseArray &operator=(SparseArray &&) noexcept = default;

        /**
         * @brief Access or create the slot for the given entity id.
         *
         * If the internal vector is too small to hold the requested index it
         * is resized. Returns a reference to the optional component for the
         * entity.
         *
         * @param entity Entity id index.
         * @return reference to the std::optional<C> at the given index.
         */
        reference_type operator[](entity_id_type entity) {
            if (entity >= this->_data.size())
                this->_data.resize(entity + 1);
            return this->_data[entity];
        }

        /**
         * @brief Const access to the slot for the given entity id.
         *
         * Returns a const reference to an internal optional if the index is
         * valid; otherwise returns a reference to a static empty optional.
         *
         * @param entity Entity id index.
         * @return const reference to the optional component at the index, or
         *         a reference to std::nullopt if out of range.
         */
        const_reference_type operator[](entity_id_type entity) const {
            static const value_type empty = std::nullopt;
            if (entity >= this->_data.size())
                return empty;
            return this->_data[entity];
        }

        iterator begin() { return this->_data.begin(); }
        const_iterator begin() const { return this->_data.begin(); }
        const_iterator cbegin() const { return this->_data.cbegin(); }

        iterator end() { return this->_data.end(); }
        const_iterator end() const { return this->_data.end(); }
        const_iterator cend() const { return this->_data.cend(); }

        size_type size() const { return this->_data.size(); }

        /**
         * @brief Inserts a copy of a component at the given position.
         *
         * Resizes the internal storage if necessary. Returns a reference to
         * the optional containing the newly stored component.
         *
         * @param pos Index where the component should be inserted.
         * @param component Component to copy into storage.
         * @return reference to the stored optional component.
         */
        reference_type insert_at(size_type pos, C const &component) {
            if (pos >= this->_data.size()) {
                this->_data.resize(pos + 1);
            }
            return this->_data[pos] = component;
        }

        reference_type insert_at(size_type pos, C &&component) {
            if (pos >= this->_data.size()) {
                this->_data.resize(pos + 1);
            }
            return this->_data[pos] = std::move(component);
        }

        template<class... Params>
        reference_type emplace(size_type pos, Params &&... params) {
            if (pos >= this->_data.size())
                this->_data.resize(pos + 1);
            this->_data[pos].emplace(std::forward<Params>(params)...);
            return this->_data[pos];
        }

        /**
         * @brief Remove the component for the given entity (if present).
         *
         * The optional at the provided index is reset to std::nullopt; the
         * internal vector size is not reduced.
         */
        void erase(entity_id_type entity) {
            if (entity < this->_data.size()) {
                this->_data[entity] = std::nullopt;
            }
        }

        /**
         * @brief Count how many components are currently present in storage.
         *
         * @return Number of engaged optionals in the internal vector.
         */
        [[nodiscard]] size_t count() const {
            return std::ranges::count_if(this->_data, [](const std::optional<C> &item) { return item.has_value(); });
        }

        /**
         * @brief Find the index of the provided optional element.
         *
         * This compares addresses of optionals to locate the exact element in
         * the internal vector. Throws std::out_of_range when the element is
         * not present in this storage.
         *
         * @param component Reference to an element previously obtained from this storage.
         * @return Index of the element in the internal vector.
         * @throws std::out_of_range If the element is not found.
         */
        size_type getIndex(value_type const &component) const {
            auto it = std::ranges::find_if(this->_data, [&](const value_type &item) {
                return std::addressof(item) == std::addressof(component);
            });
            if (it == this->_data.end())
                throw std::out_of_range("Component not found in SparseArray");
            return static_cast<size_type>(std::distance(this->_data.begin(), it));
        }

    private:
        std::vector<std::optional<C> > _data;
    };
} // RType

#endif

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

        SparseArray() = default;

        SparseArray(const SparseArray &) = default;

        SparseArray(SparseArray &&) noexcept = default;

        ~SparseArray() = default;

        SparseArray &operator=(const SparseArray &) = default;

        SparseArray &operator=(SparseArray &&) noexcept = default;

        reference_type operator[](entity_id_type entity) {
            if (entity >= this->_data.size())
                this->_data.resize(entity + 1);
            return this->_data[entity];
        }

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

        void erase(entity_id_type entity) {
            if (entity < this->_data.size()) {
                this->_data[entity] = std::nullopt;
            }
        }

        [[nodiscard]] size_t count() const {
            return std::ranges::count_if(this->_data, [](const std::optional<C> &item) { return item.has_value(); });
        }

        size_type getIndex(value_type const &component) const {
            auto it = std::ranges::find_if(this->_data, [&](const value_type &item) {
                return std::addressof(item) == std::addressof(component);
            });
            if (it == this->_data.end())
                throw std::out_of_range("Component not found in SparseArray");
            return static_cast<size_type>(std::distance(this->_data.begin(), it));
        }

        template<typename Func>
        void forEach(Func func) {
            for (entity_id_type i = 0; i < this->_data.size(); ++i) {
                if (this->_data[i].has_value()) {
                    func(i, *this->_data[i]);
                }
            }
        }

    private:
        std::vector<std::optional<C> > _data;
    };
} // RType

#endif

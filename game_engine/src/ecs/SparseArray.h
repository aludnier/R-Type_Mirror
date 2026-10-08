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

#include "Component.h"

namespace RType {
    template<Component C>
    class SparseArray {
    public:
        std::optional<C> find(entity_id_type entity) const {
            if (entity >= _data.size())
                return std::nullopt;
            return _data[entity];
        }

        C &emplace(entity_id_type entity, C &&component) {
            if (entity >= _data.size()) {
                _data.resize(entity + 1);
            }
            _data[entity] = std::move(component);
            return *_data[entity];
        }

        void erase(entity_id_type entity) {
            if (entity < _data.size()) {
                _data[entity] = std::nullopt;
            }
        }

        [[nodiscard]] size_t count() const {
            return std::ranges::count_if(_data, [](const std::optional<C> &item) { return item.has_value(); });
        }

        template<typename Func>
        void forEach(Func func) {
            for (entity_id_type i = 0; i < _data.size(); ++i) {
                if (_data[i].has_value()) {
                    func(i, *_data[i]);
                }
            }
        }

    private:
        std::vector<std::optional<C> > _data;
    };
} // RType

#endif

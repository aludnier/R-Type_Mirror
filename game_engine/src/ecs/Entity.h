/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Entity.h
*/

#ifndef ENTITY_H
#define ENTITY_H
#include <cstddef>

namespace RType {
using entity_id_type = size_t;

class Entity {
public:
    operator entity_id_type() const;
    bool operator==(const Entity &other) const;
    bool operator!=(const Entity &other) const;

private:
    entity_id_type _id;

    friend class Registry;
    explicit Entity(entity_id_type id);
};
} // namespace RType

#endif

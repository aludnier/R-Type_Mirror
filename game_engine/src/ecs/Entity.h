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
    class Entity {
    public:
        operator size_t() const;
        bool operator==(const Entity &other) const;
        bool operator!=(const Entity &other) const;
    private:
        size_t _id;

        friend class Registry;
        explicit Entity(size_t id);
    };
}

#endif

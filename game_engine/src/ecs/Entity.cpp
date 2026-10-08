/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Entity.cpp
*/

#include "Entity.h"

RType::Entity::operator size_t() const {
    return _id;
}

bool RType::Entity::operator==(const Entity &other) const {
    return this->_id == other._id;
}

bool RType::Entity::operator!=(const Entity &other) const {
    return this->_id != other._id;
}

RType::Entity::Entity(size_t id) : _id(id) {
}

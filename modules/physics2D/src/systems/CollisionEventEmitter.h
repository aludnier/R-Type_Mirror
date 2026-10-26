/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionEventEmitter.h
*/

#ifndef PHYSICS2D_COLLISIONEVENTEMITTER_H
    #define PHYSICS2D_COLLISIONEVENTEMITTER_H

#include <set>
#include "../components/CollisionEvents2D.h"
#include "Contact.h"
#include "ecs/SparseArray.h"

namespace RType::Physics2D {
/** Turns the difference between two frames' contacts into Enter/Exit events. */
class CollisionEventEmitter {
public:
    void emit(
        const ContactBuffer &contacts, SparseArray<CollisionEvents2D> &events);

private:
    std::set<EntityPair> _previous;
};
} // namespace RType::Physics2D

#endif // PHYSICS2D_COLLISIONEVENTEMITTER_H

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CollisionEventEmitter.cpp
*/

#include "CollisionEventEmitter.h"
#include "ecs/Registry.h"

namespace RType::Physics2D {
namespace {
    void clearEvents(SparseArray<CollisionEvents2D> &events)
    {
        for (auto &e : events)
            if (e.has_value())
                e->events.clear();
    }

    void push(SparseArray<CollisionEvents2D> &events, entity_id_type self,
        entity_id_type other, bool trigger, Vector2 normal, CollisionKind kind)
    {
        if (self >= events.size() || !events[self].has_value())
            return;
        events[self]->events.emplace_back(
            Registry::entityFromIndex(other), trigger, normal, kind);
    }

    void pushBoth(SparseArray<CollisionEvents2D> &events,
        const EntityPair &pair, bool trigger, Vector2 normal,
        CollisionKind kind)
    {
        push(events, pair.first, pair.second, trigger, normal, kind);
        push(events, pair.second, pair.first, trigger, -normal, kind);
    }
} // namespace

void CollisionEventEmitter::emit(
    const ContactBuffer &contacts, SparseArray<CollisionEvents2D> &events)
{
    clearEvents(events);
    std::set<EntityPair> current;
    for (const auto &c : contacts) {
        current.insert(c.pair);
        if (!_previous.contains(c.pair))
            pushBoth(events, c.pair, c.trigger, c.normal, CollisionKind::Enter);
    }
    for (const auto &pair : _previous)
        if (!current.contains(pair))
            pushBoth(events, pair, false, {}, CollisionKind::Exit);
    _previous = std::move(current);
}
} // namespace RType::Physics2D

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Contact.h
*/

#ifndef PHYSICS2D_CONTACT_H
    #define PHYSICS2D_CONTACT_H

#include <memory>
#include <utility>
#include <vector>
#include "ecs/Entity.h"
#include "math/Vector2.h"

namespace RType::Physics2D {
    using EntityPair = std::pair<entity_id_type, entity_id_type>;

    struct Contact {
        EntityPair pair;
        Vector2 normal; // from pair.first to pair.second
        float penetration;
        bool trigger;
    };

    /** Contacts of the current frame, shared between detection and response systems. */
    using ContactBuffer = std::vector<Contact>;
    using SharedContacts = std::shared_ptr<ContactBuffer>;
}

#endif // PHYSICS2D_CONTACT_H

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Physics2DBindings.cpp
*/

#include "Physics2DBindings.h"

#include "components/BoxCollider2D.h"
#include "components/CircleCollider2D.h"
#include "components/CollisionEvent2D.h"
#include "components/CollisionEvents2D.h"
#include "components/PhysicsSettings2D.h"
#include "components/Rigidbody2D.h"
#include "math/Vector2.h"

namespace RType::Physics2D {
namespace {
    // Vector2 fields are exposed through pointers so that
    // `body.velocity.x = 1` edits the component instead of a copy.
    template<class C, Vector2 C::*Field>
    auto vectorField()
    {
        return sol::property([](C &component) { return &(component.*Field); },
            [](C &component, const Vector2 &value) {
                component.*Field = value;
            });
    }

    void bindRigidbody(sol::state &lua)
    {
        lua.new_usertype<Rigidbody2D>("Rigidbody2D",
            sol::constructors<Rigidbody2D()>(),
            "velocity",
            vectorField<Rigidbody2D, &Rigidbody2D::velocity>(),
            "mass",
            &Rigidbody2D::mass,
            "drag",
            &Rigidbody2D::drag,
            "gravity_scale",
            &Rigidbody2D::gravityScale,
            "is_kinematic",
            &Rigidbody2D::isKinematic,
            "is_static",
            &Rigidbody2D::isStatic,
            "add_force",
            &Rigidbody2D::addForce,
            "add_impulse",
            &Rigidbody2D::addImpulse,
            "force",
            &Rigidbody2D::force,
            "clear_force",
            &Rigidbody2D::clearForce);
    }

    void bindColliders(sol::state &lua)
    {
        lua.new_usertype<BoxCollider2D>("BoxCollider2D",
            sol::factories([] { return BoxCollider2D {}; },
                [](const Vector2 &size) { return BoxCollider2D {.size = size}; }),
            "size",
            vectorField<BoxCollider2D, &BoxCollider2D::size>(),
            "offset",
            vectorField<BoxCollider2D, &BoxCollider2D::offset>(),
            "is_trigger",
            &BoxCollider2D::isTrigger,
            "layer",
            &BoxCollider2D::layer,
            "mask",
            &BoxCollider2D::mask);
        lua.new_usertype<CircleCollider2D>("CircleCollider2D",
            sol::factories([] { return CircleCollider2D {}; },
                [](float radius) { return CircleCollider2D {.radius = radius}; }),
            "radius",
            &CircleCollider2D::radius,
            "offset",
            vectorField<CircleCollider2D, &CircleCollider2D::offset>(),
            "is_trigger",
            &CircleCollider2D::isTrigger,
            "layer",
            &CircleCollider2D::layer,
            "mask",
            &CircleCollider2D::mask);
    }

    void bindSettings(sol::state &lua)
    {
        lua.new_usertype<PhysicsSettings2D>("PhysicsSettings2D",
            sol::constructors<PhysicsSettings2D()>(),
            "gravity",
            vectorField<PhysicsSettings2D, &PhysicsSettings2D::gravity>(),
            "dt",
            &PhysicsSettings2D::dt,
            "bounciness",
            &PhysicsSettings2D::bounciness);
    }

    void bindCollisionEvents(sol::state &lua)
    {
        lua.new_enum("CollisionKind",
            "Enter",
            CollisionKind::Enter,
            "Exit",
            CollisionKind::Exit);
        // Read-only: the physics systems fill them every frame.
        lua.new_usertype<CollisionEvent2D>("CollisionEvent2D",
            sol::no_constructor,
            "other",
            sol::readonly_property([](const CollisionEvent2D &event) {
                return static_cast<lua_Integer>(
                    static_cast<entity_id_type>(event.other));
            }),
            "is_trigger",
            sol::readonly(&CollisionEvent2D::isTrigger),
            "normal",
            sol::readonly(&CollisionEvent2D::normal),
            "kind",
            sol::readonly(&CollisionEvent2D::kind));
        lua.new_usertype<CollisionEvents2D>("CollisionEvents2D",
            sol::constructors<CollisionEvents2D()>(),
            "events",
            sol::readonly_property(
                [](CollisionEvents2D &events) { return &events.events; }));
    }
} // namespace

void bindScripting(ScriptEngine &scripts)
{
    sol::state &lua = scripts.lua();

    bindRigidbody(lua);
    bindColliders(lua);
    bindSettings(lua);
    bindCollisionEvents(lua);
    scripts.bindComponent<Rigidbody2D>("Rigidbody2D");
    scripts.bindComponent<BoxCollider2D>("BoxCollider2D");
    scripts.bindComponent<CircleCollider2D>("CircleCollider2D");
    scripts.bindComponent<PhysicsSettings2D>("PhysicsSettings2D");
    scripts.bindComponent<CollisionEvents2D>("CollisionEvents2D");
}
} // namespace RType::Physics2D

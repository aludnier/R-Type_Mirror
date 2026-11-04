/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CoreBindings.cpp
*/

#include "CoreBindings.h"

#include <array>
#include <cstdint>

#include "components/Action.h"
#include "components/Controllable.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Transform2D.h"
#include "math/Vector2.h"

namespace {
using RType::Controllable;
using RType::Health;
using RType::Hitbox;
using RType::Transform2D;
using RType::Vector2;

Vector2 add(const Vector2 &a, const Vector2 &b)
{
    return a + b;
}

Vector2 subtract(const Vector2 &a, const Vector2 &b)
{
    return a - b;
}

Vector2 scale(const Vector2 &v, float s)
{
    return v * s;
}

bool equal(const Vector2 &a, const Vector2 &b)
{
    return a.x == b.x && a.y == b.y;
}

// Vector2 fields are exposed through pointers so that `t.position.x = 1`
// edits the component instead of a copy.
Vector2 *position(Transform2D &t)
{
    return &t.position;
}

void setPosition(Transform2D &t, const Vector2 &v)
{
    t.position = v;
}

Vector2 *scaleOf(Transform2D &t)
{
    return &t.scale;
}

void setScale(Transform2D &t, const Vector2 &v)
{
    t.scale = v;
}

Vector2 *size(Hitbox &h)
{
    return &h.size;
}

void setSize(Hitbox &h, const Vector2 &v)
{
    h.size = v;
}

std::uint8_t actionBit(Action action)
{
    return static_cast<std::uint8_t>(1U << static_cast<unsigned>(action));
}

bool isActive(const Controllable &c, Action action)
{
    return (c.actionMask & actionBit(action)) != 0;
}

void setActive(Controllable &c, Action action, bool active)
{
    if (active)
        c.actionMask |= actionBit(action);
    else
        c.actionMask &= static_cast<std::uint8_t>(~actionBit(action));
}

void bindVector2(sol::state &lua)
{
    lua.new_usertype<Vector2>("Vector2",
        sol::constructors<Vector2(), Vector2(float, float)>(),
        "x",
        &Vector2::x,
        "y",
        &Vector2::y,
        sol::meta_function::addition,
        &add,
        sol::meta_function::subtraction,
        &subtract,
        sol::meta_function::multiplication,
        &scale,
        sol::meta_function::equal_to,
        &equal);
}

void bindTransform2D(sol::state &lua)
{
    lua.new_usertype<Transform2D>("Transform2D",
        sol::constructors<Transform2D(),
            Transform2D(Vector2),
            Transform2D(Vector2, Vector2),
            Transform2D(Vector2, Vector2, float)>(),
        "position",
        sol::property(&position, &setPosition),
        "scale",
        sol::property(&scaleOf, &setScale),
        "rotation",
        &Transform2D::rotation);
}

void bindHealth(sol::state &lua)
{
    lua.new_usertype<Health>("Health",
        sol::constructors<Health(), Health(int), Health(int, int)>(),
        "current",
        &Health::current,
        "max",
        &Health::max,
        "is_dead",
        &Health::isDead,
        "take_damage",
        &Health::takeDamage);
}

void bindHitbox(sol::state &lua)
{
    lua.new_usertype<Hitbox>("Hitbox",
        sol::constructors<Hitbox(), Hitbox(Vector2), Hitbox(float, float)>(),
        "size",
        sol::property(&size, &setSize));
}

void bindAction(sol::state &lua)
{
    lua.new_enum<Action>("Action",
        {{"MoveUp", Action::MoveUp},
            {"MoveDown", Action::MoveDown},
            {"MoveLeft", Action::MoveLeft},
            {"MoveRight", Action::MoveRight},
            {"Shoot", Action::Shoot}});
}

void bindControllable(sol::state &lua)
{
    lua.new_usertype<Controllable>("Controllable",
        sol::constructors<Controllable(), Controllable(std::uint8_t)>(),
        "action_mask",
        &Controllable::actionMask,
        "is_active",
        &isActive,
        "set_active",
        &setActive);
}

// Add a new core type here.
constexpr std::array coreTypes = {
    &bindVector2,
    &bindTransform2D,
    &bindHealth,
    &bindHitbox,
    &bindAction,
    &bindControllable,
};
} // namespace

void RType::Scripting::bindCoreTypes(sol::state &lua)
{
    for (const auto bind : coreTypes)
        bind(lua);
}

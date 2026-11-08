/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** CoreBindings.cpp
*/

#include "CoreBindings.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>

#include "components/Action.h"
#include "components/Controllable.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Name.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "math/Vector2.h"

namespace {
using RType::Controllable;
using RType::Health;
using RType::Hitbox;
using RType::Name;
using RType::Sprite;
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

std::uint8_t actionBit(RType::ActionId action)
{
    if (action >= RType::MAX_ACTIONS)
        throw std::out_of_range("action does not fit the mask");
    return RType::ActionMask {1} << action;
}

bool isActive(const Controllable &c, RType::ActionId action)
{
    return (c.actionMask & actionBit(action)) != 0;
}

void setActive(Controllable &c, RType::ActionId action, bool active)
{
    if (active)
        c.actionMask |= actionBit(action);
    else
        c.actionMask &= ~actionBit(action);
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

void bindName(sol::state &lua)
{
    lua.new_usertype<Name>("Name",
        sol::constructors<Name(), Name(std::string)>(),
        "value",
        &Name::value);
}

/** @brief A color channel clamped to 0..255 instead of wrapping around. */
template<std::uint8_t Sprite::*Channel>
auto colorChannel()
{
    return sol::property(
        [](const Sprite &sprite) { return static_cast<int>(sprite.*Channel); },
        [](Sprite &sprite, lua_Number value) {
            sprite.*Channel =
                static_cast<std::uint8_t>(std::clamp(value, 0.0, 255.0));
        });
}

void bindSprite(sol::state &lua)
{
    lua.new_usertype<Sprite>("Sprite",
        sol::constructors<Sprite(),
            Sprite(std::uint32_t),
            Sprite(std::uint32_t, int),
            Sprite(std::uint32_t, int, int, int, int),
            Sprite(std::uint32_t, int, int, int, int, int)>(),
        "asset_id",
        &Sprite::assetId,
        "rect_left",
        &Sprite::rectLeft,
        "rect_top",
        &Sprite::rectTop,
        "rect_width",
        &Sprite::rectWidth,
        "rect_height",
        &Sprite::rectHeight,
        "origin_x",
        &Sprite::originX,
        "origin_y",
        &Sprite::originY,
        "r",
        colorChannel<&Sprite::r>(),
        "g",
        colorChannel<&Sprite::g>(),
        "b",
        colorChannel<&Sprite::b>(),
        "a",
        colorChannel<&Sprite::a>(),
        "z_index",
        &Sprite::zIndex);
}

void bindAction(sol::state &lua)
{
    // Filled by the game, see ScriptEngine::defineAction().
    lua["Action"] = lua.create_table();
}

void bindControllable(sol::state &lua)
{
    lua.new_usertype<Controllable>("Controllable",
        sol::constructors<Controllable(), Controllable(RType::ActionMask)>(),
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
    &bindName,
    &bindSprite,
    &bindAction,
    &bindControllable,
};
} // namespace

void RType::Scripting::bindCoreTypes(sol::state &lua)
{
    for (const auto bind : coreTypes)
        bind(lua);
}

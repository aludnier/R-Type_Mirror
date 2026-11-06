/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Reflection.cpp
*/

#include "Reflection.h"

#include <algorithm>

#include "components/Health.h"
#include "components/Name.h"
#include "components/Sprite.h"
#include "components/Transform2D.h"
#include "scripting/ScriptComponent.h"

std::vector<RType::ComponentInfo> &RType::Reflection::components()
{
    return _components;
}

const std::vector<RType::ComponentInfo> &RType::Reflection::components() const
{
    return _components;
}

RType::ComponentInfo *RType::Reflection::find(const std::string &name)
{
    auto it = std::ranges::find(_components, name, &ComponentInfo::name);
    return it == _components.end() ? nullptr : &*it;
}

const RType::ComponentInfo *RType::Reflection::find(
    const std::string &name) const
{
    auto it = std::ranges::find(_components, name, &ComponentInfo::name);
    return it == _components.end() ? nullptr : &*it;
}

void RType::reflectCoreComponents(Reflection &reflection)
{
    reflection.reflect<Name>("Name").field("value", &Name::value);
    reflection.reflect<Transform2D>("Transform2D")
        .field("position", &Transform2D::position)
        .field("scale", &Transform2D::scale)
        .field("rotation", &Transform2D::rotation);
    reflection.reflect<Health>("Health")
        .field("current", &Health::current)
        .field("max", &Health::max);
    reflection.reflect<Sprite>("Sprite")
        .field("assetId", &Sprite::assetId)
        .field("rectLeft", &Sprite::rectLeft)
        .field("rectTop", &Sprite::rectTop)
        .field("rectWidth", &Sprite::rectWidth)
        .field("rectHeight", &Sprite::rectHeight)
        .field("originX", &Sprite::originX)
        .field("originY", &Sprite::originY)
        .field("r", &Sprite::r)
        .field("g", &Sprite::g)
        .field("b", &Sprite::b)
        .field("a", &Sprite::a)
        .field("zIndex", &Sprite::zIndex);
    reflection.reflect<ScriptComponent>("Script").field(
        "scripts", &ScriptComponent::scripts);
}

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Reflection.h
*/

#ifndef REFLECTION_H
#define REFLECTION_H

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "ecs/Registry.h"
#include "ecs/SparseArray.h"
#include "math/Vector2.h"

namespace RType {
/** @brief Type of a reflected field, used to pick the right editor widget. */
enum class FieldType {
    Float,
    Int,
    Bool,
    String,
    Vector2,
    UInt8,
    UInt32,
    /** @brief `std::vector<std::filesystem::path>`, like script paths. */
    PathList,
};

/**
 * @brief The FieldType matching a C++ member type.
 *
 * Unsupported types fail to compile in Reflection::reflect().field().
 */
template<typename T>
constexpr FieldType fieldTypeOf() = delete;

template<>
constexpr FieldType fieldTypeOf<float>()
{
    return FieldType::Float;
}

template<>
constexpr FieldType fieldTypeOf<int>()
{
    return FieldType::Int;
}

template<>
constexpr FieldType fieldTypeOf<bool>()
{
    return FieldType::Bool;
}

template<>
constexpr FieldType fieldTypeOf<std::string>()
{
    return FieldType::String;
}

template<>
constexpr FieldType fieldTypeOf<RType::Vector2>()
{
    return FieldType::Vector2;
}

template<>
constexpr FieldType fieldTypeOf<std::uint8_t>()
{
    return FieldType::UInt8;
}

template<>
constexpr FieldType fieldTypeOf<std::uint32_t>()
{
    return FieldType::UInt32;
}

template<>
constexpr FieldType fieldTypeOf<std::vector<std::filesystem::path>>()
{
    return FieldType::PathList;
}

/**
 * @brief One reflected member of a component.
 *
 * `get` takes a pointer to the component (as returned by ComponentInfo::get)
 * and returns a pointer to the member, to cast to the C++ type of @ref type
 * (`float *` for Float, `RType::Vector2 *` for Vector2...).
 */
struct FieldInfo {
    std::string name;
    FieldType type;
    std::function<void *(void *component)> get;

    /** @brief Typed access to the member, @p T must match @ref type. */
    template<typename T>
    T &as(void *component) const
    {
        return *static_cast<T *>(get(component));
    }
};

/**
 * @brief Type-erased access to one component type of a Registry.
 *
 * Same idea as Scripting::ComponentBinding, without Lua. `get` returns a
 * pointer to the component of the entity, or nullptr when it has none.
 * `has` is false when the component storage is not registered.
 */
struct ComponentInfo {
    std::string name;
    std::vector<FieldInfo> fields;
    std::function<bool(Registry &, Entity const &)> has;
    std::function<void *(Registry &, Entity const &)> get;
    /** @brief Adds a default constructed component (replaces an existing one).
     */
    std::function<void(Registry &, Entity const &)> add;
    std::function<void(Registry &, Entity const &)> remove;
};

/**
 * @brief List of the component types an editor (or a scene file) can see.
 *
 * @code
 * reflection.reflect<Health>("Health")
 *     .field("current", &Health::current)
 *     .field("max", &Health::max);
 *
 * for (ComponentInfo &c : reflection.components()) {
 *     if (!c.has(reg, e))
 *         continue;
 *     for (const FieldInfo &f : c.fields)
 *         drawField(f, c.get(reg, e));
 * }
 * @endcode
 */
class Reflection {
public:
    /** @brief Returned by reflect() to declare the fields of @p C. */
    template<Component C>
    class Builder {
    public:
        Builder(Reflection &reflection, std::size_t index) :
            _reflection(reflection), _index(index)
        {
        }

        template<typename T>
        Builder &field(const std::string &name, T C::*member)
        {
            _reflection._components[_index].fields.push_back(FieldInfo {
                .name = name,
                .type = fieldTypeOf<std::remove_cv_t<T>>(),
                .get = [member](void *component) -> void * {
                    return &(static_cast<C *>(component)->*member);
                },
            });
            return *this;
        }

    private:
        Reflection &_reflection;
        std::size_t _index;
    };

    /**
     * @brief Declares a component type under @p name.
     * @note Calling it again with the same name replaces the previous entry.
     */
    template<Component C>
        requires std::default_initializable<C>
    Builder<C> reflect(const std::string &name);

    /** @brief Reflected components, in declaration order. */
    [[nodiscard]] std::vector<ComponentInfo> &components();
    [[nodiscard]] const std::vector<ComponentInfo> &components() const;

    /** @brief The component declared under @p name, or nullptr. */
    [[nodiscard]] ComponentInfo *find(const std::string &name);
    [[nodiscard]] const ComponentInfo *find(const std::string &name) const;

private:
    std::vector<ComponentInfo> _components;
};

/**
 * @brief Declares the engine components: Name, Transform2D, Health, Sprite
 *        and Script (ScriptComponent).
 * @note Sprite::assetId is a raw id for now. Scene files must save it as an
 *       asset name instead, ids are not stable between runs.
 * @note Only ScriptComponent::scripts is reflected: `started` is runtime state
 *       and goes back to 0 when a scene is loaded.
 */
void reflectCoreComponents(Reflection &reflection);

template<Component C>
    requires std::default_initializable<C>
Reflection::Builder<C> Reflection::reflect(const std::string &name)
{
    ComponentInfo info {
        .name = name,
        .fields = {},
        .has =
            [](Registry &registry, Entity const &entity) {
                if (!registry.isRegistered<C>())
                    return false;
                auto &storage = registry.getComponents<C>();
                return entity < storage.size() && storage[entity].has_value();
            },
        .get = [](Registry &registry, Entity const &entity) -> void * {
            if (!registry.isRegistered<C>())
                return nullptr;
            auto &storage = registry.getComponents<C>();
            if (entity >= storage.size() || !storage[entity].has_value())
                return nullptr;
            return &storage[entity].value();
        },
        .add =
            [](Registry &registry, Entity const &entity) {
                registry.addComponent(entity, C {});
            },
        .remove =
            [](Registry &registry, Entity const &entity) {
                registry.removeComponent<C>(entity);
            },
    };
    if (ComponentInfo *existing = find(name)) {
        *existing = std::move(info);
        return Builder<C>(
            *this, static_cast<std::size_t>(existing - _components.data()));
    }
    _components.push_back(std::move(info));
    return Builder<C>(*this, _components.size() - 1);
}
} // namespace RType

#endif // REFLECTION_H

/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** Scene.cpp
*/

#include "Scene.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>

#include "components/Name.h"
#include "math/Vector2.h"

namespace {
using nlohmann::json;
using RType::FieldInfo;
using RType::FieldType;

constexpr const char *NAME_COMPONENT = "Name";

void warn(const std::string &message)
{
    std::cerr << "[Scene] warning: " << message << std::endl;
}

json saveField(
    const FieldInfo &field, void *component, const AssetManager &assets)
{
    switch (field.type) {
        case FieldType::Float:
            return field.as<float>(component);
        case FieldType::Int:
            return field.as<int>(component);
        case FieldType::Bool:
            return field.as<bool>(component);
        case FieldType::String:
            return field.as<std::string>(component);
        case FieldType::Vector2: {
            const auto &value = field.as<RType::Vector2>(component);
            return json::array({value.x, value.y});
        }
        case FieldType::UInt8:
            return field.as<std::uint8_t>(component);
        case FieldType::UInt32:
            return field.as<std::uint32_t>(component);
        case FieldType::PathList: {
            json paths = json::array();
            for (const auto &path :
                field.as<std::vector<std::filesystem::path>>(component))
                paths.push_back(path.generic_string());
            return paths;
        }
        case FieldType::AssetId: {
            const auto id = field.as<std::uint32_t>(component);
            if (id == 0)
                return nullptr;
            auto name = assets.getAssetName(id);
            if (!name) {
                warn("no asset name for id " + std::to_string(id));
                return nullptr;
            }
            return *name;
        }
    }
    return nullptr;
}

template<typename T>
bool loadInteger(const json &value, T &out)
{
    if (!value.is_number_integer())
        return false;
    const auto number = value.get<long long>();
    if (number < std::numeric_limits<T>::min() ||
        number > std::numeric_limits<T>::max())
        return false;
    out = static_cast<T>(number);
    return true;
}

/** @return false if @p value does not fit the field. */
bool loadField(const FieldInfo &field, void *component, const json &value,
    const AssetManager &assets)
{
    switch (field.type) {
        case FieldType::Float:
            if (!value.is_number())
                return false;
            field.as<float>(component) = value.get<float>();
            return true;
        case FieldType::Int:
            return loadInteger(value, field.as<int>(component));
        case FieldType::Bool:
            if (!value.is_boolean())
                return false;
            field.as<bool>(component) = value.get<bool>();
            return true;
        case FieldType::String:
            if (!value.is_string())
                return false;
            field.as<std::string>(component) = value.get<std::string>();
            return true;
        case FieldType::Vector2:
            if (!value.is_array() || value.size() != 2 ||
                !value[0].is_number() || !value[1].is_number())
                return false;
            field.as<RType::Vector2>(component) = {
                value[0].get<float>(), value[1].get<float>()};
            return true;
        case FieldType::UInt8:
            return loadInteger(value, field.as<std::uint8_t>(component));
        case FieldType::UInt32:
            return loadInteger(value, field.as<std::uint32_t>(component));
        case FieldType::PathList: {
            if (!value.is_array())
                return false;
            std::vector<std::filesystem::path> paths;
            for (const auto &path : value) {
                if (!path.is_string())
                    return false;
                paths.emplace_back(path.get<std::string>());
            }
            field.as<std::vector<std::filesystem::path>>(component) =
                std::move(paths);
            return true;
        }
        case FieldType::AssetId: {
            if (value.is_null())
                return true;
            if (!value.is_string())
                return false;
            const auto name = value.get<std::string>();
            if (!assets.hasAsset(name)) {
                warn("unknown asset '" + name + "'");
                return true;
            }
            field.as<std::uint32_t>(component) = assets.getAsset(name).id;
            return true;
        }
    }
    return false;
}

const FieldInfo *findField(
    const RType::ComponentInfo &component, const std::string &name)
{
    for (const auto &field : component.fields) {
        if (field.name == name)
            return &field;
    }
    return nullptr;
}

void loadComponent(RType::Registry &registry, RType::Entity entity,
    const RType::ComponentInfo &info, const json &fields,
    const AssetManager &assets)
{
    if (!fields.is_object()) {
        warn("component '" + info.name + "' is not an object");
        return;
    }
    info.add(registry, entity);
    void *component = info.get(registry, entity);
    for (const auto &[name, value] : fields.items()) {
        const FieldInfo *field = findField(info, name);
        if (field == nullptr)
            warn("unknown field '" + info.name + "." + name + "'");
        else if (!loadField(*field, component, value, assets))
            warn("bad value for '" + info.name + "." + name + "'");
    }
}

RType::Entity loadEntity(RType::Registry &registry,
    const RType::Reflection &reflection, const json &data,
    const AssetManager &assets)
{
    RType::Entity entity = registry.spawnEntity();

    if (auto name = data.find("name"); name != data.end()) {
        const RType::ComponentInfo *info = reflection.find(NAME_COMPONENT);
        if (info != nullptr)
            loadComponent(registry,
                entity,
                *info,
                json::object({{"value", *name}}),
                assets);
    }
    auto components = data.find("components");
    if (components == data.end())
        return entity;
    if (!components->is_object()) {
        warn("'components' is not an object");
        return entity;
    }
    for (const auto &[name, fields] : components->items()) {
        const RType::ComponentInfo *info = reflection.find(name);
        if (info == nullptr || name == NAME_COMPONENT)
            warn("unknown component '" + name + "'");
        else
            loadComponent(registry, entity, *info, fields, assets);
    }
    return entity;
}
} // namespace

nlohmann::json RType::Scene::toJson(Registry &registry,
    const Reflection &reflection, const AssetManager &assets)
{
    json entities = json::array();

    for (const Entity &entity : registry.entities()) {
        if (registry.isPendingKill(entity))
            continue;
        json data = json::object();
        json components = json::object();

        for (const ComponentInfo &info : reflection.components()) {
            if (!info.has(registry, entity))
                continue;
            void *component = info.get(registry, entity);
            if (info.name == NAME_COMPONENT) {
                data["name"] = static_cast<Name *>(component)->value;
                continue;
            }
            json fields = json::object();
            for (const FieldInfo &field : info.fields)
                fields[field.name] = saveField(field, component, assets);
            components[info.name] = std::move(fields);
        }
        data["components"] = std::move(components);
        entities.push_back(std::move(data));
    }
    return json::object({{"entities", std::move(entities)}});
}

std::vector<RType::Entity> RType::Scene::fromJson(Registry &registry,
    const Reflection &reflection, const nlohmann::json &scene,
    const AssetManager &assets)
{
    std::vector<Entity> spawned;

    if (!scene.is_object() || !scene.contains("entities") ||
        !scene["entities"].is_array()) {
        warn("no 'entities' array in the scene");
        return spawned;
    }
    for (const auto &data : scene["entities"]) {
        if (!data.is_object()) {
            warn("an entity is not an object");
            continue;
        }
        spawned.push_back(loadEntity(registry, reflection, data, assets));
    }
    return spawned;
}

std::string RType::Scene::saveToString(Registry &registry,
    const Reflection &reflection, const AssetManager &assets)
{
    return toJson(registry, reflection, assets).dump(4);
}

std::vector<RType::Entity> RType::Scene::loadFromString(Registry &registry,
    const Reflection &reflection, const std::string &text,
    const AssetManager &assets)
{
    json scene = json::parse(text, nullptr, false);

    if (scene.is_discarded())
        throw std::runtime_error("the scene is not valid JSON");
    return fromJson(registry, reflection, scene, assets);
}

void RType::Scene::saveToFile(Registry &registry, const Reflection &reflection,
    const std::filesystem::path &file, const AssetManager &assets)
{
    // Built before opening the file: a throwing dump must not truncate it.
    const std::string text = saveToString(registry, reflection, assets);
    std::ofstream stream(file);

    if (!stream)
        throw std::runtime_error("cannot write the scene " + file.string());
    stream << text << '\n';
    stream.close();
    if (!stream)
        throw std::runtime_error("cannot write the scene " + file.string());
}

std::vector<RType::Entity> RType::Scene::loadFromFile(Registry &registry,
    const Reflection &reflection, const std::filesystem::path &file,
    const AssetManager &assets)
{
    std::ifstream stream(file);

    if (!stream)
        throw std::runtime_error("cannot read the scene " + file.string());
    json scene = json::parse(stream, nullptr, false);
    if (scene.is_discarded())
        throw std::runtime_error(file.string() + " is not valid JSON");
    return fromJson(registry, reflection, scene, assets);
}

void RType::Scene::clear(Registry &registry)
{
    for (const Entity &entity : registry.entities())
        registry.killEntity(entity);
    registry.killAllEntities();
}

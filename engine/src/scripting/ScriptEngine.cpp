/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ScriptEngine.cpp
*/

#include "ScriptEngine.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <lua.h>
#include <map>
#include <stdexcept>

#include "CoreBindings.h"
#include "components/Controllable.h"
#include "components/Health.h"
#include "components/Hitbox.h"
#include "components/Name.h"
#include "components/Transform2D.h"
#include "library/NlohmanReader.h"

namespace {
RType::entity_id_type toEntityId(lua_Integer value)
{
    if (value < 0)
        throw std::runtime_error("invalid entity id " + std::to_string(value));
    return static_cast<RType::entity_id_type>(value);
}
} // namespace

RType::ScriptEngine::ScriptEngine(Registry &registry) :
    _registry(registry)
{
    _registry.registerComponent<ScriptData>();
    _registry.registerComponent<ScriptComponent>();
    openSandbox();
    Scripting::bindCoreTypes(_lua);
    bindCoreComponents();
    bindWorld();
}

RType::ScriptEngine::~ScriptEngine()
{
    _systems.clear();
    _entityScripts.clear();
    // ScriptData holds Lua references: drop them while the state still exists.
    for (auto &data : _registry.getComponents<ScriptData>())
        data.reset();
}

void RType::ScriptEngine::defineAction(const std::string &name, ActionId action)
{
    if (action >= MAX_ACTIONS)
        throw std::out_of_range("action '" + name + "' does not fit the mask");
    _lua["Action"][name] = action;
}

void RType::ScriptEngine::openSandbox()
{
    _lua.open_libraries(sol::lib::base,
        sol::lib::math,
        sol::lib::string,
        sol::lib::table,
        sol::lib::coroutine,
        sol::lib::utf8);
    // Scripts must not read files or load precompiled bytecode.
    _lua["dofile"] = sol::lua_nil;
    _lua["loadfile"] = sol::lua_nil;
    _lua["load"] = sol::lua_nil;
}

void RType::ScriptEngine::bindCoreComponents()
{
    bindComponent<Transform2D>("Transform2D");
    bindComponent<Health>("Health");
    bindComponent<Hitbox>("Hitbox");
    bindComponent<Name>("Name");
    bindComponent<Controllable>("Controllable");
    bindComponent<ScriptComponent>("Script");
}

void RType::ScriptEngine::bindWorld()
{
    // Same binding as set_function(name, &ScriptEngine::fn, this).
    const auto method = [this](auto fn) {
        return sol::make_object(_lua, sol::as_function(fn, this));
    };
    // Lua name -> ScriptEngine method. Add a world function here.
    const std::map<std::string, sol::object> functions = {
        {"spawn", method(&ScriptEngine::spawn)},
        {"kill", method(&ScriptEngine::kill)},
        {"has", method(&ScriptEngine::has)},
        {"get", method(&ScriptEngine::get)},
        {"add", method(&ScriptEngine::add)},
        {"remove", method(&ScriptEngine::remove)},
        {"data", method(&ScriptEngine::data)},
        {"each", method(&ScriptEngine::each)},
        {"add_system", method(&ScriptEngine::addSystem)},
    };
    sol::table world = _lua.create_named_table("world");
    for (const auto &[name, function] : functions)
        world[name] = function;
}

lua_Integer RType::ScriptEngine::spawn() const
{
    return static_cast<lua_Integer>(
        static_cast<entity_id_type>(_registry.spawnEntity()));
}

void RType::ScriptEngine::kill(lua_Integer e)
{
    _registry.killEntity(Registry::entityFromIndex(toEntityId(e)));
}

bool RType::ScriptEngine::has(lua_Integer e, const std::string &name) const
{
    return binding(name).has(_registry, toEntityId(e));
}

sol::object RType::ScriptEngine::get(
    lua_Integer e, const std::string &name) const
{
    return binding(name).get(_registry, toEntityId(e), _lua.lua_state());
}

sol::object RType::ScriptEngine::add(
    lua_Integer e, const std::string &name, const sol::object &value)
{
    const auto &component = binding(name);
    const auto id = toEntityId(e);
    try {
        component.add(_registry, id, value);
    } catch (const std::runtime_error &) {
        throw std::runtime_error("world.add: value is not a " + name);
    }
    return component.get(_registry, id, _lua.lua_state());
}

void RType::ScriptEngine::remove(lua_Integer e, const std::string &name)
{
    binding(name).remove(_registry, toEntityId(e));
}

sol::table RType::ScriptEngine::data(lua_Integer e)
{
    return entityData(toEntityId(e));
}

void RType::ScriptEngine::addSystem(
    const std::string &name, const sol::protected_function &fn)
{
    _systems.push_back({.name = name, .update = fn, .enabled = true});
}

const RType::Scripting::ComponentBinding &RType::ScriptEngine::binding(
    const std::string &name) const
{
    const auto it = _components.find(name);
    if (it == _components.end())
        throw std::runtime_error("unknown component '" + name + "'");
    return it->second;
}

sol::table RType::ScriptEngine::entityData(entity_id_type id)
{
    auto &storage = _registry.getComponents<ScriptData>();
    if (id < storage.size() && storage[id].has_value())
        return storage[id]->fields;
    sol::table fields = _lua.create_table();
    _registry.addComponent<ScriptData>(
        Registry::entityFromIndex(id), ScriptData {fields});
    return fields;
}

void RType::ScriptEngine::each(
    const sol::table &names, const sol::protected_function &fn)
{
    std::vector<const Scripting::ComponentBinding *> components;
    for (std::size_t i = 1; i <= names.size(); ++i)
        components.push_back(&binding(names.get<std::string>(i)));
    if (components.empty())
        throw std::runtime_error("world.each: no component given");

    // Entities spawned during the loop are not visited.
    std::size_t count = std::numeric_limits<std::size_t>::max();
    for (const auto *component : components)
        count = std::min(count, component->size(_registry));

    std::vector<sol::object> args(components.size() + 1);
    for (entity_id_type id = 0; id < count; ++id) {
        const bool match =
            std::ranges::all_of(components, [&](const auto *component) {
                return component->has(_registry, id);
            });
        if (!match)
            continue;
        args[0] = sol::make_object(_lua, static_cast<lua_Integer>(id));
        for (std::size_t i = 0; i < components.size(); ++i)
            args[i + 1] = components[i]->get(_registry, id, _lua.lua_state());
        auto result = fn(sol::as_args(args));
        if (!result.valid()) {
            sol::error error = result;
            throw std::runtime_error(error.what());
        }
    }
}

void RType::ScriptEngine::runSystems()
{
    // Index loop: a system may add another one while it runs.
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (std::size_t i = 0; i < _systems.size(); ++i) {
        if (!_systems[i].enabled)
            continue;
        auto result = _systems[i].update(_deltaTime);
        if (!result.valid()) {
            sol::error error = result;
            // Disabled so that a broken script does not flood the logs every
            // frame. reload() enables it again.
            _systems[i].enabled = false;
            reportError("system '" + _systems[i].name + "': " + error.what());
        }
    }
}

void RType::ScriptEngine::update(SparseArray<ScriptComponent> &scripts)
{
    runSystems();
    runEntityScripts(scripts);
}

void RType::ScriptEngine::runEntityScripts(
    SparseArray<ScriptComponent> &scripts)
{
    // Scripts may add components: the storage is indexed again after each
    // call, and entities spawned during the loop wait for the next frame.
    const std::size_t count = scripts.size();
    for (entity_id_type id = 0; id < count; ++id) {
        if (!scripts[id].has_value())
            continue;
        const auto paths = scripts[id]->scripts;
        const auto started = scripts[id]->started;
        for (std::size_t i = 0; i < paths.size(); ++i) {
            auto key = std::make_pair(id, paths[i].string());
            if (_failedEntityScripts.contains(key))
                continue;
            bool ok = i >= started ? callEntityScript(id, paths[i], "on_start")
                                   : true;
            ok = ok && callEntityScript(id, paths[i], "on_update");
            if (!ok)
                _failedEntityScripts.insert(std::move(key));
        }
        if (id < scripts.size() && scripts[id].has_value())
            scripts[id]->started = std::max(scripts[id]->started, paths.size());
    }
}

bool RType::ScriptEngine::callEntityScript(
    entity_id_type id, const std::filesystem::path &path, const char *function)
{
    const auto script = entityScript(path);
    if (!script.has_value())
        return false;
    const sol::object callback = (*script)[function];
    if (callback.get_type() != sol::type::function)
        return true;
    auto result = callback.as<sol::protected_function>()(
        static_cast<lua_Integer>(id), _deltaTime);
    if (!result.valid()) {
        sol::error error = result;
        reportError("entity " + std::to_string(id) + " script '" +
            path.string() + "' " + function + ": " + error.what());
        return false;
    }
    return true;
}

std::optional<sol::table> RType::ScriptEngine::entityScript(
    const std::filesystem::path &path)
{
    const auto file = path.is_relative() ? _scriptDirectory / path : path;
    const auto key = file.string();
    if (const auto it = _entityScripts.find(key); it != _entityScripts.end())
        return it->second;
    std::optional<sol::table> script;
    if (!std::filesystem::is_regular_file(file)) {
        reportError("cannot open script '" + key + "'");
    } else {
        auto result = _lua.safe_script_file(key, sol::script_pass_on_error);
        if (!result.valid()) {
            sol::error error = result;
            reportError(error.what());
        } else if (result.get_type() != sol::type::table) {
            reportError("script '" + key + "' must return a table");
        } else {
            script = result.get<sol::table>();
        }
    }
    _entityScripts.emplace(key, script);
    return script;
}

bool RType::ScriptEngine::loadString(
    const std::string &code, const std::string &chunkName)
{
    auto result = _lua.safe_script(code, sol::script_pass_on_error, chunkName);
    if (!result.valid()) {
        sol::error error = result;
        reportError(error.what());
        return false;
    }
    return true;
}

bool RType::ScriptEngine::loadFile(const std::filesystem::path &path)
{
    if (!std::filesystem::is_regular_file(path)) {
        reportError("cannot open script '" + path.string() + "'");
        return false;
    }
    if (std::ranges::find(_files, path) == _files.end())
        _files.push_back(path);
    auto result =
        _lua.safe_script_file(path.string(), sol::script_pass_on_error);
    if (!result.valid()) {
        sol::error error = result;
        reportError(error.what());
        return false;
    }
    return true;
}

void RType::ScriptEngine::loadConfigFile(
    const std::filesystem::path &configFile)
{
    nlohamReader reader(configFile);
    std::filesystem::path directory = reader.get<std::string>("directory", ".");
    if (directory.is_relative())
        directory = configFile.parent_path() / directory;
    _scriptDirectory = directory;
    for (const auto &script :
        reader.get<std::vector<std::string>>("scripts", {})) {
        if (!loadFile(directory / script))
            throw std::runtime_error("[ScriptEngine] " + _lastError);
    }
}

bool RType::ScriptEngine::reload()
{
    _systems.clear();
    _entityScripts.clear();
    _failedEntityScripts.clear();
    _lastError.clear();
    bool ok = true;
    const auto files = _files;
    for (const auto &file : files)
        ok = loadFile(file) && ok;
    return ok;
}

void RType::ScriptEngine::setScriptDirectory(
    const std::filesystem::path &directory)
{
    _scriptDirectory = directory;
    _entityScripts.clear();
}

void RType::ScriptEngine::setDeltaTime(float dt)
{
    _deltaTime = dt;
}

const std::string &RType::ScriptEngine::lastError() const
{
    return _lastError;
}

std::size_t RType::ScriptEngine::systemCount() const
{
    return _systems.size();
}

sol::state &RType::ScriptEngine::lua()
{
    return _lua;
}

void RType::ScriptEngine::reportError(const std::string &message)
{
    _lastError = message;
    std::cerr << "[ScriptEngine] " << message << std::endl;
}

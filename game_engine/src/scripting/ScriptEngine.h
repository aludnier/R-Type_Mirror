/*
** EPITECH PROJECT, 2026
** R-Type
** File description:
** ScriptEngine.h
*/

#ifndef SCRIPTENGINE_H_
#define SCRIPTENGINE_H_

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <sol/sol.hpp>

#include "ScriptComponent.h"
#include "ecs/Registry.h"
#include "ecs/SparseArray.h"

namespace RType {
/**
 * @brief Lua state attached to an entity, created by `world.data(e)`.
 *
 * Lets scripts keep their own per-entity fields (AI state, timers...)
 * without declaring a C++ component.
 */
struct ScriptData {
    sol::table fields;
};

/**
 * @brief Runs Lua scripts on top of a Registry.
 *
 * Scripts get a sandboxed Lua 5.5 state (no io / os / package / debug) and a
 * `world` table to spawn and kill entities, read and write the components
 * bound with bindComponent(), and add game-wide systems. Entity scripts are
 * attached with a ScriptComponent. Nothing runs until addScriptSystem() adds
 * the system calling update(). The whole Lua API is described in
 * docs/scripting.md.
 *
 * @note The engine registers the ScriptData and ScriptComponent storages, and
 *       must be destroyed before the registry, never after.
 */
class ScriptEngine {
public:
    /**
     * @brief Creates the Lua state, binds the core components and registers
     *        the ScriptData and ScriptComponent storages.
     * @param registry Registry the scripts act on. Must outlive the engine.
     */
    explicit ScriptEngine(Registry &registry);
    ~ScriptEngine();

    ScriptEngine(const ScriptEngine &) = delete;
    ScriptEngine &operator=(const ScriptEngine &) = delete;

    /**
     * @brief Makes a component type usable from scripts under @p name.
     *
     * The Lua usertype of @p C (constructors, fields) must be declared with
     * lua().new_usertype<C>() before scripts use it. The component storage
     * must be registered in the registry.
     *
     * @tparam C Component type.
     * @param name Name used in `world.add/get/has/remove/each`.
     */
    template<Component C>
    void bindComponent(const std::string &name)
    {
        _components.insert_or_assign(name,
            ComponentBinding {
                .has =
                    [](Registry &r, entity_id_type id) {
                        auto &storage = r.getComponents<C>();
                        return id < storage.size() && storage[id].has_value();
                    },
                .get = [](Registry &r, entity_id_type id, lua_State *lua)
                    -> sol::object {
                    auto &storage = r.getComponents<C>();
                    if (id >= storage.size() || !storage[id].has_value())
                        return sol::make_object(lua, sol::lua_nil);
                    return sol::make_object(lua, &storage[id].value());
                },
                .add =
                    [](Registry &r,
                        entity_id_type id,
                        const sol::object &value) {
                        if (value.is<C>()) {
                            r.addComponent<C>(Registry::entityFromIndex(id),
                                C(value.as<C>()));
                            return;
                        }
                        if constexpr (std::default_initializable<C>) {
                            if (!value.valid() ||
                                value.get_type() == sol::type::lua_nil) {
                                r.addComponent<C>(
                                    Registry::entityFromIndex(id), C {});
                                return;
                            }
                        }
                        throw std::runtime_error("bad value for component");
                    },
                .remove =
                    [](Registry &r, entity_id_type id) {
                        r.removeComponent<C>(Registry::entityFromIndex(id));
                    },
                .size = [](Registry &r) { return r.getComponents<C>().size(); },
            });
    }

    /**
     * @brief Runs a script file and remembers it for reload().
     * @return false if the file cannot be read or raises an error, see
     *         lastError().
     */
    bool loadFile(const std::filesystem::path &path);

    /**
     * @brief Runs a chunk of Lua code. It is not replayed by reload().
     * @return false if the code raises an error, see lastError().
     */
    bool loadString(
        const std::string &code, const std::string &chunkName = "chunk");

    /**
     * @brief Loads the scripts listed in a JSON config file.
     *
     * Format: `{"directory": "scripts", "scripts": ["a.lua", "b.lua"]}`.
     * Relative directories are resolved from the config file location. The
     * directory also becomes the script directory (see setScriptDirectory()).
     * `scripts` may be omitted when only entity scripts are used.
     *
     * @throws std::runtime_error If the config or one script cannot be loaded.
     */
    void loadConfigFile(const std::filesystem::path &configFile);

    /**
     * @brief Sets the directory relative ScriptComponent paths are read from.
     */
    void setScriptDirectory(const std::filesystem::path &directory);

    /**
     * @brief Drops every script system, runs the loaded files again and
     *        reloads the entity scripts on their next update.
     *
     * Per-entity `world.data` tables, the registry and the on_start already
     * done are kept, so a game can be edited while it runs.
     *
     * @return false if a file failed, see lastError().
     */
    bool reload();

    /**
     * @brief Sets the delta time (seconds) passed to the script systems on the
     *        next Registry::runSystems().
     */
    void setDeltaTime(float dt);

    /**
     * @brief Runs one frame: the `world.add_system` systems, then the scripts
     *        of every entity having a ScriptComponent.
     *
     * Called by the system added with addScriptSystem().
     */
    void update(SparseArray<ScriptComponent> &scripts);

    /** @return Expires when the engine is destroyed. */
    [[nodiscard]] std::weak_ptr<bool> lifetime() const;

    /** @return The last error raised by a script, empty if none. */
    [[nodiscard]] const std::string &lastError() const;

    /** @return The number of systems added by scripts, disabled ones included.
     */
    [[nodiscard]] std::size_t systemCount() const;

    /** @return The Lua state, to bind extra types or call script functions. */
    [[nodiscard]] sol::state &lua();

private:
    struct ComponentBinding {
        std::function<bool(Registry &, entity_id_type)> has;
        std::function<sol::object(Registry &, entity_id_type, lua_State *)> get;
        std::function<void(Registry &, entity_id_type, const sol::object &)>
            add;
        std::function<void(Registry &, entity_id_type)> remove;
        std::function<std::size_t(Registry &)> size;
    };

    struct ScriptSystem {
        std::string name;
        sol::protected_function update;
        bool enabled = true;
    };

    void openSandbox();
    void bindCoreTypes();
    void bindWorld();
    void runSystems();
    void runEntityScripts(SparseArray<ScriptComponent> &scripts);
    bool callEntityScript(entity_id_type id, const std::filesystem::path &path,
        const char *function);
    [[nodiscard]] std::optional<sol::table> entityScript(
        const std::filesystem::path &path);
    void bindScriptComponent();
    void reportError(const std::string &message);

    [[nodiscard]] const ComponentBinding &binding(
        const std::string &name) const;
    [[nodiscard]] sol::table entityData(entity_id_type id);
    void each(const sol::table &names, const sol::protected_function &fn);

    // Declared first so that every Lua reference below dies before the state.
    sol::state _lua;
    Registry &_registry;
    std::unordered_map<std::string, ComponentBinding> _components;
    std::vector<ScriptSystem> _systems;
    std::vector<std::filesystem::path> _files;
    std::filesystem::path _scriptDirectory = ".";
    // Entity scripts, loaded once per path.
    std::unordered_map<std::string, std::optional<sol::table>> _entityScripts;
    // Entity scripts disabled after an error, until reload().
    std::set<std::pair<entity_id_type, std::string>> _failedEntityScripts;
    std::string _lastError;
    float _deltaTime = 0.f;
    // Lets the script system know whether the engine is still alive.
    std::shared_ptr<bool> _alive = std::make_shared<bool>(true);
};
} // namespace RType

#endif /* !SCRIPTENGINE_H_ */

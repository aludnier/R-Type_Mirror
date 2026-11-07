# Scripting games in Lua

The engine can run games written in Lua. Scripts come in two kinds:

- **entity scripts**, attached to one entity with a `ScriptComponent` (a list of
  script paths), for the behaviour of that entity;
- **game scripts**, loaded once, which spawn entities and add game-wide systems
  (waves, score, level flow).

The C++ side only owns the `Registry`, registers the component storages and
runs the frame.

The scripting layer lives in `engine/src/scripting` and is built as the
`engine_scripting` static library (`Eng::scripting`). It only depends on `engine_core`, so both
`r-type_server` and `r-type_client` can link it.

## 1. Host side (C++)

```cpp
#include "scripting/ScriptEngine.h"
#include "scripting/ScriptSystem.h"

RType::Registry registry;
registry.registerComponent<RType::Transform2D>();
registry.registerComponent<RType::Health>();

// Declared after the registry: it must be destroyed first.
RType::ScriptEngine scripts(registry);
RType::addScriptSystem(registry, scripts);   // like any other system
scripts.loadConfigFile("assets/config/scripts.json");

// An entity script attached from C++ (relative to the script directory).
registry.addComponent<RType::ScriptComponent>(player,
    RType::ScriptComponent {{"player.lua", "blink.lua"}});

while (running) {
    scripts.setDeltaTime(dt);   // seconds since the last frame
    registry.runSystems();      // runs C++ systems and script systems
}
```

`ScriptEngine` holds the Lua state and the script data; the work of each frame
is done by the system added with `addScriptSystem`, which runs the game-wide
script systems, then the `ScriptComponent` of every entity. The host decides
where it runs among the other systems.

`scripts.json` gives the script directory and lists the game scripts to run,
in order (`scripts` may be omitted). A relative `directory` is resolved from
the config file location:

```json
{ "directory": "../scripts", "scripts": ["enemies.lua", "waves.lua"] }
```

`ScriptEngine` API:

| Method | Description |
|---|---|
| `loadFile(path)` | Runs a file and remembers it for `reload()`. Returns `false` on error. |
| `loadString(code, name)` | Runs a chunk of code (not replayed by `reload()`). |
| `loadConfigFile(path)` | Runs every script listed in a JSON config. Throws on error. |
| `setScriptDirectory(dir)` | Directory relative `ScriptComponent` paths are read from. |
| `reload()` | Drops the script systems, runs the files again and reloads entity scripts (hot reload). Entities and `world.data` are kept. |
| `setDeltaTime(dt)` | Delta time passed to the script systems on the next `runSystems()`. |
| `bindComponent<C>(name)` | Makes a C++ component usable from scripts. |
| `lastError()` | Last script error, empty if none. |
| `lua()` | The underlying `sol::state`, to bind extra types. |

### Exposing a new component

A module or the game exposes its own component in two lines: the Lua type
(constructors, fields), then the binding used by `world.*`:

```cpp
scripts.lua().new_usertype<Score>("Score",
    sol::constructors<Score(), Score(int)>(), "value", &Score::value);
scripts.bindComponent<Score>("Score");
```

## 2. Script side (Lua)

### Entity scripts

An entity script returns a table with optional `on_start(e)`, called once when
the script starts on the entity, and `on_update(e, dt)`, called every frame.
Each file is loaded once and shared by every entity using it, so per-entity
state goes in `world.data(e)`:

```lua
-- enemy.lua
local enemy = {}

function enemy.on_start(e)
    world.data(e).kind = "enemy"
end

function enemy.on_update(e, dt)
    local t = world.get(e, "Transform2D")
    t.position.x = t.position.x - 200 * dt
end

return enemy
```

Attached with `world.add(e, "Script", {"enemy.lua", "blink.lua"})` (or a single
path string), or with a `ScriptComponent` from C++. `world.get(e, "Script")`
returns a copy of the paths.

### The world API

Entities are integers. Components are accessed by name.

| Function | Description |
|---|---|
| `world.spawn()` | Creates an entity and returns its id. |
| `world.kill(e)` | Destroys `e` at the end of the frame. |
| `world.add(e, name, value)` | Adds a component (`value` may be omitted for a default one) and returns it. |
| `world.get(e, name)` | Returns the component, or `nil`. |
| `world.has(e, name)` | Tells whether `e` has the component. |
| `world.remove(e, name)` | Removes the component. |
| `world.each({names...}, fn)` | Calls `fn(e, c1, c2, ...)` for every entity having all the components. |
| `world.data(e)` | Per-entity Lua table, to store script state without a C++ component. |
| `world.add_system(name, fn)` | Adds a system; `fn(dt)` runs on every `Registry::runSystems()`. |

Components returned by `world.get`, `world.add` and `world.each` are
references: `t.position.x = 10` changes the component in the registry. Do not
keep them across frames, store the entity id instead.

Bound types: `Vector2` (`x`, `y`, `+`, `-`, `* number`), `Transform2D`
(`position`, `scale`, `rotation`), `Health` (`current`, `max`, `is_dead()`,
`take_damage(n)`), `Hitbox` (`size`), `Name` (`value`), `Sprite` (`asset_id`,
`rect_left`, `rect_top`, `rect_width`, `rect_height`, `origin_x`, `origin_y`,
`r`, `g`, `b`, `a`, `z_index`; `Sprite.new(asset_id, z_index)`), `Controllable` (`action_mask`,
`is_active(Action.X)`, `set_active(Action.X, bool)`) and the `Action` table,
which is empty until the game fills it with `ScriptEngine::defineAction()`.

A complete example is in [`engine/scripts/examples/waves.lua`](../engine/scripts/examples/waves.lua)
(game script spawning waves) and [`engine/scripts/examples/enemy.lua`](../engine/scripts/examples/enemy.lua)
(entity script of each enemy).

## 3. Safety

- **Sandbox**: only the `base`, `math`, `string`, `table`, `coroutine` and
  `utf8` libraries are opened. `io`, `os`, `package`, `debug`, `require`,
  `dofile`, `loadfile` and `load` are not available, so a script cannot read
  or write files, run commands or load precompiled bytecode.
- **Errors never crash the host**: every script call is protected. A load error
  makes `loadFile` return `false`; a system that raises an error is disabled
  and reported through `lastError()` instead of flooding the logs every frame.
  A failing entity script is disabled on that entity only.
  `reload()` enables it again.
- **Checked API**: sol2 is built with `SOL_ALL_SAFETIES_ON`, so wrong argument
  types, unknown components and negative entity ids raise Lua errors instead
  of undefined behaviour.

## 4. Why Lua (comparative study)

| Criteria | Lua 5.5 + sol2 | Python + pybind11 | AngelScript / ChaiScript | JavaScript (QuickJS) |
|---|---|---|---|---|
| Embedding | Designed to be embedded; ~300 KB C library | Designed to be extended; embedding drags the whole CPython runtime and its standard library | Designed to be embedded | Embeddable |
| Binary size / runtime | Tiny | Large (tens of MB with the stdlib to ship) | Small | Small |
| Several states in one process | Yes, one `lua_State` per game / per server | One interpreter per process (sub-interpreters are limited) | Yes | Yes |
| Threads | No global lock, the state belongs to the game loop thread | GIL: one Python thread at a time, conflicts with the multithreaded server | No global lock | No global lock |
| Sandboxing | Choose the opened libraries | Not possible in a reliable way | Possible | Possible |
| C++ binding | sol2: header only, C++17+, type checks | pybind11: very good | Native for AngelScript, slow for ChaiScript | Manual C API |
| Packaging | `lua` and `sol2` in vcpkg, works the same on Linux and Windows | `python3` in vcpkg is heavy and the runtime must be shipped with the binaries | AngelScript in vcpkg, ChaiScript is unmaintained | `quickjs` in vcpkg, few C++ wrappers |
| Game industry use | Standard (World of Warcraft, Roblox (Luau), Factorio, Love2D, Defold...) | Tools and editors (Blender, Maya), rarely in-game | Niche | Niche |

Lua is chosen because it is made to live inside a C++ host: it is small, has no
global lock, can be sandboxed, and both its packages are available in vcpkg on
every platform we target. Python would be more familiar, but the GIL, the
runtime size and the lack of sandboxing are real problems for a multithreaded
game server that must not trust its content. LuaJIT was considered for speed;
it is stuck on the Lua 5.1 language and the game logic is not CPU-bound, so the
standard Lua interpreter (5.5, from the vcpkg baseline) is used.

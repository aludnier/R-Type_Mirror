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
| `setKillHandler(fn)` | Makes `world.kill(e)` call `fn(entity)` instead of `Registry::killEntity()` (e.g. a server that must tell its clients). `fn` kills the entity itself; an empty function restores the default. |
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

### Bindings of a dynamic module

Lua is linked statically (vcpkg `liblua.a`), so a module library must never
touch the host's Lua state: it would run its own copy of Lua on it. A module
ships its bindings as a small static library next to its sources, linked by
the hosts that run scripts, and the host calls them once the `PluginManager`
loaded the module (its component storages exist from then on):

```cpp
#include "Physics2DBindings.h"   // target Eng::physics2d_scripting

if (plugins.isLoaded(RType::Physics2D::MODULE_NAME))
    RType::Physics2D::bindScripting(scripts);
```

To add bindings to another module, copy `engine/modules/physics2D/scripting`:
a `bindScripting(ScriptEngine &)` function, built as a `<module>_scripting`
static library that links `engine_scripting` and the module's headers. The
module's shared library does not change. `r-type_server` binds physics2D when
`game.json` loads it (`GameServer::isModuleLoaded()`).

The game components have the same kind of function, `Game::bindGameComponents(scripts)`
(target `r-type_game_scripting`).

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
| `world.kill(e)` | Destroys `e` at the end of the frame (or calls the host's kill handler, see `setKillHandler`). |
| `world.add(e, name, value)` | Adds a component (`value` may be omitted for a default one) and returns it. |
| `world.get(e, name)` | Returns the component, or `nil`. |
| `world.has(e, name)` | Tells whether `e` has the component. |
| `world.remove(e, name)` | Removes the component. |
| `world.each({names...}, fn)` | Calls `fn(e, c1, c2, ...)` for every entity having all the components. |
| `world.data(e)` | Per-entity Lua table, to store script state without a C++ component. |
| `world.add_system(name, fn)` | Adds a system; `fn(dt)` runs on every `Registry::runSystems()`. |

`world.add` registers the component storage if the C++ side did not; before
that, `world.has` is false and `world.get` returns `nil`.

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

In the editor, the actions come from the second argument, a file of key codes
(GLFW codes, like `RType::Key`):

```json
{"bindings": {"MoveRight": [68, 262], "Shoot": [32]}}
```

```sh
editor config/assets.json config/input.json
```

During Play, give the entity a `Controllable` and read it in its script:

```lua
local input = world.get(id, "Controllable")
if input:is_active(Action.MoveRight) then
    -- ...
end
```

Module and game components, once their bindings are called (section 1):

| Component | Lua fields and methods |
|---|---|
| `Rigidbody2D` | `velocity`, `mass`, `drag`, `gravity_scale`, `is_kinematic`, `is_static`, `add_force(v)`, `add_impulse(v)`, `force()`, `clear_force()` |
| `BoxCollider2D` | `size`, `offset`, `is_trigger`, `layer`, `mask`; `BoxCollider2D.new(size)` |
| `CircleCollider2D` | `radius`, `offset`, `is_trigger`, `layer`, `mask`; `CircleCollider2D.new(radius)` |
| `PhysicsSettings2D` | `gravity`, `dt`, `bounciness` |
| `CollisionEvents2D` | `events`: collisions of the current frame, read-only. Each has `other` (entity id), `is_trigger`, `normal` and `kind` (`CollisionKind.Enter` or `CollisionKind.Exit`) |
| `Enemy` (game) | `enemy_type`, `shoot_timer`, `shoot_interval`, `score_value`, `wave_frequency`, `wave_amplitude`, `initial_y`, `elapsed_time`; `Enemy.new(type, interval, score, y)` |
| `Projectile` (game) | `is_player_bullet`, `damage`, `is_charged_beam`, `speed`; `Projectile.new(is_player, damage, charged, speed)` |
| `Player` (game) | `score`, `lives`, `shoot_cooldown`, `charge_time`, `is_charging`, `has_force_pod`; `Player.new(lives, has_force_pod)` |
| `ForcePod` (game) | `owner_id`, `offset_x`, `offset_y`, `level`; `ForcePod.new(owner, x, y, level)` |

The physics systems fill `CollisionEvents2D` every frame, so a script system
added after the module reads the collisions of this frame:

```lua
for _, event in ipairs(world.get(e, "CollisionEvents2D").events) do
    if event.kind == CollisionKind.Enter then
        world.get(event.other, "Health"):take_damage(1)
    end
end
```

On the server, `Enemy` is the server's own component (`base_y`, `elapsed`,
`shoot_timer`, section 3): the game components are not bound there.

A complete example is in [`engine/scripts/examples/waves.lua`](../engine/scripts/examples/waves.lua)
(game script spawning waves) and [`engine/scripts/examples/enemy.lua`](../engine/scripts/examples/enemy.lua)
(entity script of each enemy).

## 3. The R-Type server scripts

`r-type_server` runs the enemies from Lua: the waves and the Bydo behaviour
are scripts in [`server/scripts`](../server/scripts), listed in
`config/scripts.json`, which the server reads next to `game.json` (the copy in
the build tree uses the same layout):

```json
{ "directory": "../server/scripts", "scripts": ["waves.lua", "collision.lua", "cleanup.lua"] }
```

- [`waves.lua`](../server/scripts/waves.lua), game script: the levels. A
  `world.add_system` spawns an enemy past the right edge at a random height
  (every 1.5 s at level 1, a gunner every 4 enemies) for 45 s, then the
  boss. When the boss is shot down, the next level starts with more and
  faster enemies (`rtype.level`, `rtype.speed_scale`). When no player is
  left, it goes back to level 1 and waits for one.
- [`bydo.lua`](../server/scripts/bydo.lua), entity script of the Bydo
  scouts: flies left on a sine wave. Scouts do not shoot.
- [`gunner.lua`](../server/scripts/gunner.lua), entity script of the
  gunners: flies left slowly, drifting up and down, and shoots an
  `EnemyBullet` every 1.5 s once on screen.
- [`boss.lua`](../server/scripts/boss.lua), entity script of the boss: comes
  in from the right, moves up and down and shoots a fan of 5 bullets. Its
  health is 60 per level.
- [`walker.lua`](../server/scripts/walker.lua), entity script of the
  walkers: walks left on the ground and shoots up and to the left every
  2.2 s. `waves.lua` sends one every 6 s at level 1, more often after.
- `collision.lua` resolves the hits: a player bullet damages an enemy, an
  enemy or its bullet makes a player lose a life (`rtype.hit_player`). A
  shot down enemy drops a `PowerUp` 15 % of the time; the player who touches
  it gets the double shot (`rtype.power_up`).
- `cleanup.lua` removes what left the screen.

Every tick, after the player weapons, `RTypeServer` sets the delta time and
runs the scripts, then moves the projectiles, resolves the hits, removes what
left the screen and spawns what was requested. The scripts change the
components of entities the server already replicates, so their positions reach
the clients like any other entity.

On top of the `world` API, the server gives the scripts:

| Name | Description |
|---|---|
| `rtype.spawn(kind, x, y [, vx, vy])` | Spawns a replicated `"Enemy"` (Bydo scout), `"Gunner"`, `"Boss"`, `"Walker"`, `"PowerUp"`, `"EnemyBullet"`, `"PlayerLaser"` or `"ChargedBeam"` at the end of the tick (nothing is returned). An enemy gets its health, hitbox and entity script; a projectile its speed and damage, or the velocity `vx, vy` if given. |
| `rtype.hit_player(e)` | The player `e` loses a life and respawns, unless it is invulnerable after its last hit; without lives left its ship is despawned. |
| `rtype.power_up(e)` | The player `e` gets the double shot, until it loses a life. |
| `rtype.level`, `rtype.speed_scale` | Current level (from 1) and the speed factor of the enemies, set by `waves.lua`. |
| `rtype.SCREEN_WIDTH`, `rtype.SCREEN_HEIGHT` | Size of the play area, in pixels (1920 x 1080). |
| `rtype.GROUND_TOP`, `rtype.GROUND_SCROLL_SPEED` | Height of the top of the ground (960) and its scrolling speed (120 pixels/s). |
| `Enemy` component | State of an enemy: `base_y` (spawn height), `elapsed` and `shoot_timer` (seconds). |
| `Boss` component | Marks the boss, so `waves.lua` can tell when it is shot down. |
| `PowerUp` component | Marks a pickup; `collision.lua` gives it to the player who touches it. |

`world.kill(e)` goes through the server's kill handler: a replicated entity is
also removed on the clients (`ENTITY_DESPAWN`). Random numbers come from
`math.random`. A script error is printed on stderr and disables that script
only; the server keeps running. A script that cannot be loaded at startup stops
the server with an error.

## 4. Safety

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

## 5. Why Lua (comparative study)

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

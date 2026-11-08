# Technical and comparative study

This document explains the main technical choices of the project. Each choice
is compared with the alternatives we considered, and we say what is
implemented today and what is only planned.

The study covers:

1. [Language and build](#1-language-and-build)
2. [Algorithms, data structures and design patterns](#2-algorithms-data-structures-and-design-patterns)
3. [Network](#3-network)
4. [Storage](#4-storage)
5. [Security](#5-security)

The choice of the scripting language (Lua against Python, AngelScript and
JavaScript) is studied in
[Scripting games in Lua, section 5](scripting.md#5-why-lua-comparative-study).

## 1. Language and build

### 1.1 C++23

| Criteria | C++23 | Rust | C# (Unity, Godot .NET) | Java |
|---|---|---|---|---|
| Control over memory layout | Full: we choose how components are stored | Full | Limited, managed heap | Limited, managed heap |
| Garbage collector pauses | None | None | Yes | Yes |
| Game libraries | SFML, ImGui, Asio, sol2, ... | Fewer, younger bindings | Through the engine | Few (LibGDX) |
| Load shared libraries at run time | `dlopen` / `LoadLibrary`, C ABI | Possible, no stable ABI | Assemblies | Class loaders |
| Team knowledge | Whole team, used since first year | Nobody | Partial | Partial |
| Allowed by the subject | Yes, required | No | No | No |

The subject requires C++, but it would still be our choice. The engine stores
components in contiguous arrays and runs at a fixed tick rate: we need to
control memory layout and have no garbage collector pauses. C++23 also gives us
features that make the engine code safer and shorter:

- **Concepts** check templates at compile time. `Component` and
  `ComponentStorage` (in `engine/src/ecs/Component.h`) reject a wrong type
  with a clear error, and `WireInteger` (in `engine/src/network/Protocol.h`)
  only lets integers and enums be written on the wire.
- **`std::span`, `std::optional`, ranges** describe buffers and missing values
  without raw pointers: a decoded packet is a `std::optional<Packet>`, a
  missing component is an empty `std::optional<C>`.
- **`std::any` and `std::type_index`** let the `Registry` hold storages of
  any component type without a common base class.

### 1.2 CMake and vcpkg

| Criteria | vcpkg (chosen) | Conan | CMake CPM |
|---|---|---|---|
| Install | Git submodule, one bootstrap script | Python package, profiles to set up | One CMake file |
| Version pinning | `builtin-baseline` in `vcpkg.json`: one commit pins every library | Lock files | Git tag per dependency |
| Binary cache | Yes, used by the CI | Yes | No, rebuilds from sources |
| Windows and Linux | Same manifest | Same recipe | Depends on each library's CMake |
| Our dependencies | All available (`asio`, `nlohmann-json`, `sfml`, `imgui-sfml`, `catch2`, `lua`, `sol2`, `glfw3`, `glad`, `stb`, `imgui`) | Most | Libraries without CMake support need manual work |

The project first used CPM. We moved to vcpkg because CPM builds every
dependency from sources in each new build folder, with no binary cache, and
each library must provide a usable CMake build. With vcpkg the `vcpkg.json`
manifest lists the dependencies, the `builtin-baseline` pins their versions,
and the CI keeps a binary cache (`.github/workflows/warmup-vcpkg.yml`).
Nothing has to be installed on the system except a compiler and CMake.

### 1.3 Graphics and windowing

The client uses **SFML 3** for the window, input, 2D drawing and audio, and
**Dear ImGui** (through ImGui-SFML) for menus and the connection dialog. The
engine editor uses ImGui with docking on top of GLFW and OpenGL.

| Criteria | SFML 3 | SDL3 | raylib |
|---|---|---|---|
| API | C++ classes (`sf::Sprite`, `sf::Sound`) | C functions | C functions |
| 2D drawing, audio, input | Included | Included (rendering is basic) | Included |
| ImGui backend | ImGui-SFML | Official | rlImGui (community) |
| In vcpkg | Yes | Yes | Yes |

SFML fits a 2D game written in C++ and the team already knew it. The engine
does not depend on SFML: only `engine_client` and the game link it, and the
server never does (see the README).

## 2. Algorithms, data structures and design patterns

### 2.1 Entity Component System

The engine is built around an Entity Component System (ECS): an entity is only
an id, components are plain data, and systems are functions that run on every
entity that has a given set of components.

| Criteria | ECS (chosen) | Inheritance tree (`Enemy : Ship : GameObject`) | Component objects (Unity `GameObject` + `MonoBehaviour`) |
|---|---|---|---|
| Add a behaviour to an entity | Add a component | Change the class tree | Add a component |
| Memory layout | One array per component type | Objects scattered on the heap | Objects scattered on the heap |
| Logic | In systems, run in a known order | In virtual methods | In each component |
| Network replication | Send components, they are plain data | Each class serialises itself | Each component serialises itself |
| Dynamic modules | A module registers new components and systems | Needs the classes at compile time | Possible |

Inheritance does not scale for a game where a "player ship" can lose its
controls, an enemy can become controllable in the editor, or a module adds
physics to any entity. With an ECS, all these are just components. It also
matches our network model: the server sends components, and the client applies
them to its own registry.

### 2.2 How the ECS is implemented

**Component storage: sparse arrays.** The `Registry`
(`engine/src/ecs/Registry.h`) keeps one storage per component type, a
`SparseArray<C>` (`engine/src/ecs/SparseArray.h`). It is an
`std::vector<std::optional<C>>` indexed by entity id: getting the component of
an entity is a single index, in O(1), and the components of one type are
contiguous in memory.

| Storage | Get a component | Iterate on one type | Memory | Chosen |
|---|---|---|---|---|
| `vector<optional<C>>` indexed by id (sparse array) | O(1), one index | Contiguous, skips empty slots | One slot per entity id, even without the component | Yes |
| Packed sparse set (dense array + index) | O(1), two indexes | Contiguous, no holes | Smaller for rare components | Possible later |
| Archetypes (one table per set of components) | Find the table first | Fastest for many components | Moving entities between tables | No: complex for our entity count |
| `unordered_map<Entity, C>` | O(1) average, hashing | Not contiguous | Overhead per entry | No |

Our games have at most a few thousand entities, so the memory taken by empty
slots is small. The `ComponentStorage` concept lets a component choose another
storage later (through `storage_for<C>`) without changing the systems.

**Type erasure for the registry.** The registry does not know the component
types at compile time: modules add new ones at run time. Storages are kept in
an `std::unordered_map<std::type_index, RegistryEntry>`, where the entry holds
the storage in an `std::any` and an "erase" function created by
`registerComponent<C>()`. Destroying an entity calls each erase function, so
the registry can clean storages of types it does not know.

**Iteration: zippers.** `Zipper` (`engine/src/ecs/Zipper.h`) walks several
storages at the same time and only stops on indexes where every storage has a
value. `IndexedZipper` also gives the entity index. A system reads like this:

```cpp
for (auto [transform, velocity] : Zipper(transforms, velocities))
    transform.position += velocity.value * dt;
```

The zipper stops at the end of the shortest storage, so the cost is
proportional to the number of entity slots, not to the number of entities that
match. With the entity counts of a shoot'em up this is fast enough, and much
simpler than keeping per-query entity lists up to date.

**Deferred destruction.** `killEntity()` only adds the entity to a list. The
components are removed at the end of `runSystems()` (`killAllEntities()`). A
system can kill an entity while another system is iterating on its storage,
without invalidating the iteration, and the entity stays valid until the end of
the frame. The list of live entities is sorted, since ids only grow, so
`isAlive()` is a binary search.

**Systems as functions.** A system is any callable that takes the storages it
needs: `registry.addSystem<Transform2D, Velocity>(f)`. Systems run in the order
they were added. There is no `ISystem` base class: lambdas, free functions and
objects with `operator()` all work, and the storages a system uses are visible
in its signature.

### 2.3 Fixed time step

Gameplay must not depend on the speed of the machine. Both the server
(`GameServer::run`) and the client (`engine/src/time/FixedStep.h`) use an
accumulator: the time elapsed since the last frame is added to a counter, and
the simulation runs one tick for each full step in it (60 per second by
default). After a long freeze, at most 5 ticks are run in one frame, so the
game slows down instead of trying to catch up forever. The server sleeps until
the next tick is due instead of spinning on the CPU.

| Loop | Same result on every machine | CPU use | Chosen |
|---|---|---|---|
| Variable step (`update(dt)`) | No: physics and collisions depend on `dt` | Low | No |
| Fixed step with accumulator | Yes | Low with sleep | Yes |
| Fixed step, busy wait | Yes | One core at 100 % | No |

### 2.4 Design patterns

| Pattern | Where | Why |
|---|---|---|
| Entity Component System | `engine/src/ecs/` | See 2.1 |
| Plugin (dynamic loading) | `PluginManager`, `DLLoader` | Modules (physics, lifetime, environment) are shared libraries loaded at run time from a list in the config, and can be replaced without rebuilding the game |
| Topological sort | `PluginManager::resolveLoadOrder` | A module lists its dependencies; a depth-first search registers dependencies first and detects cycles. If a module fails, the ones loaded in the same call are rolled back |
| Producer / consumer | `NetworkThread` + `ThreadSafeQueue` | The network thread receives, the game thread consumes (see 3.3) |
| Registry of factories | `AssetLoaderRegistry` | Each asset type (`Texture`, `Sound`, ...) has a loader registered by name, so modules and games add their own types |
| Reflection | `engine/src/reflection/` | Components describe their fields once; the editor inspector and the scene files use that description |
| Facade | `GameApp` (`engine/src/game/`) | One object gives a game the window, inputs, assets and registry |

## 3. Network

### 3.1 UDP, not TCP

The subject requires UDP for the in-game traffic. It is also the right choice
for an action game:

| Criteria | UDP (chosen) | TCP |
|---|---|---|
| A lost packet | Lost, the next state replaces it | Every later packet waits for the resend (head-of-line blocking) |
| Latency under 2 % loss | Constant | Spikes of several round trips |
| Message boundaries | One datagram is one packet | Byte stream, framing needed |
| Reliability | Built by us, only where needed | Always, for everything |
| Connection state | Built by us (sessions, timeouts) | Given by the OS |

Most of our traffic is state: positions are sent again and again, and an old
position is useless once a newer one arrived. With TCP, one lost packet would
freeze every later update until it is resent. With UDP, we keep the newest
state and only a few messages (connection, spawn, despawn) need to arrive.

### 3.2 Asio

| Criteria | Asio standalone (chosen) | Boost.Asio | Raw BSD sockets / Winsock | ENet |
|---|---|---|---|---|
| Linux and Windows | Same code | Same code | Two code paths | Same code |
| Dependency | Header only, no Boost | All of Boost | None | Small C library |
| Async I/O | `io_context` | `io_context` | Manual `select`/`poll` | Event loop |
| Protocol | Ours | Ours | Ours | ENet's own reliability layer |

The subject allows Asio. The standalone version gives portable UDP sockets
without the rest of Boost. ENet was not chosen: its protocol is its own, and
the subject asks us to design and document ours.

### 3.3 Threads

The server is multithreaded:

- The **network thread** (`NetworkThread`, `engine/src/network/`) runs the
  Asio `io_context`. It receives datagrams, checks their header, and pushes
  valid packets into a bounded `ThreadSafeQueue`. Sends are posted to the same
  `io_context`, so only this thread touches the socket.
- The **game thread** (`GameServer::run`) pops the received packets at the
  start of each tick, runs the systems, then sends the updates.

The game thread never waits on the network: if the queue is full, new packets
are dropped and counted, instead of blocking or growing memory. A slow or
malicious client cannot freeze the game for the others. The client uses the
same `NetworkThread`, so the window keeps drawing while packets arrive.

### 3.4 Protocol design

The protocol is specified in [the network protocol](protocol.md). Its main
choices:

- **Binary, little-endian, fixed header.** Every packet starts with a 15-byte
  header: magic number `RT`, version, message type, flags, sequence, ack,
  ack bits and payload size. Binary is much smaller than JSON or text, and the
  size check catches truncated datagrams.
- **At most 1200 bytes per datagram.** This stays under the usual Internet MTU,
  so packets are never fragmented by IP (a lost fragment loses the whole
  datagram). 60 entity positions fit in one packet.
- **Server authoritative.** Clients only send their inputs (a 32-bit action
  mask). The server runs the game and sends the result, so a modified client
  cannot move faster or teleport.
- **Only changes are sent.** The server sends a position only if it changed
  since the last one sent, and every entity once per second. This keeps the
  bandwidth low (the subject's test case is 5 KB/s).
- **Acknowledgements in every header.** `ack` and `ack_bits` tell the other
  side which of its last 33 packets arrived, without sending extra packets.
- **Client-side prediction.** The client moves its own ship from its inputs
  right away, instead of waiting a round trip, and only snaps it to the
  server's position when they differ by more than 40 pixels. Projectiles that
  move in a straight line are sent once with their velocity: the clients move
  them, and the server does not send their positions again, except in the
  once-per-second refresh.

### 3.5 Compared with other models

| Model | Bandwidth | Cheating | Latency felt by the player | Chosen |
|---|---|---|---|---|
| Server authoritative, state updates, local prediction | Medium | Server decides | Low for own ship | Yes |
| Lockstep (send inputs only, everyone simulates) | Very low | Desync if a client cheats | Waits for the slowest player | No |
| Peer to peer | Low | Each peer decides | Low | No, the subject requires a server |

## 4. Storage

### 4.1 What the project stores

The game has no database and no save files. Everything stored on disk is
configuration or content written by the developers, read when a program
starts:

| Data | Format | File | Read by |
|---|---|---|---|
| Server and game settings (name, port, players, tick rate, modules) | JSON | `config/game.json` | `Configuration`, `GameServer`, `PluginManager` |
| Assets (name, type, path) | JSON | `config/assets.json` | `AssetManager` |
| Server scripts to run | JSON | `config/scripts.json` | `RTypeScripting` |
| UI documents | JSON | `config/ui/*.json` | `UiManager` (see [UI documents](ui.md)) |
| Input bindings (`{"bindings": {"Action": [keys]}}`) | JSON | given to the editor | `InputManager::loadActions` |
| Editor scenes (entities and reflected components) | JSON | saved from the editor | `Scene::toJson` / `fromJson` (`engine/src/scene/`) |
| Textures, sounds, fonts | PNG, WAV/OGG, TTF | `assets/` | `ClientAssetManager`, through loaders registered per type |
| Game scripts | Lua source | `server/scripts/`, `engine/scripts/` | `ScriptEngine` |

All the game state lives in memory, in the server's registry. When the server
stops, the game ends. There is no account, score history or saved game to
keep, so the project does not need a database yet.

### 4.2 Format comparison

| Criteria | JSON (chosen) | Binary format (custom, FlatBuffers) | SQLite | YAML / TOML | XML |
|---|---|---|---|---|---|
| Edit by hand and review in a pull request | Yes | No | No (needs a tool) | Yes | Yes, verbose |
| Read speed | Fast enough for a few KB at start-up | Fastest | Fast queries | Slower | Slow |
| Persistence and reliability | A file written at once; a crash during a write can leave a half file | Same | Transactions, journal: survives crashes | Like JSON | Like JSON |
| Concurrent writers | No | No | Yes, with locks | No | No |
| Schema | Checked in code when loading | In the schema | Tables and types | In code | XSD |
| Library | `nlohmann-json`, already used | Generator or our own code | `sqlite3` | Extra library | Extra library |
| Nested data (UI trees, scenes) | Natural | Natural | Needs several tables | Natural | Natural |

JSON fits our data: small files, written by people, nested (UI documents,
scenes), read once when the program starts. The files are reviewed in pull
requests like code, and the editor writes scenes in the same format a
developer can fix by hand. `nlohmann-json` throws on a syntax error, and each
loader checks the fields it needs, so a wrong file stops the program with a
message that names it (for example a UI include cycle names the file).

A binary format is used where size and speed matter: on the network (see 3.4),
not on disk.

SQLite would be the right tool for data that changes during the game and must
survive a crash: player accounts, high scores, a server that keeps a world
between sessions. None of these exist today. If we add them, they would go in
a SQLite file on the server only, since its transactions protect against a
crash in the middle of a write, and it needs no separate database server.

### 4.3 Storage constraints

- **Paths are relative to the config file**, not to the working directory, so
  the programs can be started from anywhere. `game.json` names the module
  directory (`plugin_directory`), and `scripts.json` the scripts directory.
- **Assets are loaded once.** `ClientAssetManager` keeps a cache by name: the
  same texture is not read twice, and the file is read again only if the
  config points the name to another path or type.
- **Content is not trusted blindly.** Lua scripts run in a sandbox without file
  or process access (see [Scripting, section 4](scripting.md#4-safety)), and
  modules are only loaded from the directory named in the config.

## 5. Security

### 5.1 Main risks of each technology

| Technology | Main risks | What we do |
|---|---|---|
| C++ | Buffer overflows, use after free, undefined behaviour | No raw buffers in the network code: `std::span`, `std::vector`, bounds-checked `ByteReader`. Ownership with RAII and `std::unique_ptr`. Warnings and `clang-tidy` in the CI |
| UDP | Spoofed source address, floods, amplification (a small request that triggers a big answer) | Packets from an unknown address are dropped unless they are `ASK_CONNECT`. The answer to `ASK_CONNECT` is small. The receive queue is bounded |
| Our binary protocol | Malformed packets that crash the server or make it allocate too much | See 5.2 |
| Dynamic modules | A shared library runs with the rights of the game | Only libraries in the configured directory are loaded; modules must export the expected entry point and API version |
| Lua scripts | A script reading files or running commands | Sandbox: `io`, `os`, `package`, `debug`, `load` are not available; errors are caught |
| JSON files | Wrong or hostile content | Parsed by `nlohmann-json`, which throws on errors instead of reading out of bounds; fields are checked |
| Dependencies | A vulnerable library version | Versions pinned by the vcpkg baseline (see 5.4) |

### 5.2 Data integrity on the network

A datagram goes through these checks before the game sees it:

1. **Size.** Datagrams shorter than the 15-byte header or longer than 1200
   bytes are dropped. The receive buffer has a fixed size, so a big datagram
   cannot make the server allocate more.
2. **Header.** The magic number must be `RT`, and the payload size in the
   header must equal the bytes received. A truncated or padded datagram is
   dropped.
3. **Version.** A client with another protocol version is refused with
   `FORCE_DISCONNECT` (`BAD_VERSION`).
4. **Session.** Except `ASK_CONNECT`, packets are only accepted from the
   address of a connected client.
5. **Payload.** `ByteReader` checks every read against the remaining bytes.
   Reading too far does not throw or read out of bounds: the reader enters a
   failed state and the packet is dropped. Strings are limited to 255 bytes,
   list counts are 8-bit (so a packet never reserves more than 255 elements),
   and trailing bytes make the packet invalid.
6. **Sequence.** Sequence numbers wrap around after 65535; they are compared
   with `isSequenceMoreRecent`, so an old or duplicated packet does not move
   the acknowledgement state backwards.

Dropped datagrams are counted, and the server prints the count once per
second. These cases are covered by the tests in
`engine/tests/network_protocol_tests.cpp` and
`engine/tests/network_transport_tests.cpp` (invalid framing, truncated and
trailing bytes, unknown enum values, a count larger than the data, oversized
datagrams, full queue).

### 5.3 Known limits of version 1

We prefer to list what is not done:

- **No encryption and no authentication.** Packets are sent in clear, and a
  client is identified by its address and port. Anyone on the path can read
  the game state or spoof a connected client's address. For a LAN game without
  accounts this is acceptable; with accounts, we would add a key exchange at
  connection and encrypt with a library such as libsodium, or use DTLS.
- **Reliable resend is specified but not implemented.** The protocol
  ([section 7](protocol.md#7-sequencing-and-reliability)) describes resending
  reliable messages until they are acknowledged, and the reliable flag is set
  on these packets, but the code does not resend yet. A lost spawn or despawn
  is corrected only by later updates.
- **No rate limit on `ASK_CONNECT`.** The protocol says a server SHOULD limit
  connection requests per address; it is not implemented. The bounded queue
  limits the damage of a flood, but a flood can still fill the server's slots
  with fake players until they time out (5 seconds).

### 5.4 Security monitoring over time

What runs today:

- **CI on every pull request** to `Dev` and `main`: build and tests on Linux
  and Windows, and `cpp-linter` (clang-tidy and clang-format). A pull request
  whose tests fail is not merged.
- **Pinned dependencies.** The vcpkg `builtin-baseline` fixes the version of
  every library. An update is a visible change of one line in `vcpkg.json`,
  reviewed and tested like code.
- **Tests for hostile input** on the protocol, run in the CI (see 5.2).
- **Dropped packet counter** printed by the server, to notice a flood or a
  broken client.

Next steps, in order of value:

1. **Fuzz the packet decoder** with libFuzzer on `Packet::deserialize` and
   `readPayload`, and run it in the CI for a few minutes.
2. **Build the tests with AddressSanitizer and UndefinedBehaviorSanitizer** in
   one CI job, to catch memory errors the tests do not see.
3. **Update the vcpkg baseline regularly** (for example each month, through a
   Dependabot or Renovate pull request on the `vcpkg` submodule) and read the
   security advisories of our dependencies (SFML, Lua, Asio, nlohmann-json).
4. **Implement the protocol's rate limit and reliable resend**, then add
   authentication if the game gets accounts.

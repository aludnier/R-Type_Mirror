# R-Type

A networked multiplayer remake of R-Type, written in C++23 on top of our own
game engine. The server is authoritative: it runs the game, and the clients
send their controls and display what the server sends back over UDP.

## Project layout

The project builds three engine libraries and two executables. The server
never links the client library, so it never depends on the graphics library.

| Target | Kind | Contents | Used by |
|---|---|---|---|
| `engine_core` | static library | ECS, math, shared components and systems, network (Asio), modules | server and client |
| `engine_client` | static library | engine loop, input, later rendering and audio | client |
| `engine_server` | static library | server runtime: game loop, client sessions | server |
| `r-type_server` | executable | server entry point | |
| `r-type_client` | executable | client entry point (work in progress) | |
| `physics2d_module`, `example_module` | shared libraries | dynamically loaded modules | server and client |

| Folder | Contents |
|---|---|
| `game_engine/` | `engine_core`, `engine_client` and their tests |
| `server/` | `engine_server` and `r-type_server` |
| `game/` | `r-type_client` |
| `modules/` | dynamic modules (see `docs/writing_dynamic_modules.md`) |
| `docs/` | network protocol (`protocol.md`), architecture diagrams |

## Requirements

- CMake 3.21 or newer
- A C++23 compiler: GCC 13+ or Clang 17+ on Linux, Visual Studio 2022
  ("Desktop development with C++") on Windows
- Git

Nothing else needs to be installed: dependencies are fetched by the build.
Asio, SFML, ImGui-SFML comes from [vcpkg](https://github.com/microsoft/vcpkg) (a git submodule).

## Build

Clone with the submodules, then bootstrap vcpkg once:

```bash
git clone --recurse-submodules <repository-url>
cd r-type
./vcpkg/bootstrap-vcpkg.sh          # Windows: .\vcpkg\bootstrap-vcpkg.bat
```

If the repository was cloned without `--recurse-submodules`, run
`git submodule update --init` first.

Configure and build with a preset (`debug` or `release`):

```bash
cmake --preset release
cmake --build --preset release
```

The first configure is slower: vcpkg downloads and builds the dependencies.

### Building a single program

By default, everything is built. To build only one program and what it
needs, name its target:

    cmake --build --preset release --target r-type_server
    cmake --build --preset release --target r-type_client

## Run

```bash
./build/release/server/r-type_server [-p port]     # default port: 4242
./build/release/game/r-type_client
```

On Windows, Visual Studio puts the executables in a `Release` (or `Debug`)
subfolder, e.g. `build\release\server\Release\r-type_server.exe`.

The server stops cleanly on Ctrl+C: connected clients are told that it shuts
down.

## Tests

```bash
ctest --test-dir build/release --output-on-failure
```

On Windows, add the configuration: `ctest --test-dir build/release -C Release`.

## Documentation

- [Network protocol](docs/protocol.md): binary protocol between the server
  and the clients
- [Architecture diagrams](docs/architecture/README.md)
- [Writing dynamic modules](docs/writing_dynamic_modules.md)

## Authors

- Natan Pereira
- Ayrton Ludger-Crosnier
- Léandre Rocher
- Alen Sarsen

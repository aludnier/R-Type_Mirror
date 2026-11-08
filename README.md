# R-Type

A networked multiplayer remake of R-Type, written in C++23 on top of our own
game engine. The server is authoritative: it runs the game, and the clients
send their controls and display what the server sends back over UDP.

## Project layout

The project builds four engine libraries and two executables. The server
never links the client library, so it never depends on the graphics library.

| Target                               | Kind             | Contents                                                          | Used by           |
|--------------------------------------|------------------|-------------------------------------------------------------------|-------------------|
| `engine_core`                        | static library   | ECS, math, shared components and systems, network (Asio), modules | server and client |
| `engine_client`                      | static library   | engine loop, input, audio, later rendering                        | client            |
| `engine_server`                      | static library   | server runtime: game loop, client sessions                        | server            |
| `engine_scripting`                   | static library   | Lua scripting (sol2), see `docs/scripting.md`                     | server and client |
| `r-type_server`                      | executable       | server entry point                                                |                   |
| `r-type_client`                      | executable       | client entry point (work in progress)                             |                   |
| `editor`                             | executable       | scene editor of the engine (ImGui)                                |                   |
| `physics2d_module`, `example_module` | shared libraries | dynamically loaded modules                                        | server and client |

| Folder            | Contents                                                |
|-------------------|---------------------------------------------------------|
| `engine/`         | `engine_core`, `engine_client`, `engine_scripting` and their tests |
| `server/`         | `engine_server` and `r-type_server`                     |
| `game/`           | `r-type_client`                                         |
| `engine/modules/` | dynamic modules (see `docs/writing_dynamic_modules.md`) |
| `engine/scripts/` | example Lua scripts (see `docs/scripting.md`)           |
| `server/scripts/` | R-Type server game scripts (see `docs/scripting.md`)    |
| `docs/`           | documentation, published as a website (see below)      |
| `mkdocs/`         | configuration of the documentation website             |

## Requirements

- CMake 3.21 or newer
- A C++23 compiler: GCC 13+ or Clang 17+ on Linux, Visual Studio 2022 ("Desktop development with C++") on Windows
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

The first time is slower: vcpkg downloads and builds the dependencies.

### Building a single program

By default, everything is built. To build only one program and what it
needs, name its target:

    cmake --build --preset release --target r-type_server
    cmake --build --preset release --target r-type_client

## Run

```bash
./build/release/server/r-type_server [-p port]     # default port: 4242
./build/release/game/r-type_client
./build/release/engine/apps/editor/editor config/assets.json   # --help lists the arguments
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

The documentation is in [`docs/`](docs/) and is published as a website on
GitHub Pages: <https://aludnier.github.io/R-Type_Mirror/>.

- [Architecture](docs/architecture/README.md): the engine libraries and
  [diagrams](docs/architecture/diagrams.md)
- [Network protocol](docs/protocol.md): binary protocol between the server
  and the clients
- [Writing dynamic modules](docs/writing_dynamic_modules.md)
- [Scripting games in Lua](docs/scripting.md)
- [UI documents](docs/ui.md)
- [Technical and comparative study](docs/comparative_study.md): why we chose
  C++, vcpkg, an ECS, UDP and Asio, JSON, and how we handle security
- [Accessibility](docs/accessibility.md)
- [Contributing](docs/contributing.md): branches, commits, code style, tests

The site is built with MkDocs by `.github/workflows/docs.yml` and published on
every push to `main`. To build it locally, see
[Contributing](docs/contributing.md#documentation).

GitHub Pages must be enabled once by the repository owner: in the repository
settings, under **Pages**, set **Source** to **GitHub Actions**.

## Authors

- Natan Pereira
- Ayrton Ludger-Crosnier
- Léandre Rocher
- Alen Sarsen

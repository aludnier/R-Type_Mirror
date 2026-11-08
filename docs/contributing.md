# Contributing

This page explains how we work on the project: branches, commits, code style,
tests and documentation.

## Branches and pull requests

- `main` is the delivered version. The CI mirrors it to the Epitech
  repository.
- `Dev` is the integration branch. Every change goes through a pull request
  into `Dev`.
- Work on a feature branch created from `Dev`, named after its type:
  `feat/editor-modules`, `fix/projectile-sync-enemies`, `docs/...`.
- Open the pull request with the template in `.github/pull_request_template.md`
  (type of change and description), and link the issue it closes.
- Merge a pull request only when the CI passes, after another member has
  read it.

## Commits

We use [Conventional Commits](https://www.conventionalcommits.org/):

```text
type(scope): what the commit does
```

- `type`: `feat`, `fix`, `refactor`, `test`, `docs`, `style`, `ci`, `build`.
- `scope`: the part of the project, for example `ECS`, `network`, `server`,
  `editor`, `scripting`, `physics2d`.
- Prefer several small commits, each one building and passing the tests, to
  one big commit.

Examples from the history: `feat(input): added loadActions`,
`test(server): check the server scripts`,
`docs(scripting): list the module and game bindings`.

## Code style

- C++23, formatted with `clang-format` (`.clang-format`, based on the LLVM
  style with 4-space indentation). Format before committing:
  `clang-format -i <files>`.
- `clang-tidy` checks are listed in `.clang-tidy`. The CI runs both with
  `cpp-linter` on every pull request and posts its report as a comment on the
  pull request.
- Public classes and functions have a Doxygen comment (`@brief`, `@param`,
  `@return`, `@throws`) in their header.
- Code, comments and documentation are written in English.
- Namespaces: `RType` for the engine, `RType::Network` for the network code,
  `Game` for the R-Type client.

## Tests

Tests use [Catch2](https://github.com/catchorg/Catch2) and run with CTest:

```bash
cmake --build --preset debug
ctest --test-dir build/debug --output-on-failure
```

| Test executable | Folder | Covers |
|---|---|---|
| `game_engine_tests` | `engine/tests/` | ECS, network protocol and transport, modules, reflection, scenes, UI |
| `engine_server_tests` | `engine/tests/` | `GameServer` sending and sessions |
| `engine_scripting_tests` | `engine/tests/` | Lua scripting and bindings |
| `engine_editor_tests` | `engine/tests/` | Editor panels, scene files, Play mode |
| `r-type_game_tests` | `game/tests/` | Game bindings |
| `r-type_server_tests` | `server/tests/` | R-Type server scripts |

Add a test with every feature and every fixed bug. Add the new file to the
test executable of its folder in the `CMakeLists.txt`.

## Continuous integration

`.github/workflows/CICD.yml` runs on every pull request to `Dev` and `main`:

1. Warm up the vcpkg cache (Linux and Windows).
2. Build the engine and the game, and run the tests, on Linux and Windows.
3. Run the linter.
4. On `main`, mirror the repository to the Epitech repository.

`.github/workflows/docs.yml` builds this documentation site on every pull
request and publishes it on GitHub Pages on every push to `main`.

## Documentation

The documentation is written in Markdown in `docs/`, in English, and built
into a website with [MkDocs](https://www.mkdocs.org/) and the Material theme.
The site menu is set in `mkdocs.yml`: add a new page there.

Build it locally:

```bash
python3 -m venv .venv
.venv/bin/pip install -r docs/requirements.txt
.venv/bin/mkdocs serve          # http://127.0.0.1:8000, reloads on change
.venv/bin/mkdocs build --strict # what the CI runs
```

The architecture diagrams need the `plantuml` command (and Graphviz). Without
it, set `PLANTUML_SERVER=https://www.plantuml.com/plantuml` to render them
with the public PlantUML server.

When you write documentation:

- Write short sentences with common words.
- Use headings, and give every table a header row.
- Describe each diagram in text.
- Put commands and code in code blocks, not in images.
- Say what is implemented and what is only planned.

# Editor

The editor builds a scene without code: create entities, edit their
components, save the scene, and press Play to run it. It is built with the
engine (target `editor`) and does not know about R-Type.

## Run

```bash
./build/release/engine/apps/editor/editor [assets.json [input.json [game.json]]]
```

| Argument | Used for |
|---|---|
| `assets.json` | The assets of the game. Needed to show sprites and to save scenes, since assets are saved by name |
| `input.json` | The actions of the game and their keys. Edited in the Input panel, and given to the scripts in Play mode. The file may not exist yet: Save creates it |
| `game.json` | The game config. The modules in its `"plugins"` are loaded, so their components can be added in the inspector and their systems run in Play mode. Default: the build's `config/game.json` |

A file that cannot be loaded prints `warning: cannot load <file>: <reason>`,
and the editor starts without it.

## Panels

| Panel | What it does |
|---|---|
| Hierarchy | Lists the entities. Create one with the button; right-click an entity to rename or delete it; click it to show it in the inspector |
| Viewport | Draws the scene |
| Inspector | Edits the components of the selected entity: every reflected field, Add Component and Remove |
| Input | Edits the actions and their keys (see below) |

The menu bar has the **File** menu (New, Open, Save, Save As, with Ctrl+N,
Ctrl+O and Ctrl+S) and **Play** / **Stop**. Play saves the scene in memory and
runs its systems and Lua scripts; Stop puts the scene back as it was. The
panels cannot edit anything while the scene plays.

## Key bindings (Input panel)

A game reads actions, such as `MoveUp` or `Shoot`, not keys. The Input panel
sets which keys trigger each action:

- **Add action** creates an action. Each action takes a free bit of the
  32-bit action mask sent to the server, so a game has at most 32 actions.
- **Right-click an action** to rename or remove it. Renaming keeps its bit
  and its keys.
- **Bind...** waits for the next key pressed and binds it to the action. An
  action can have several keys (for example the arrows and WASD). Click
  **Cancel** to stop waiting.
- **x** next to a key unbinds it.
- **Save** writes the input file given on the command line, or asks for a
  path. **Save As...** writes another file, which becomes the one Save
  writes.

The file has the format `InputManager::loadActions` reads. Key codes are the
ones of `engine/src/library/Keys.h` (GLFW values); named inputs are strings:

```json
{
    "bindings": {
        "MoveUp": [265, 87],
        "Shoot": [32]
    }
}
```

The panel shows the keys by name (`Up`, `W`, `Space`). The R-Type client uses
SFML and does not read this file yet: its keys are set in its code.

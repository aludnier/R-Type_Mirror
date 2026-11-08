# Accessibility

This document lists what the game and the engine do for players with
disabilities, what they do not do yet, and what we plan. It follows three
groups of needs: physical and motor, audio and visual, mental and cognitive.
The last section is about the accessibility of this documentation.

Each point is marked:

- **Done**: in the code on the `Dev` branch.
- **Partial**: possible with the engine, but not offered to the player yet.
- **Planned**: not started.

## 1. Physical and motor

| Feature | Status | Details |
|---|---|---|
| Actions instead of keys | Done | The game never reads a key directly. It reads actions (`MoveUp`, `MoveDown`, `MoveLeft`, `MoveRight`, `Shoot`), and each action can be bound to several keys (`InputManager::bind`). Anything that produces key events, such as an adapted keyboard, a one-hand keyboard or a switch interface that emulates keys, can play |
| Two layouts by default | Done | The client binds both the arrow keys and WASD to movement, and Space to shoot, so a player can use the left or the right hand |
| Remappable bindings | Done | The R-Type client reads its keys from `config/inputs.json` (`{"bindings": {"MoveUp": [<key code>, ...]}}`, SFML key codes), so a player can change them without rebuilding. The [editor](editor.md#key-bindings-input-panel)'s Input panel edits and saves the same format for games made with the engine (GLFW key codes). Next step: show and change the bindings from the pause menu |
| No key mashing to charge | Done | The charged shot (wave cannon) is done by holding the fire key and releasing it, not by pressing it fast |
| Auto-fire | Planned | A normal shot needs one press and one release per shot. An option that fires while the key is held would help players who cannot tap quickly |
| Gamepad | Planned | Only keyboard events are turned into actions. Since the game works with actions, adding gamepad buttons and sticks means adding inputs in `InputManager`, without touching the game |
| Mouse not required in game | Done | The game is played with the keyboard only. The connection dialog uses the mouse or Tab to move between fields |
| Speed and difficulty | Planned | The game runs at a fixed tick rate on the server, the same for every player. A slower game speed or fewer enemies would be a server option (`tick_rate` is already in `config/game.json`, but changing it also changes the network rate, so a separate game speed setting is needed) |

## 2. Audio and visual

| Feature | Status | Details |
|---|---|---|
| Playable without sound | Done | Every game event is visible: ships, enemies, shots, the charge of the wave cannon (a bar in the bottom-left corner). No information is given by sound only |
| Visual cues for sounds | Done | The server plays a sound for each shot, explosion, hit, power-up and the boss's arrival, and each one has a visual cue: the shot itself, the enemy disappearing, the lives in the HUD going down, the ship respawning, the "BOSS" banner. The music has no information. Next step: optional subtitles for the sounds |
| Volume per channel | Partial | `AudioManager::setVolume` sets the master, music and sound effect volumes separately, so a player can lower the music and keep the effects. The client has no menu for it yet |
| Contrast | Done | The background is almost black (`#0A0A19`) and the sprites are bright, so ships, enemies and shots stay visible |
| Shapes, not only colours | Done | Players, enemies, the force pod and each kind of shot have their own sprite, so a colour-blind player can tell them apart by shape |
| Distinguish the four players | Done | Each ship is tinted by client id with Okabe-Ito colours, which stay distinct for the common types of colour blindness: player 1 keeps the original colours, then orange, yellow and purple. Next step: a number above each ship |
| Colour-blind friendly palette | Partial | The player tints are colour-blind safe. The enemy and shot sprites keep the colours of the original game; their shapes are what make them different |
| Text size | Partial | The menus use the default ImGui font at twice its size. An option to choose the scale would help more players with low vision |
| Screen reader | Not planned | Neither SFML nor ImGui exposes the UI to screen readers. The connection dialog is the only text input; it is short and the default values (`127.0.0.1`, port `4242`) work for a local game |
| Flashing | Done | The game has no flashing effects or fast light changes, which can trigger photosensitive epilepsy |

## 3. Mental and cognitive

| Feature | Status | Details |
|---|---|---|
| Few controls | Done | Four directions and one fire key. The charged shot uses the same key |
| Simple menus | Done | One connection dialog with three fields (host, port, name) and one button, then the game. Escape opens a menu to resume or disconnect, and it tells how to close it ("Press Escape to resume play") |
| Clear error messages | Done | When the connection fails or the server refuses it, the dialog shows why (server full, wrong version, timeout) instead of closing |
| Pause | Partial | Escape opens the menu, but the game keeps running on the server, since other players are in the same game. A real pause would need the server to pause the game for everyone, or a single-player mode |
| Tutorial | Planned | No tutorial yet. The controls should be shown on the connection screen and in the pause menu |
| Steady game speed | Done | The game runs at the same speed on every machine (fixed time step), so it does not suddenly speed up on a fast computer |

## 4. Accessibility of the documentation

| Point | What we do |
|---|---|
| Plain English | Short sentences, common words, one idea per sentence. Technical words are explained the first time they are used |
| Structure | Every page uses real headings, so screen readers and the site outline can jump between sections |
| Tables have headers | Every table has a header row |
| Diagrams have text | Every architecture diagram is also described in text, and its PlantUML source is readable as text |
| Code is text | Commands and code are in code blocks, never in images |
| Online site | The [documentation site](index.md) has search, a navigation menu, an outline of each page, a light and a dark theme, and works with the browser zoom |

## 5. Next steps, by priority

1. Show a number above each ship, in addition to its colour.
2. Show and change the key bindings from the pause menu.
3. Add an auto-fire option.
4. Add optional subtitles for the sounds, and a volume menu (master,
   music, effects).
5. Add a font scale option to the menus.
6. Add gamepad support in `InputManager`.

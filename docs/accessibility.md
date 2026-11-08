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
| Remappable bindings | Partial | `InputManager::loadConfigFile` and `loadActions` read bindings from a JSON file: `{"bindings": {"MoveUp": [<key code>, ...], "Shoot": [<key code>]}}`, where each key code is the one of the window library. The editor takes such a file as its second argument; the game client does not load one yet, so its bindings are fixed in the code. Next step: load a `bindings.json` next to the client and show the bindings in the pause menu |
| No key mashing to charge | Done | The charged shot (wave cannon) is done by holding the fire key and releasing it, not by pressing it fast |
| Auto-fire | Planned | A normal shot needs one press and one release per shot. An option that fires while the key is held would help players who cannot tap quickly |
| Gamepad | Planned | Only keyboard events are turned into actions. Since the game works with actions, adding gamepad buttons and sticks means adding inputs in `InputManager`, without touching the game |
| Mouse not required in game | Done | The game is played with the keyboard only. The connection dialog uses the mouse or Tab to move between fields |
| Speed and difficulty | Planned | The game runs at a fixed tick rate on the server, the same for every player. A slower game speed or fewer enemies would be a server option (`tick_rate` is already in `config/game.json`, but changing it also changes the network rate, so a separate game speed setting is needed) |

## 2. Audio and visual

| Feature | Status | Details |
|---|---|---|
| Playable without sound | Done | Every game event is visible: ships, enemies, shots, the charge of the wave cannon (a glow that grows in front of the ship). No information is given by sound only |
| Subtitles and visual cues for sounds | Planned | The engine plays sounds and music (`AudioManager`), and the server can ask the clients to play a sound with `PLAY_SOUND`, but the R-Type server does not send any sound yet. Each important sound (enemy wave, hit, death) must also get a visual cue or a subtitle |
| Volume per channel | Partial | `AudioManager::setVolume` sets the master, music and sound effect volumes separately, so a player can lower the music and keep the effects. The client has no menu for it yet |
| Contrast | Done | The background is almost black (`#0A0A19`). Players are blue, enemies are red, the force pod is orange, and every ship has a light outline, so shapes stay visible on the background |
| Shapes, not only colours | Done | Players, enemies and the force pod have different shapes, so a colour-blind player can tell them apart |
| Distinguish the four players | Planned | All player ships have the same colour today. Each player should have a colour from a colour-blind safe set (for example blue, orange, yellow and white, which stay distinct for the three common types of colour blindness) and a number above the ship |
| Colour-blind friendly palette | Partial | Blue against red/orange is readable for most colour-blind players, but red and orange (enemies and force pod) can look alike for protanopia and deuteranopia; the shapes and outlines are what make them different today |
| Text size | Planned | Menus use the default ImGui font. An option to scale the font (`ImGuiIO::FontGlobalScale`) would help players with low vision |
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

1. Give each player a colour from a colour-blind safe set and a number
   (also needed by the subject: the four players must be distinguishable).
2. Load the client's key bindings from a JSON file and show them in the menu.
3. Add an auto-fire option.
4. Add visual cues or subtitles for every sound the server sends, and a
   volume menu (master, music, effects).
5. Add a font scale option to the menus.
6. Add gamepad support in `InputManager`.

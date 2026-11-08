# UI documents

UI documents are JSON files loaded by `RType::UI::UiManager`. A document has a
`name` and an `elements` array. The engine provides `panel`, `text`, `image`,
`button`, `input_text`, `checkbox`, `slider`, and `include` elements.

Every element uses a four-number `rect` in `[x, y, width, height]` order. Each
number from `0` through `1` is interpreted as a fraction of the parent region;
other values are pixels. A `panel` renders children inside its region. An
`include` has a unique `name`, a JSON `file`, and a region; the included file's
elements are laid out inside that region. Include paths and direct image `path`
values are relative to the file that contains them. Includes can nest, and
include cycles cause a load error that names the offending file.

Widget fields:

- `text`: requires `id`, `rect`, and `text`.
- `image`: requires `id`, `rect`, and exactly one of `asset` or `path`. Asset
  names are resolved through the game's `AssetManager` and must have type
  `Texture`.
- `button`: requires `id` and `rect`; its optional `text` is the label.
- `input_text`: requires `id` and `rect`; supports `hint`, `max_length`, and an
  initial string `value`.
- `checkbox`: requires `id` and `rect`; supports a label `text` and boolean
  `checked` value.
- `slider`: requires `id`, `rect`, `min`, and `max`; supports label `text` and
  an initial `value` in range.

See [`config/ui/example.json`](../config/ui/example.json) and its included
[`status.json`](../config/ui/status.json) for a complete example.

### C++ and Lua

Load a document into the manager and display it with `show`; `hide` and
`isVisible` control visibility. Widget keys use `document/id`, or
`document/include_name/id` for controls inside an included document. Panel
children remain in their containing document's ID scope. Use `getText` and
`setText` for text and input widgets, and `getValue`/`setValue` for checkbox and
slider values. `onClick` handlers can be registered using a fully-qualified
widget key or just the widget ID. Clicks also accumulate as fully-qualified
keys and can be retrieved with `takeClickedEvents`.
The renderer defers click handlers until it has finished walking the visible
documents, so handlers may safely load documents or replace handlers. Events
are cleared at the start of each render and can be consumed with
`takeClickedEvents` (or `ui.take_clicked_events` in Lua).

`RType::Scripting::bindUi(lua, manager)` exposes the same features as
`ui.load`, `ui.show`, `ui.hide`, `ui.is_visible`, `ui.get_text`,
`ui.set_text`, `ui.get_value`, `ui.set_value`, `ui.on_click`, and
`ui.take_clicked_events`. UI render and click-handler errors are logged to
standard error rather than escaping the draw loop. The game
application exposes its manager through `getUiManager()` so an integrating
script engine can bind it to its Lua state.
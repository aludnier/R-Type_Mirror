# Split PlantUML architecture

- `00_overview.puml` — high-level architecture and cross-view links.
- `01_engine_core_modules.puml` — static Engine Core, dynamic ECS modules and lifecycle.
- `02_ecs_components.puml` — ECS registry/storage and shared/game-defined components.
- `03_network_protocol.puml` — message types, packet registry, packets, UDP transport and reliability.
- `04_server.puml` — server runtime, sessions, game state and server systems.
- `05_client_game.puml` — client runtime, client systems and game-facing libraries.
- `06_network_sequences.puml` — connection, in-game and disconnection message flows (byte format in `docs/protocol.md`).



## Diagram names and CMake targets

| Name in the diagrams | CMake target | Folder |
|---|---|---|
| Engine Core | `engine_core` | `game_engine/` |
| `shared_lib` | `engine_core` | `game_engine/` |
| `client_lib` | `engine_client` | `game_engine/` |
| `server_lib` | `engine_server` | `game_engine/` |
| `game_server` | `GameServer` in `engine_server` | `game_engine/src/server/` |

The server links `engine_server` and `engine_core` only, never `engine_client`.

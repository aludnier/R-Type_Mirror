# Architecture diagrams

These diagrams are drawn from the PlantUML files of `docs/architecture/` when
the documentation site is built. On GitHub, open the `.puml` files to read
their source. Each diagram is described in text before it is drawn.

## Overview

The game uses the static Engine Core. The core discovers, loads and unloads
the dynamic modules (physics, rendering, animation, audio), and each module
adds its components and systems to the ECS. The core also contains the ECS,
the network protocol and transport, the server runtime and the client runtime.
Source: [`00_overview.puml`](00_overview.puml).

```plantuml source="00_overview.puml"
```

## Engine core and modules

The static Engine Core, the dynamic ECS modules and their lifecycle: how a
module library is found, checked, registered and unloaded.
Source: [`01_engine_core_modules.puml`](01_engine_core_modules.puml).

```plantuml source="01_engine_core_modules.puml"
```

## ECS and components

The registry, the component storages and the components shared by the engine
and defined by the game.
Source: [`02_ecs_components.puml`](02_ecs_components.puml).

```plantuml source="02_ecs_components.puml"
```

## Network protocol

The message types, the packets, the UDP transport and the reliability layer.
The byte format is in [the network protocol](../protocol.md).
Source: [`03_network_protocol.puml`](03_network_protocol.puml).

```plantuml source="03_network_protocol.puml"
```

## Server

The server runtime: the game loop, the client sessions, the game state and the
server systems.
Source: [`04_server.puml`](04_server.puml).

```plantuml source="04_server.puml"
```

## Client and game

The client runtime, the client systems and the libraries a game uses.
Source: [`05_client_game.puml`](05_client_game.puml).

```plantuml source="05_client_game.puml"
```

## Network sequences

The messages exchanged when a client connects, while it plays and when it
disconnects.
Source: [`06_network_sequences.puml`](06_network_sequences.puml).

```plantuml source="06_network_sequences.puml"
```

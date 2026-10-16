# R-Type Network Protocol

| | |
|---|---|
| Status | Draft, version 1 |
| Issue | #11 Package Definition |
| Implementation | `game_engine/src/network/` |
| Sequence diagrams | `docs/architecture/06_network_sequences.puml` |

## Abstract

This document specifies the binary protocol used between the R-Type server
and its clients. It runs over UDP. The server is authoritative: it owns the
game state, and clients only send the state of their controls.

The protocol belongs to the game engine. The engine defines a fixed set of
messages (connection, entity replication, heartbeat), and a game can add its
own messages in a reserved range.

## 1. Conventions

The key words "MUST", "MUST NOT", "SHOULD", "SHOULD NOT" and "MAY" are to be
interpreted as described in RFC 2119.

- **Peer**: the server or a client.
- **Datagram**: one UDP datagram. A datagram carries exactly one packet.
- **Packet**: a header (Section 4) followed by a payload (Section 6).
- **Tick**: one step of the server simulation.

## 2. Transport

- The protocol uses UDP only. The server listens on a single port chosen at
  launch. Each client uses a single socket.
- A datagram MUST NOT exceed 1200 bytes, header included, so that it is not
  fragmented on usual internet paths.
- The server identifies a client by its endpoint (IP address and port).
  Packets from an endpoint without a session are ignored, except
  ASK_CONNECT.
- UDP does not guarantee delivery or ordering. Section 7 explains how the
  protocol handles lost, duplicated and reordered datagrams.

## 3. Data types

All multi-byte values are little-endian.

| Type | Size (bytes) | Encoding |
|---|---|---|
| `u8`, `u16`, `u32` | 1, 2, 4 | Unsigned integer |
| `f32` | 4 | IEEE 754 binary32 |
| `string` | 1 + N | `u8` length N, then N bytes of UTF-8, no terminator (N ≤ 255) |
| `bytes` | 2 + N | `u16` length N, then N raw bytes |

Identifiers:

| Name | Type | Meaning |
|---|---|---|
| `ClientId` | `u16` | Assigned by the server when a client connects. 0 means "the server". |
| `NetworkId` | `u32` | Identifies a replicated entity on every peer. Assigned by the server, starts at 1, never reused during a session. It is not the local ECS entity index. 0 means "no entity". |
| `Tick` | `u32` | Server simulation tick counter. |
| `Sequence` | `u16` | Datagram sequence number (Section 7). Wraps around. |
| `ComponentTypeId` | `u16` | Stable identifier of a component type, given when the component is registered. `0x0000`–`0x7FFF`: engine components. `0x8000`–`0xFFFF`: game components. |

## 4. Packet header

Every datagram starts with a 15-byte header.

| Offset | Field | Type | Description |
|---|---|---|---|
| 0 | `magic` | `u16` | `0x5452` (bytes `52 54`, "RT") |
| 2 | `version` | `u8` | Protocol version: 1 for this document |
| 3 | `type` | `u8` | Message type (Section 5) |
| 4 | `flags` | `u8` | Bit 0: `RELIABLE` (Section 7.2). Bits 1–7 are reserved: they MUST be sent as 0 and MUST be ignored on reception |
| 5 | `sequence` | `u16` | Sequence number of this datagram |
| 7 | `ack` | `u16` | Most recent sequence received from the peer |
| 9 | `ack_bits` | `u32` | Bit i set: sequence `ack - 1 - i` was also received |
| 13 | `payload_size` | `u16` | Size of the payload, in bytes |

A receiver MUST drop a datagram when:

- it is shorter than 15 bytes or longer than 1200 bytes;
- `magic` is not `0x5452`;
- `payload_size` is not exactly the datagram size minus 15;
- the message type is unknown;
- the payload cannot be decoded exactly: missing bytes, trailing bytes, or a
  value outside its allowed range.

Dropping a datagram does not close the connection, and MUST NOT crash the
peer.

**Versioning.** `magic`, `version` and `type` keep the same offsets in every
future version. A server that receives an ASK_CONNECT with another version
answers FORCE_DISCONNECT with reason `BAD_VERSION`. Any other packet with
another version is dropped.

## 5. Message types

| Range | Use |
|---|---|
| `0x00` | Invalid |
| `0x01`–`0x3F` | Engine messages, client → server |
| `0x40`–`0x7F` | Engine messages, server → client |
| `0x80`–`0xFF` | Game-defined messages, either direction |

| Id | Name | Direction | Reliable |
|---|---|---|---|
| `0x01` | ASK_CONNECT | client → server | No, retried by the client (Section 8.1) |
| `0x02` | ASK_DISCONNECT | client → server | No |
| `0x03` | CONTROLLABLE | client → server | No |
| `0x04` | HEARTBEAT | client → server | No |
| `0x40` | ACCEPT_CONNECT | server → client | Yes |
| `0x41` | FORCE_DISCONNECT | server → client | No, the client times out if it is lost |
| `0x42` | PLAYER_CONNECTED | server → client | Yes |
| `0x43` | PLAYER_DISCONNECTED | server → client | Yes |
| `0x44` | ENTITY_SPAWN | server → client | Yes |
| `0x45` | ENTITY_DESPAWN | server → client | Yes |
| `0x46` | COMPONENT_UPDATE | server → client | No |
| `0x47` | HEARTBEAT_REPLY | server → client | No |

## 6. Messages

Fields are listed in the order they are written in the payload.

### 6.1 Client → server

**ASK_CONNECT (`0x01`)**: asks to join the game.

| Field | Type | Description |
|---|---|---|
| `player_name` | `string` | Display name, may be empty |

**ASK_DISCONNECT (`0x02`)**: the client leaves the game. Empty payload.

**CONTROLLABLE (`0x03`)**: current state of the player's controls.

| Field | Type | Description |
|---|---|---|
| `input_sequence` | `u32` | Incremented by the client for every CONTROLLABLE it sends |
| `actions` | `u32` | Bit `i` set: game-defined action `i` is active |

The client sends the state of its controls, never raw key events. The server
keeps the CONTROLLABLE with the highest `input_sequence` and ignores older
ones.

The engine gives no meaning to the bits of `actions`: moving is an action like
any other, so a game can have no vertical move, or more than four directions.
Each game lists its actions, R-Type in Appendix B.

**HEARTBEAT (`0x04`)**

| Field | Type | Description |
|---|---|---|
| `client_time` | `u32` | Client clock in milliseconds, sent back by the server |

### 6.2 Server → client

**ACCEPT_CONNECT (`0x40`)**: the connection is accepted.

| Field | Type | Description |
|---|---|---|
| `client_id` | `ClientId` | Identifier given to this client |
| `linked_entity` | `NetworkId` | Entity controlled by this client |
| `tick_rate` | `u16` | Server ticks per second |

**FORCE_DISCONNECT (`0x41`)**: the server refuses or closes the connection.

| Field | Type | Description |
|---|---|---|
| `reason` | `u8` | `DisconnectReason` (below) |
| `message` | `string` | Human-readable text, may be empty |

**PLAYER_CONNECTED (`0x42`)**: another player is in the game.

| Field | Type | Description |
|---|---|---|
| `client_id` | `ClientId` | Identifier of that player |
| `linked_entity` | `NetworkId` | Entity controlled by that player |
| `player_name` | `string` | Name sent in its ASK_CONNECT |

**PLAYER_DISCONNECTED (`0x43`)**: another player left the game.

| Field | Type | Description |
|---|---|---|
| `client_id` | `ClientId` | Identifier of that player |
| `reason` | `u8` | `DisconnectReason` (below) |

**ENTITY_SPAWN (`0x44`)**: a replicated entity appears.

| Field | Type | Description |
|---|---|---|
| `network_id` | `NetworkId` | Entity identifier |
| `component_count` | `u8` | Number of components that follow |
| `components` | `component` × `component_count` | Initial value of each component |

A `component` is encoded as:

| Field | Type | Description |
|---|---|---|
| `type_id` | `ComponentTypeId` | Component type |
| `data` | `bytes` | Serialized component. The layout is defined by the component itself |

**ENTITY_DESPAWN (`0x45`)**: a replicated entity disappears.

| Field | Type | Description |
|---|---|---|
| `network_id` | `NetworkId` | Entity identifier |

**COMPONENT_UPDATE (`0x46`)**: new values of some components of several
entities.

| Field | Type | Description |
|---|---|---|
| `tick` | `Tick` | Tick at which the values were computed |
| `entity_count` | `u8` | Number of entities that follow |
| `entities` | `entity` × `entity_count` | Updated entities |

An `entity` is encoded as:

| Field | Type | Description |
|---|---|---|
| `network_id` | `NetworkId` | Entity identifier |
| `component_count` | `u8` | Number of components that follow |
| `components` | `component` × `component_count` | Same encoding as in ENTITY_SPAWN |

Grouping entities saves the header of one datagram per entity: an entity
that only updates its position costs 17 bytes, so about 69 of them fit in one
datagram. When the update does not fit, the server sends several
COMPONENT_UPDATE with the same `tick`.

**HEARTBEAT_REPLY (`0x47`)**

| Field | Type | Description |
|---|---|---|
| `client_time` | `u32` | Copied from the HEARTBEAT |
| `server_tick` | `Tick` | Current server tick |

**DisconnectReason** values. Any other value makes the payload invalid.

| Value | Name | Meaning |
|---|---|---|
| 0 | `LEFT` | The client sent ASK_DISCONNECT |
| 1 | `TIMEOUT` | Nothing received for too long: the client crashed or lost its connection (Section 8.3) |
| 2 | `KICKED` | Removed by the server |
| 3 | `SERVER_FULL` | No free slot |
| 4 | `BAD_VERSION` | Protocol versions differ |
| 5 | `SERVER_SHUTDOWN` | The server is stopping |

### 6.3 Engine components

ENTITY_SPAWN and COMPONENT_UPDATE carry components as a `type_id` and its
`data`. The engine defines the components below. `controllable` and
`replicated` are never sent as components: the controls travel in
CONTROLLABLE, and every entity sent over the network is replicated.

| Id | Component | `data` layout | Size (bytes) |
|---|---|---|---|
| `0x0001` | POSITION | `x: f32`, `y: f32` | 8 |
| `0x0002` | PHYSICS | `velocity_x: f32`, `velocity_y: f32`, `hitbox_w: f32`, `hitbox_h: f32` | 16 |
| `0x0003` | ANIMATION | `sprite_id: string`, `frame_count: u8`, `frame_duration_ms: u16`, `current_frame: u8` | 5 + N |

Coordinates and units are chosen by the game and documented in Appendix B.

A receiver ignores a component whose `type_id` it does not know: the length
prefix of `data` lets it skip the component. A component whose `data` does
not match its layout is ignored as well.

### 6.4 Game-defined messages and components

Message ids `0x80`–`0xFF` and component ids `0x8000`–`0xFFFF` are free for
the game. The game chooses their layout and registers them in the engine.
They follow every other rule of this document: header, size limit and
validation. A reliable game message MUST be idempotent (Section 7.2).

A game MUST document each of its messages and components in Appendix B, so
that a client can be written from this document alone.

## 7. Sequencing and reliability

### 7.1 Acknowledgements

- Each peer keeps one `u16` counter per connection, incremented for every
  datagram it sends, reliable or not.
- In every datagram, `ack` and `ack_bits` describe the 33 most recent
  sequences received from the peer. Acknowledgements travel inside regular
  traffic; HEARTBEAT and the per-tick messages make sure there always is
  some.
- Sequence numbers wrap around. `a` is more recent than `b` when
  `a > b and a - b <= 32768`, or `a < b and b - a > 32768`.

### 7.2 Reliable messages

- The sender keeps every datagram that has the `RELIABLE` flag until its
  sequence is acknowledged, or until the connection is closed.
- If it is not acknowledged after the resend timeout, the sender sends the
  same payload again, in a new datagram with a new sequence number.
- A reliable message can therefore arrive more than once, and out of order.
  Every reliable message is idempotent, so receivers handle this without
  message identifiers:
  - ENTITY_SPAWN for a `NetworkId` that is already known, or that was already
    despawned, is ignored.
  - ENTITY_DESPAWN for an unknown `NetworkId` is ignored, and the id is
    remembered as despawned for at least the connection timeout (5 s). After
    that it MAY be forgotten, which keeps the memory used bounded: a spawn
    still resent that late would mean the connection is lost anyway.
  - ACCEPT_CONNECT, PLAYER_CONNECTED and PLAYER_DISCONNECTED have no further
    effect when applied twice.

### 7.3 Unreliable messages

Unreliable messages are never resent: the next one carries fresher data.

- COMPONENT_UPDATE: for each (`network_id`, `type_id`) pair, the receiver
  keeps the tick of the last applied value and ignores updates with an older
  or equal tick, and forgets these ticks when the entity is despawned.
  Updates for an unknown `NetworkId` are ignored.
- CONTROLLABLE: only the highest `input_sequence` counts.

## 8. Session lifecycle

### 8.1 Connection

1. The client sends ASK_CONNECT every 500 ms until it receives
   ACCEPT_CONNECT or FORCE_DISCONNECT, at most 10 times.
2. The server answers:
   - FORCE_DISCONNECT (`BAD_VERSION`) if the header version differs from its
     own;
   - FORCE_DISCONNECT (`SERVER_FULL`) if no slot is free;
   - otherwise, it creates a session and spawns the player's entity. It then
     sends to the new client ACCEPT_CONNECT, one ENTITY_SPAWN per replicated
     entity (its own included) and one PLAYER_CONNECTED per player already in
     the game. It also sends PLAYER_CONNECTED for the new player to every
     other client.
3. An ASK_CONNECT from an endpoint that already has a session is answered
   with the same ACCEPT_CONNECT. No second session is created.

Because of reordering, the client MUST accept server messages that arrive
before ACCEPT_CONNECT.

### 8.2 In game

- The server runs at `tick_rate` (60 recommended). Every tick, it processes
  received messages, runs its systems, then sends COMPONENT_UPDATE for the
  replicated components that changed, and ENTITY_SPAWN / ENTITY_DESPAWN when
  needed. To save bandwidth, the server MAY send COMPONENT_UPDATE only every
  few ticks: clients then interpolate between two updates using `tick`.
- The client SHOULD send CONTROLLABLE once per server tick period
  (`1 / tick_rate`), so that a lost message is quickly replaced.
- The client sends HEARTBEAT every second. The server answers
  HEARTBEAT_REPLY right away. The round-trip time is the current client time
  minus `client_time`.

### 8.3 Disconnection

- **The client leaves**: it sends ASK_DISCONNECT (up to 3 times, since the
  message is unreliable), then closes. The server closes the session,
  despawns the player's entity and sends PLAYER_DISCONNECTED (`LEFT`) to the
  other clients.
- **Timeout**: a peer that has received nothing from the other side for 5
  seconds considers the connection lost. This is how a crashed client is
  detected: the server acts as above with reason `TIMEOUT`, so the other
  players are notified, and the game goes on. The client goes back to its
  menu.
- **Kick or shutdown**: the server sends FORCE_DISCONNECT (`KICKED` or
  `SERVER_SHUTDOWN`), then closes the session.

## 9. Security considerations

- Every value read from the network is untrusted. Every read is
  bounds-checked (sizes, counts, lengths) and every enumeration is validated
  (Section 4).
- Memory is bounded: decoding a datagram allocates at most a few tens of
  kilobytes, whatever its content, and per-session state (reliable messages
  waiting for an ack, despawned ids) is freed at the latest by the
  connection timeout. The number of sessions is bounded by the number of
  player slots.
- The server is authoritative: clients only send their controls, never
  positions or other state, so a modified client cannot move its entity
  freely.
- An ASK_CONNECT makes the server send several packets to the source
  endpoint, which can be spoofed. The server SHOULD limit the rate of
  ASK_CONNECT per endpoint.
- Version 1 has no encryption and no authentication.

## 10. Constants

| Name | Value |
|---|---|
| Magic | `0x5452` |
| Protocol version | 1 |
| Header size | 15 bytes |
| Maximum datagram size | 1200 bytes |
| Maximum payload size | 1185 bytes |
| Recommended tick rate | 60 Hz |
| Heartbeat interval | 1 s |
| Connection timeout | 5 s |
| Resend timeout | 100 ms |
| ASK_CONNECT retry | every 500 ms, 10 attempts |

## Appendix A. Example

A HEARTBEAT with `client_time = 1000`, sequence 7, acknowledging sequences 5,
4 and 3:

```
52 54  01  04  00  07 00  05 00  03 00 00 00  04 00  e8 03 00 00
magic  ver type flg seq   ack    ack_bits     size   client_time
```

## Appendix B. R-Type game content

This appendix lists what the R-Type game adds to the engine protocol. It is
completed as the game components and messages are implemented (issue #7).

For each game component: id (`0x8000`–`0xFFFF`), name and `data` layout.
For each game message: id (`0x80`–`0xFF`), direction, reliability and payload
layout. It also gives the bits of the CONTROLLABLE `actions` (for example
bit 0: up, bit 4: shoot), the coordinate system and units used by POSITION,
and how a `ClientId` maps to a player color.

| Id | Name | Layout |
|---|---|---|
| | *none yet* | |

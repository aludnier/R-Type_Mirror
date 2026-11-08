# Physics dynamic module

It contains Components and Systems for physics simulation, including collision detection, rigid body dynamics, and more.

## Layout

- `src/components/`: one header per component (`Rigidbody2D`, `BoxCollider2D`, `CircleCollider2D`, `CollisionEvent2D`,
  `CollisionEvents2D`, `PhysicsSettings2D`).
- `src/systems/`: one header/cpp pair per system (`MovementSystem`, `CollisionDetectionSystem`,
  `CollisionResponseSystem`) plus helpers (`OverlapTests`, `CollisionEventEmitter`, `Shape.h`, `Contact.h`).

Systems run in this order: movement, detection (contacts), response (separation, bounce, Enter/Exit events).

Requires `Transform2D` to be registered by the game before this module. Add a `PhysicsSettings2D` on one entity (set
`dt` each frame); add `CollisionEvents2D` to receive events.

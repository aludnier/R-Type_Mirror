# Lifetime and timer module

The Lifetime module registers systems for timed entity destruction and
one-shot timer events.

## Components

- `Lifetime` stores an entity's lifetime duration and elapsed time.
- `Timer` stores a duration, elapsed time, event name, and whether it has fired.
- `TimerEvents` stores timer events emitted during the current system update.
- `TimerSettings` provides the `dt` in seconds used by both systems. If no
  valid settings component exists, the systems use `1 / 60` second.

Add a `TimerSettings` component to any entity to set the frame delta. Attach a
`Lifetime` to entities that should be automatically destroyed. Attach both a
`Timer` and `TimerEvents` to entities that should emit an event; read the
event's `name` from `TimerEvents::events` after `Registry::runSystems()`.
Events are cleared at the start of each timer-system update, and each `Timer`
fires once.

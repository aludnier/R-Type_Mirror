# Animation dynamic module

The Animation module provides named 2D animation clips and a system that updates
the `Sprite` texture rectangle from each entity's current animation.

## Components

- `Animator` stores named clips, the selected clip, frame index, elapsed frame
  time, playback speed, and loop mode.
- `AnimationClip` can be built from explicit frame rectangles or a sprite-sheet
  region, frame size, frame count, and frame duration.
- `AnimationSettings` provides the per-frame `dt` used by the registered system.

Register `Sprite` before registering the module. Add an `AnimationSettings`
component to an entity and update its `dt` before `Registry::runSystems()`.
Add clips with `AnimationSystem::addClip`, select one with
`AnimationSystem::play(animator, "clip-name")`, and attach both that animator
and a `Sprite` to the entity. `play` returns `false` when the requested name
is not present. Use `AnimationSystem::createClip` for explicit rectangles or
`AnimationSystem::createSpriteSheetClip` for a sheet region.

-- Minimal game written only with the scripting API: a game-wide system spawns
-- enemies on the right in waves, and each enemy runs bydo.lua, its entity
-- script, to move to the left and die when it leaves the screen.
-- The host registers Transform2D, Health and Hitbox, adds the script system,
-- then calls ScriptEngine::setDeltaTime(dt) and Registry::runSystems() every
-- frame.

local SCREEN_WIDTH = 1280
local SCREEN_HEIGHT = 720
local WAVE_DELAY = 2.0
local WAVE_SIZE = 4

local spawner = world.spawn()
world.data(spawner).timer = 0
waves_spawned = 0

local function spawn_enemy(y)
    local enemy = world.spawn()
    world.add(enemy, "Transform2D", Transform2D.new(Vector2.new(SCREEN_WIDTH, y)))
    world.add(enemy, "Health", Health.new(2))
    world.add(enemy, "Hitbox", Hitbox.new(32, 32))
    world.add(enemy, "Script", {"bydo.lua"})
    return enemy
end

world.add_system("spawn_waves", function(dt)
    local state = world.data(spawner)
    state.timer = state.timer - dt
    if state.timer > 0 then
        return
    end
    state.timer = WAVE_DELAY
    waves_spawned = waves_spawned + 1
    for i = 1, WAVE_SIZE do
        spawn_enemy(i * SCREEN_HEIGHT / (WAVE_SIZE + 1))
    end
end)

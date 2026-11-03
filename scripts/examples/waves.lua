-- Minimal game written only with the scripting API: enemies spawn on the
-- right in waves, move to the left and die when they leave the screen.
-- The host registers Transform2D, Health and Hitbox, then calls
-- ScriptEngine::setDeltaTime(dt) and Registry::runSystems() every frame.

local SCREEN_WIDTH = 1280
local SCREEN_HEIGHT = 720
local ENEMY_SPEED = 200
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
    world.data(enemy).kind = "bydo"
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

world.add_system("move_enemies", function(dt)
    world.each({"Transform2D", "Health"}, function(e, transform, health)
        transform.position.x = transform.position.x - ENEMY_SPEED * dt
        if transform.position.x < 0 or health:is_dead() then
            world.kill(e)
        end
    end)
end)

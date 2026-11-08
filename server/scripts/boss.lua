-- Entity script of the boss: comes in from the right, then moves up and
-- down and shoots a fan of FAN_BULLETS bullets every SHOOT_INTERVAL
-- seconds. Its health grows with the level (rtype.level, set by
-- waves.lua).

local STOP_X = 1550
local ENTRY_SPEED = 150
local SWAY_AMPLITUDE = 300
local SWAY_FREQUENCY = 0.6
local SHOOT_INTERVAL = 1.6
local FAN_BULLETS = 5
local FAN_ANGLE = math.rad(60)
local BULLET_SPEED = 380
local HEALTH_PER_LEVEL = 60

local boss = {}

function boss.on_update(e, dt)
    local enemy = world.get(e, "Enemy")
    local position = world.get(e, "Transform2D").position
    local scale = rtype.speed_scale or 1

    local health = world.get(e, "Health")
    local max = HEALTH_PER_LEVEL * (rtype.level or 1)
    if health.max ~= max then
        health.current = max
        health.max = max
    end
    if position.x > STOP_X then
        position.x = position.x - ENTRY_SPEED * dt
        return
    end
    -- The sway starts once it stopped, from the middle of the screen
    -- where it came in: no jump.
    enemy.elapsed = enemy.elapsed + dt
    position.y = rtype.SCREEN_HEIGHT / 2
        + math.sin(enemy.elapsed * SWAY_FREQUENCY * scale) * SWAY_AMPLITUDE
    enemy.shoot_timer = enemy.shoot_timer + dt * scale
    if enemy.shoot_timer < SHOOT_INTERVAL then
        return
    end
    enemy.shoot_timer = 0
    for i = 0, FAN_BULLETS - 1 do
        local angle = -FAN_ANGLE / 2 + FAN_ANGLE * i / (FAN_BULLETS - 1)
        rtype.spawn("EnemyBullet", position.x - 90, position.y,
            -math.cos(angle) * BULLET_SPEED, math.sin(angle) * BULLET_SPEED)
    end
end

return boss

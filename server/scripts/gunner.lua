-- Entity script of every gunner: flies to the left, slower than a scout,
-- drifting up and down, and shoots an EnemyBullet every SHOOT_INTERVAL
-- seconds once it is on screen.

local SPEED = 100
local DRIFT_AMPLITUDE = 120
local DRIFT_FREQUENCY = 0.8
local SHOOT_INTERVAL = 1.5

local gunner = {}

function gunner.on_update(e, dt)
    local enemy = world.get(e, "Enemy")
    local position = world.get(e, "Transform2D").position
    local speed = SPEED * (rtype.speed_scale or 1)

    enemy.elapsed = enemy.elapsed + dt
    position.x = position.x - speed * dt
    position.y = enemy.base_y
        + math.sin(enemy.elapsed * DRIFT_FREQUENCY) * DRIFT_AMPLITUDE
    enemy.shoot_timer = enemy.shoot_timer + dt
    if enemy.shoot_timer >= SHOOT_INTERVAL
        and position.x < rtype.SCREEN_WIDTH then
        enemy.shoot_timer = 0
        rtype.spawn("EnemyBullet", position.x - 25, position.y)
    end
end

return gunner

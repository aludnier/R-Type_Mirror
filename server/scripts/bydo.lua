-- Entity script of every Bydo: flies to the left on a sine wave and shoots
-- an EnemyBullet every SHOOT_INTERVAL seconds once it is on screen. The
-- server removes it when it is shot down or leaves the screen.

local SPEED = 150
local WAVE_AMPLITUDE = 60
local WAVE_FREQUENCY = 2
local SHOOT_INTERVAL = 2

local bydo = {}

function bydo.on_update(e, dt)
    local enemy = world.get(e, "Enemy")
    local position = world.get(e, "Transform2D").position

    enemy.elapsed = enemy.elapsed + dt
    position.x = position.x - SPEED * dt
    position.y = enemy.base_y
        + math.sin(enemy.elapsed * WAVE_FREQUENCY) * WAVE_AMPLITUDE
    enemy.shoot_timer = enemy.shoot_timer + dt
    if enemy.shoot_timer >= SHOOT_INTERVAL
        and position.x < rtype.SCREEN_WIDTH then
        enemy.shoot_timer = 0
        rtype.spawn("EnemyBullet", position.x - 20, position.y)
    end
end

return bydo

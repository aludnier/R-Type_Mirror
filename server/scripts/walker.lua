-- Entity script of every walker: walks to the left on the ground, a bit
-- faster than the ground scrolls, and shoots up and to the left every
-- SHOOT_INTERVAL seconds once on screen.

local WALK_SPEED = 40
local SHOOT_INTERVAL = 2.2
local BULLET_VELOCITY_X = -220
local BULLET_VELOCITY_Y = -320
-- Half the height of its hitbox (42): its feet are on the ground.
local HALF_HEIGHT = 21

local walker = {}

function walker.on_update(e, dt)
    local enemy = world.get(e, "Enemy")
    local position = world.get(e, "Transform2D").position
    local scale = rtype.speed_scale or 1

    enemy.elapsed = enemy.elapsed + dt
    position.x = position.x
        - (rtype.GROUND_SCROLL_SPEED + WALK_SPEED * scale) * dt
    position.y = rtype.GROUND_TOP - HALF_HEIGHT
    enemy.shoot_timer = enemy.shoot_timer + dt * scale
    if enemy.shoot_timer >= SHOOT_INTERVAL
        and position.x < rtype.SCREEN_WIDTH then
        enemy.shoot_timer = 0
        rtype.spawn("EnemyBullet", position.x - 20, position.y - 25,
            BULLET_VELOCITY_X, BULLET_VELOCITY_Y)
    end
end

return walker

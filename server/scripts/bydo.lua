-- Entity script of every Bydo scout: flies to the left on a sine wave. It
-- does not shoot: the gunners do (gunner.lua). The server removes it when
-- it is shot down or leaves the screen.

local SPEED = 150
local WAVE_AMPLITUDE = 60
local WAVE_FREQUENCY = 2

local bydo = {}

function bydo.on_update(e, dt)
    local enemy = world.get(e, "Enemy")
    local position = world.get(e, "Transform2D").position

    enemy.elapsed = enemy.elapsed + dt
    position.x = position.x - SPEED * (rtype.speed_scale or 1) * dt
    position.y = enemy.base_y
        + math.sin(enemy.elapsed * WAVE_FREQUENCY) * WAVE_AMPLITUDE
end

return bydo

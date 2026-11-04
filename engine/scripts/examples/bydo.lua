-- Entity script attached to each enemy by waves.lua: moves it to the left
-- and kills it when it leaves the screen or has no health left.

local SPEED = 200

local bydo = {}

function bydo.on_start(e)
    world.data(e).kind = "bydo"
end

function bydo.on_update(e, dt)
    local transform = world.get(e, "Transform2D")
    transform.position.x = transform.position.x - SPEED * dt
    if transform.position.x < 0 or world.get(e, "Health"):is_dead() then
        world.kill(e)
    end
end

return bydo

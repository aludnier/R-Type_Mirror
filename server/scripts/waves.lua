-- Game script of the R-Type server: spawns an enemy past the right edge of
-- the screen every SPAWN_INTERVAL seconds, at a random height: a Bydo scout
-- (bydo.lua), and a gunner (gunner.lua) every GUNNER_EVERY enemies.

local SPAWN_INTERVAL = 1.5
-- Keeps the sine wave of bydo.lua (amplitude 60) inside the screen.
local MARGIN = 100

local GUNNER_EVERY = 4
-- Keeps the drift of gunner.lua (amplitude 120) inside the screen.
local GUNNER_MARGIN = 150

local timer = 0
local count = 0

world.add_system("spawn_enemies", function(dt)
    timer = timer + dt
    if timer < SPAWN_INTERVAL then
        return
    end
    timer = 0
    count = count + 1
    if count % GUNNER_EVERY == 0 then
        local y = GUNNER_MARGIN
            + math.random() * (rtype.SCREEN_HEIGHT - 2 * GUNNER_MARGIN)
        rtype.spawn("Gunner", rtype.SCREEN_WIDTH + 40, y)
        return
    end
    local y = MARGIN + math.random() * (rtype.SCREEN_HEIGHT - 2 * MARGIN)
    rtype.spawn("Enemy", rtype.SCREEN_WIDTH + 40, y)
end)

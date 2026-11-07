-- Game script of the R-Type server: spawns a Bydo past the right edge of
-- the screen every SPAWN_INTERVAL seconds, at a random height. The server
-- attaches bydo.lua to every enemy it spawns.

local SPAWN_INTERVAL = 1.5
-- Keeps the sine wave of bydo.lua (amplitude 60) inside the screen.
local MARGIN = 100

local timer = 0

world.add_system("spawn_enemies", function(dt)
    timer = timer + dt
    if timer < SPAWN_INTERVAL then
        return
    end
    timer = 0
    local y = MARGIN + math.random() * (rtype.SCREEN_HEIGHT - 2 * MARGIN)
    rtype.spawn("Enemy", rtype.SCREEN_WIDTH + 40, y)
end)

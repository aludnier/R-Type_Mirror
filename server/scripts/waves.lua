-- Game script of the R-Type server: the levels. Each level sends enemies
-- for WAVES_TIME seconds, past the right edge of the screen at a random
-- height (a Bydo scout, and a gunner every GUNNER_EVERY enemies), then the
-- boss. When the boss is shot down, the next level starts, with more and
-- faster enemies. rtype.level and rtype.speed_scale are read by the enemy
-- scripts.

local SPAWN_INTERVAL = 1.5
local MIN_SPAWN_INTERVAL = 0.6
local WAVES_TIME = 45
-- Pause between the last wave and the boss.
local BOSS_DELAY = 4
-- Keeps the sine wave of bydo.lua (amplitude 60) inside the screen.
local MARGIN = 100
local GUNNER_EVERY = 4
-- Keeps the drift of gunner.lua (amplitude 120) inside the screen.
local GUNNER_MARGIN = 150

rtype.level = 1
rtype.speed_scale = 1

local state = "waves"
local timer = 0
local level_time = 0
local count = 0

local function spawn_interval()
    return math.max(MIN_SPAWN_INTERVAL,
        SPAWN_INTERVAL - 0.3 * (rtype.level - 1))
end

local function boss_alive()
    local alive = false
    world.each({"Boss"}, function() alive = true end)
    return alive
end

local function spawn_enemy()
    count = count + 1
    if count % GUNNER_EVERY == 0 then
        local y = GUNNER_MARGIN
            + math.random() * (rtype.SCREEN_HEIGHT - 2 * GUNNER_MARGIN)
        rtype.spawn("Gunner", rtype.SCREEN_WIDTH + 40, y)
        return
    end
    local y = MARGIN + math.random() * (rtype.SCREEN_HEIGHT - 2 * MARGIN)
    rtype.spawn("Enemy", rtype.SCREEN_WIDTH + 40, y)
end

world.add_system("levels", function(dt)
    timer = timer + dt
    if state == "waves" then
        level_time = level_time + dt
        if level_time >= WAVES_TIME then
            state = "boss_coming"
            timer = 0
        elseif timer >= spawn_interval() then
            timer = 0
            spawn_enemy()
        end
    elseif state == "boss_coming" then
        if timer >= BOSS_DELAY then
            -- Spawned at the end of the tick: it exists from the next one.
            rtype.spawn("Boss", rtype.SCREEN_WIDTH + 80,
                rtype.SCREEN_HEIGHT / 2)
            state = "boss_spawned"
        end
    elseif state == "boss_spawned" then
        if boss_alive() then
            state = "boss"
        end
    elseif state == "boss" and not boss_alive() then
        rtype.level = rtype.level + 1
        rtype.speed_scale = 1 + 0.25 * (rtype.level - 1)
        state = "waves"
        timer = 0
        level_time = 0
    end
end)

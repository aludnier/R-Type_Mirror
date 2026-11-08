-- Game script of the R-Type server: the levels. Each level sends enemies
-- for WAVES_TIME seconds, past the right edge of the screen at a random
-- height (a Bydo scout, and a gunner every GUNNER_EVERY enemies) and
-- walkers on the ground, then the boss. When the boss is shot down, the next level starts, with more and
-- faster enemies. When no player is left, it goes back to level 1 and
-- waits. rtype.level and rtype.speed_scale are read by the enemy scripts.

local SPAWN_INTERVAL = 1.5
local MIN_SPAWN_INTERVAL = 0.6
local WAVES_TIME = 45
-- Pause between the last wave and the boss.
local BOSS_DELAY = 4
-- Keeps the sine wave of bydo.lua (amplitude 60) inside the screen and
-- above the ground.
local MARGIN = 100
local GUNNER_EVERY = 4
-- Keeps the drift of gunner.lua (amplitude 120) inside the screen and
-- above the ground.
local GUNNER_MARGIN = 150
-- A walker comes every WALKER_INTERVAL seconds at level 1, more often
-- after.
local WALKER_INTERVAL = 6
local MIN_WALKER_INTERVAL = 2.5
-- Half the height of a walker's hitbox: its feet are on the ground.
local WALKER_HALF_HEIGHT = 21

rtype.level = 1
rtype.speed_scale = 1

local state = "waves"
local timer = 0
local walker_timer = 0
local level_time = 0
local count = 0

local function spawn_interval()
    return math.max(MIN_SPAWN_INTERVAL,
        SPAWN_INTERVAL - 0.3 * (rtype.level - 1))
end

local function players_alive()
    local alive = false
    world.each({"Controllable"}, function() alive = true end)
    return alive
end

-- Without players, the game waits at level 1: a new player starts from
-- the beginning, without a boss left on screen.
local function reset()
    rtype.level = 1
    rtype.speed_scale = 1
    state = "waves"
    timer = 0
    walker_timer = 0
    level_time = 0
    world.each({"Boss"}, function(e) world.kill(e) end)
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
            + math.random() * (rtype.GROUND_TOP - 2 * GUNNER_MARGIN)
        rtype.spawn("Gunner", rtype.SCREEN_WIDTH + 40, y)
        return
    end
    local y = MARGIN + math.random() * (rtype.GROUND_TOP - 2 * MARGIN)
    rtype.spawn("Enemy", rtype.SCREEN_WIDTH + 40, y)
end

world.add_system("levels", function(dt)
    if not players_alive() then
        if rtype.level ~= 1 or state ~= "waves" or level_time > 0 then
            reset()
        end
        return
    end
    timer = timer + dt
    if state == "waves" then
        level_time = level_time + dt
        walker_timer = walker_timer + dt
        if walker_timer >= math.max(MIN_WALKER_INTERVAL,
            WALKER_INTERVAL - (rtype.level - 1)) then
            walker_timer = 0
            rtype.spawn("Walker", rtype.SCREEN_WIDTH + 40,
                rtype.GROUND_TOP - WALKER_HALF_HEIGHT)
        end
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

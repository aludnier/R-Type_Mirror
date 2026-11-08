-- Game script: player weapon charging and firing logic.
-- Reads Controllable action mask, tracks charge time, and fires on release.
-- The server actually spawns the projectile; this script just tracks charge state.

local SHOOT_ACTION = Action.Shoot
local CHARGE_TIME = 1.5

world.add_system("player_weapon", function(dt)
    world.each({"Player", "Controllable"}, function(e, player, controllable)
        if player.shoot_cooldown > 0 then
            player.shoot_cooldown = player.shoot_cooldown - dt
        end

        local shooting = controllable:is_active(SHOOT_ACTION)
        if shooting then
            player.is_charging = true
            player.charge_time = player.charge_time + dt
        elseif player.is_charging then
            player.is_charging = false
            player.charge_time = 0
        end
    end)
end)

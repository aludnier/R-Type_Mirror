-- Server script: removes out-of-bounds projectiles and enemies.

local ok, _ = pcall(function()
    world.each({"Projectile"}, function() end)
end)
if not ok then
    return
end

local SCREEN_WIDTH = 1920
local SCREEN_HEIGHT = 1080
local MARGIN = 100

world.add_system("server_cleanup", function(dt)
    world.each({"Transform2D", "Projectile"}, function(e, transform, projectile)
        local x = transform.position.x
        local y = transform.position.y
        if x < -MARGIN or x > SCREEN_WIDTH + MARGIN or
           y < -MARGIN or y > SCREEN_HEIGHT + MARGIN then
            world.kill(e)
        end
    end)
    
    world.each({"Transform2D", "PowerUp"}, function(e, transform, power_up)
        if transform.position.x < -MARGIN then
            world.kill(e)
        end
    end)

    world.each({"Transform2D", "Enemy"}, function(e, transform, enemy)
        local x = transform.position.x
        local y = transform.position.y
        if x < -MARGIN or x > SCREEN_WIDTH + MARGIN or
           y < -MARGIN or y > SCREEN_HEIGHT + MARGIN then
            world.kill(e)
        end
    end)
end)

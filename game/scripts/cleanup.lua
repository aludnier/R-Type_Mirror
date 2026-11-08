-- Game script: removes out-of-bounds projectiles and enemies.
-- Replicated entities are despawned by the server, so this only affects
-- local predicted entities.

local SCREEN_WIDTH = 1920
local SCREEN_HEIGHT = 1080
local MARGIN = 100

world.add_system("cleanup_out_of_bounds", function(dt)
    world.each({"Transform2D", "Projectile"}, function(e, transform, projectile)
        local x = transform.position.x
        local y = transform.position.y
        if x < -MARGIN or x > SCREEN_WIDTH + MARGIN or
           y < -MARGIN or y > SCREEN_HEIGHT + MARGIN then
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

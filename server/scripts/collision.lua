-- Server script: processes CollisionEvents2D and resolves hits between
-- projectiles and their targets (enemies or players).

world.add_system("server_collision_resolution", function(dt)
    world.each({"CollisionEvents2D"}, function(e, events)
        for _, event in ipairs(events.events) do
            if event.kind ~= CollisionKind.Enter then
                return
            end
            local other = event.other
            local proj = world.get(e, "Projectile")
            local otherHealth = world.get(other, "Health")
            local otherEnemy = world.get(other, "Enemy")
            local otherControllable = world.get(other, "Controllable")

            if not proj then
                return
            end

            -- Player bullet hits enemy
            if proj.is_player_bullet and otherEnemy and otherHealth then
                otherHealth:take_damage(proj.damage)
                if not proj.is_charged_beam then
                    world.kill(e)
                end
                if otherHealth:is_dead() then
                    world.kill(other)
                end
            -- Enemy bullet hits player (just destroy the bullet)
            elseif not proj.is_player_bullet and otherControllable then
                if not proj.is_charged_beam then
                    world.kill(e)
                end
            end
        end
    end)
end)

-- Game script of the R-Type server: the ground. A player that touches it
-- loses a life, like when an enemy hits it (rtype.hit_player respawns it
-- above the ground).

-- Half the height of a player's hitbox (52.5).
local PLAYER_HALF_HEIGHT = 26

world.add_system("ground", function(dt)
    world.each({"Controllable", "Transform2D"}, function(e, _, transform)
        if transform.position.y + PLAYER_HALF_HEIGHT >= rtype.GROUND_TOP then
            rtype.hit_player(e)
        end
    end)
end)

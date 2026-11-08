local ENEMY_NAME = "active_enemy"

local projectile = {}

function projectile.on_update(e, dt)
    local data = world.data(e)
    local target = data.target
    local target_name = world.get(target, "Name")
    local target_transform = world.get(target, "Transform2D")
    local target_health = world.get(target, "Health")

    if target_name == nil
        or target_name.value ~= ENEMY_NAME
        or target_transform == nil
        or target_health == nil
        or target_health:is_dead() then
        world.kill(e)
        return
    end

    local position = world.get(e, "Transform2D").position
    local target_position = target_transform.position
    local dx = target_position.x - position.x
    local dy = target_position.y - position.y
    local distance = math.sqrt(dx * dx + dy * dy)
    local step = data.speed * dt

    if distance <= step then
        position.x = target_position.x
        position.y = target_position.y
        target_health:take_damage(data.damage)
        if target_health:is_dead() then
            world.kill(target)
        end
        world.kill(e)
    elseif distance > 0 then
        position.x = position.x + dx / distance * step
        position.y = position.y + dy / distance * step
    end
end

return projectile

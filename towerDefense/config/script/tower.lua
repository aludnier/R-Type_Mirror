local ENEMY_NAME = "active_enemy"
local PROJECTILE_TEMPLATE_NAME = "projectile_crossbow"
local PROJECTILE_SCRIPT = "towerDefense/config/script/projectile.lua"
local PROJECTILE_SPEED = 300

local LEVELS = {
    {range = 100, damage = 20, interval = 1.5},
    {range = 140, damage = 25, interval = 1},
    {range = 170, damage = 30, interval = 0.75},
}

local tower = {}

local function findProjectileTemplate()
    local template
    world.each({"Name", "Sprite", "Transform2D"}, function(e, name)
        if name.value == PROJECTILE_TEMPLATE_NAME then
            template = e
        end
    end)

    if template == nil then
        error("Missing projectile template named "
            .. PROJECTILE_TEMPLATE_NAME)
    end
    return template
end

local function findEnemyInRange(position, range)
    local range_squared = range * range
    local target
    local closest_distance_squared = range_squared

    world.each({"Name", "Transform2D", "Health"},
        function(enemy, name, transform, health)
            if name.value ~= ENEMY_NAME or health:is_dead() then
                return
            end

            local dx = transform.position.x - position.x
            local dy = transform.position.y - position.y
            local distance_squared = dx * dx + dy * dy
            if distance_squared <= closest_distance_squared then
                closest_distance_squared = distance_squared
                target = enemy
            end
        end)

    return target
end

local function spawnProjectile(e, data, target, damage)
    local tower_transform = world.get(e, "Transform2D")
    local template = data.projectile_template
    local projectile_transform = world.get(template, "Transform2D")
    local projectile = world.spawn()

    world.add(projectile,
        "Transform2D",
        Transform2D.new(Vector2.new(tower_transform.position.x,
            tower_transform.position.y),
            projectile_transform.scale,
            projectile_transform.rotation))
    world.add(projectile, "Sprite", world.get(template, "Sprite"))

    local projectile_data = world.data(projectile)
    projectile_data.target = target
    projectile_data.damage = damage
    projectile_data.speed = PROJECTILE_SPEED
    world.add(projectile, "Script", {PROJECTILE_SCRIPT})
end

function tower.on_start(e)
    local data = world.data(e)
    data.level = data.level or 1
    data.attack_timer = 0
    data.projectile_template = findProjectileTemplate()
    if LEVELS[data.level] == nil then
        error("Unsupported tower level: " .. tostring(data.level))
    end
end

function tower.on_update(e, dt)
    local data = world.data(e)
    local stats = LEVELS[data.level]
    data.attack_timer = data.attack_timer + dt
    if data.attack_timer < stats.interval then
        return
    end

    local transform = world.get(e, "Transform2D")
    local target = findEnemyInRange(transform.position, stats.range)
    if target == nil then
        return
    end

    spawnProjectile(e, data, target, stats.damage)
    data.attack_timer = 0
end

return tower

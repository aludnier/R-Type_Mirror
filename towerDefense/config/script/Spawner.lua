local SPAWN_POSITION = {-318, 6.50}
local ENEMY_SCRIPT = "towerDefense/config/script/enemies.lua"

local waves = {
    {
        {type = "enemy_normal_slime", count = 4, interval = 1.0, script = ENEMY_SCRIPT},
    },
    {
        {type = "enemy_normal_slime", count = 6, interval = 0.9, script = ENEMY_SCRIPT},
        {type = "enemy_skeleton", count = 2, interval = 1.0, script = ENEMY_SCRIPT},
    },
    {
        {type = "enemy_skeleton", count = 5, interval = 0.8, script = ENEMY_SCRIPT},
        {type = "enemy_zombie", count = 2, interval = 1.0, script = ENEMY_SCRIPT},
    },
    {
        {type = "enemy_normal_slime", count = 6, interval = 0.7, script = ENEMY_SCRIPT},
        {type = "enemy_zombie", count = 4, interval = 0.8, script = ENEMY_SCRIPT},
    },
    {
        {type = "enemy_skeleton", count = 6, interval = 0.7, script = ENEMY_SCRIPT},
        {type = "enemy_demon", count = 2, interval = 1.2, script = ENEMY_SCRIPT},
    },
    {
        {type = "enemy_zombie", count = 8, interval = 0.65, script = ENEMY_SCRIPT},
        {type = "enemy_demon", count = 3, interval = 1.0, script = ENEMY_SCRIPT},
    },
    {
        {type = "enemy_skeleton", count = 8, interval = 0.6, script = ENEMY_SCRIPT},
        {type = "enemy_demon", count = 4, interval = 0.9, script = ENEMY_SCRIPT},
    },
    {
        {type = "enemy_zombie", count = 10, interval = 0.55, script = ENEMY_SCRIPT},
        {type = "enemy_demon", count = 5, interval = 0.8, script = ENEMY_SCRIPT},
    },
    {
        {type = "enemy_skeleton", count = 10, interval = 0.5, script = ENEMY_SCRIPT},
        {type = "enemy_zombie", count = 8, interval = 0.55, script = ENEMY_SCRIPT},
        {type = "enemy_demon", count = 5, interval = 0.75, script = ENEMY_SCRIPT},
    },
    {
        {type = "enemy_skeleton", count = 12, interval = 0.45, script = ENEMY_SCRIPT},
        {type = "enemy_zombie", count = 10, interval = 0.5, script = ENEMY_SCRIPT},
        {type = "enemy_demon", count = 8, interval = 0.65, script = ENEMY_SCRIPT},
    },
}

local spawner = {}

local function collectTemplates(data)
    data.templates = {}
    world.each({"Name", "Sprite", "Health"}, function(template_entity, name)
        local template_type = name.value
        if template_type ~= nil and template_type ~= "" then
            if data.templates[template_type] ~= nil then
                error("Duplicate enemy template Name: " .. template_type)
            end
            data.templates[template_type] = template_entity
        end
    end)

    for wave_index, wave in ipairs(waves) do
        for _, group in ipairs(wave) do
            if type(group.type) ~= "string"
                or type(group.count) ~= "number"
                or group.count < 1
                or group.count ~= math.floor(group.count)
                or type(group.interval) ~= "number"
                or group.interval < 0 then
                error("Invalid enemy group in wave " .. wave_index)
            end
            if data.templates[group.type] == nil then
                error("Missing enemy template: " .. group.type)
            end
        end
    end
end

local function spawnEnemy(data, group)
    local template_entity = data.templates[group.type]
    local sprite = world.get(template_entity, "Sprite")
    local health = world.get(template_entity, "Health")
    local transform = world.get(template_entity, "Transform2D")
    if sprite == nil or health == nil then
        error("Enemy template is missing Sprite or Health: " .. group.type)
    end
    if transform == nil then
        error("Enemy template is missing Transform2D: " .. group.type)
    end

    local enemy = world.spawn()
    world.add(enemy,
        "Transform2D",
        Transform2D.new(Vector2.new(SPAWN_POSITION[1], SPAWN_POSITION[2]),
            transform.scale,
            transform.rotation))
    world.add(enemy, "Sprite", sprite)
    world.add(enemy, "Health", health)
    world.add(enemy, "Name", Name.new("active_enemy"))
    world.add(enemy, "Script", {group.script or ENEMY_SCRIPT})
end

function spawner.on_start(e)
    local data = world.data(e)
    data.wave_index = 1
    data.group_index = 1
    data.spawned_in_group = 0
    data.timer = 0
    data.wave_active = false
    data.next_wave_was_pressed = false
    collectTemplates(data)
end

function spawner.on_update(e, dt)
    local data = world.data(e)
    local next_wave_pressed = false
    world.each({"Controllable"}, function(_, controllable)
        if controllable:is_active(Action.NextWave) then
            next_wave_pressed = true
        end
    end)

    local launch_pressed =
        next_wave_pressed and not data.next_wave_was_pressed
    data.next_wave_was_pressed = next_wave_pressed

    if not data.wave_active then
        if not launch_pressed or waves[data.wave_index] == nil then
            return
        end
        data.wave_active = true
        data.group_index = 1
        data.spawned_in_group = 0
        data.timer = 0
    else
        data.timer = data.timer + dt
    end

    local wave = waves[data.wave_index]
    local group = wave[data.group_index]
    if group == nil then
        data.wave_index = data.wave_index + 1
        data.group_index = 1
        data.wave_active = false
        data.timer = 0
        return
    end

    if data.spawned_in_group > 0 and data.timer < group.interval then
        return
    end

    spawnEnemy(data, group)
    data.spawned_in_group = data.spawned_in_group + 1
    data.timer = 0

    if data.spawned_in_group >= group.count then
        data.group_index = data.group_index + 1
        data.spawned_in_group = 0
    end
end

return spawner

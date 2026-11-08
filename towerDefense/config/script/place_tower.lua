local TOWER_TEMPLATE_NAMES = {
    "tower_archer",
    "tower_archer_level_2",
    "tower_archer_level_3",
}
local TOWER_SCRIPT = "towerDefense/config/script/tower.lua"
local MAX_TOWER_LEVEL = #TOWER_TEMPLATE_NAMES
local SLOT_NAME_PREFIX = "tower_slot_"
local SPAWNER_NAME = "EnemieSpawner"
local STARTING_TOWER_PLACEMENTS = 3

local BUILD_SLOTS = {
    {-257.500, -50.500},
    {-142.500, -50.500},
    {-27.500, -116.500},
    {-27.500, -7.500},
    {-7, 110.500},
    {130, 36},
    {130, -83},
}

local place_tower = {}

local function collectTemplates(data)
    data.templates = {}
    local names_to_levels = {}
    for level, name in ipairs(TOWER_TEMPLATE_NAMES) do
        names_to_levels[name] = level
    end

    world.each({"Name", "Sprite", "Transform2D"}, function(e, name)
        local level = names_to_levels[name.value]
        if level ~= nil then
            data.templates[level] = e
        end
    end)

    if data.templates[1] == nil then
        error("Missing tower template named " .. TOWER_TEMPLATE_NAMES[1])
    end
end

local function findSpawner()
    local spawner
    world.each({"Name"}, function(entity, name)
        if name.value == SPAWNER_NAME then
            spawner = entity
        end
    end)

    if spawner == nil then
        error("Missing wave spawner named " .. SPAWNER_NAME)
    end
    return spawner
end

local function getTowerLevel(entity)
    local data = world.data(entity)
    data.level = data.level or 1
    if data.level < 1 or data.level > MAX_TOWER_LEVEL then
        error("Invalid tower level: " .. tostring(data.level))
    end
    return data.level
end

local function getBuildAction(data, slot_index)
    if data.actions_remaining <= 0 then
        return nil
    end

    local tower = data.slot_towers[slot_index]
    if tower == nil then
        return "build", 1, nil
    end

    local level = getTowerLevel(tower)
    if level >= MAX_TOWER_LEVEL then
        return nil
    end
    return "upgrade", level + 1, tower
end

local function findNextAction(data, start_slot, direction)
    for offset = 0, #BUILD_SLOTS - 1 do
        local index =
            ((start_slot - 1 + direction * offset) % #BUILD_SLOTS) + 1
        local mode, level, target = getBuildAction(data, index)
        if mode ~= nil then
            return index, mode, level, target
        end
    end
    return nil
end

local function updatePreview(data, slot_index, mode, level, target)
    local template = data.templates[level]
    if template == nil then
        error("Missing tower template named " .. TOWER_TEMPLATE_NAMES[level])
    end

    local template_transform = world.get(template, "Transform2D")
    local preview_transform = world.get(data.preview_entity, "Transform2D")
    local slot = BUILD_SLOTS[slot_index]
    local scale = template_transform.scale
    local rotation = template_transform.rotation

    if mode == "upgrade" then
        local target_transform = world.get(target, "Transform2D")
        scale = target_transform.scale
        rotation = target_transform.rotation
    end

    preview_transform.position.x = slot[1]
    preview_transform.position.y = slot[2]
    preview_transform.scale = scale
    preview_transform.rotation = rotation
    world.add(data.preview_entity, "Sprite", world.get(template, "Sprite"))
    local sprite = world.get(data.preview_entity, "Sprite")
    sprite.r = 0
    sprite.g = 255
    sprite.b = 0

    data.preview_slot = slot_index
    data.preview_mode = mode
    data.preview_level = level
    data.preview_target = target
end

local function startPreview(data)
    local slot_index, mode, level, target = findNextAction(data, 1, 1)
    if slot_index == nil then
        return
    end

    local template = data.templates[level]
    if template == nil then
        error("Missing tower template named " .. TOWER_TEMPLATE_NAMES[level])
    end
    local template_transform = world.get(template, "Transform2D")
    local slot = BUILD_SLOTS[slot_index]
    local preview = world.spawn()
    world.add(preview,
        "Transform2D",
        Transform2D.new(Vector2.new(slot[1], slot[2]),
            template_transform.scale,
            template_transform.rotation))
    world.add(preview, "Sprite", world.get(template, "Sprite"))
    world.add(preview, "Name", Name.new("tower_preview"))

    data.preview_entity = preview
    updatePreview(data, slot_index, mode, level, target)
end

local function confirmPreview(data)
    local slot_index = data.preview_slot
    local preview = data.preview_entity
    local final_sprite =
        world.get(data.templates[data.preview_level], "Sprite")

    if data.preview_mode == "build" then
        local tower = preview
        world.get(tower, "Name").value = SLOT_NAME_PREFIX .. slot_index
        world.add(tower, "Sprite", final_sprite)
        world.add(tower, "Script", {TOWER_SCRIPT})
        world.data(tower).level = data.preview_level
        data.slot_towers[slot_index] = tower
    else
        local target = data.preview_target
        world.add(target, "Sprite", final_sprite)
        world.data(target).level = data.preview_level
        world.kill(preview)
    end
    data.actions_remaining = data.actions_remaining - 1

    data.preview_entity = nil
    data.preview_slot = nil
    data.preview_mode = nil
    data.preview_level = nil
    data.preview_target = nil
end

local function cyclePreview(data, direction)
    local slot_index, mode, level, target = findNextAction(
        data, data.preview_slot + direction, direction)
    if slot_index ~= nil then
        updatePreview(data, slot_index, mode, level, target)
    end
end

function place_tower.on_start(e)
    local data = world.data(e)
    collectTemplates(data)
    data.spawner = findSpawner()
    data.slot_towers = {}
    data.actions_remaining = STARTING_TOWER_PLACEMENTS
    data.observed_wave_index = world.data(data.spawner).wave_index or 1
    data.preview_entity = nil
    data.preview_slot = nil
    data.build_was_pressed = false
    data.up_was_pressed = false
    data.down_was_pressed = false

    world.each({"Name"}, function(entity, name)
        if name.value:sub(1, #SLOT_NAME_PREFIX) == SLOT_NAME_PREFIX then
            local slot_index =
                tonumber(name.value:sub(#SLOT_NAME_PREFIX + 1))
            if slot_index ~= nil and BUILD_SLOTS[slot_index] ~= nil then
                data.slot_towers[slot_index] = entity
            end
        end
    end)
end

function place_tower.on_update(e, dt)
    local data = world.data(e)
    local spawner_data = world.data(data.spawner)
    local wave_index =
        spawner_data.wave_index or data.observed_wave_index
    if wave_index > data.observed_wave_index then
        data.actions_remaining =
            data.actions_remaining + wave_index - data.observed_wave_index
        data.observed_wave_index = wave_index
    end

    local input = world.get(e, "Controllable")
    if input == nil then
        error("Tower placement controller needs a Controllable component")
    end

    local build_pressed = input:is_active(Action.Build)
    local up_pressed = input:is_active(Action.MoveUp)
    local down_pressed = input:is_active(Action.MoveDown)

    if build_pressed and not data.build_was_pressed then
        if data.preview_entity == nil then
            if data.actions_remaining > 0 then
                startPreview(data)
            end
        else
            confirmPreview(data)
        end
    end

    if data.preview_entity ~= nil then
        if up_pressed and not data.up_was_pressed then
            cyclePreview(data, -1)
        elseif down_pressed and not data.down_was_pressed then
            cyclePreview(data, 1)
        end
    end

    data.build_was_pressed = build_pressed
    data.up_was_pressed = up_pressed
    data.down_was_pressed = down_pressed
end

return place_tower

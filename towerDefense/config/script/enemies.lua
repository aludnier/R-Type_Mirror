local SPEED = 40

local spawn = {-318, 6.50}

local checkpoints = {
    {-200, 6.50},
    {-200, -108},
    {-83, -108},
    {-83, 43},
    {80, 43},
    {80, -31},
    {360, -31},
}

local enemy = {}

function enemy.on_start(e)
    local position = world.get(e, "Transform2D").position
    position.x = spawn[1]
    position.y = spawn[2]
    world.data(e).checkpoint_index = 1
end

function enemy.on_update(e, dt)
    local data = world.data(e)
    local checkpoint = checkpoints[data.checkpoint_index]

    if checkpoint == nil then
        return
    end

    local position = world.get(e, "Transform2D").position
    local dx = checkpoint[1] - position.x
    local dy = checkpoint[2] - position.y
    local distance = math.sqrt(dx * dx + dy * dy)
    local step = SPEED * dt

    if distance <= step then
        position.x = checkpoint[1]
        position.y = checkpoint[2]
        data.checkpoint_index = data.checkpoint_index + 1
        if data.checkpoint_index > #checkpoints then
            world.kill(e)
        end
    elseif distance > 0 then
        position.x = position.x + dx / distance * step
        position.y = position.y + dy / distance * step
    end
end

return enemy

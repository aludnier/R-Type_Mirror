-- Entity script: ForcePod follows its owner's position with an offset.

local forcepod = {}

function forcepod.on_update(e, dt)
    local pod = world.get(e, "ForcePod")
    local transform = world.get(e, "Transform2D")
    if not pod or not transform then
        return
    end
    
    local ownerTransform = world.get(pod.owner_id, "Transform2D")
    if ownerTransform then
        transform.position.x = ownerTransform.position.x + pod.offset_x
        transform.position.y = ownerTransform.position.y + pod.offset_y
    end
end

return forcepod

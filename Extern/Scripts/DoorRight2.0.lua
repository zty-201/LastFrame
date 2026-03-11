-- Door_SharedControlled_Right.lua
-- RIGHT door is no longer player-interactable.
-- It only supports a scripted close command.

local closedAngle = 0.0
local rotateSpeed = 180.0

local currentY = 0.0
local targetY = 0.0

function on_create(Self)
    print("[DoorR] on_create EntityID:", EntityID)

    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[DoorR] ERROR: No Transform found")
        return
    end

    local _, ry, _ = tr:GetRotation()
    currentY = ry
    targetY = ry

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}

    print(string.format("[DoorR] Start RotY=%.2f", currentY))
end

function on_update(Self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end
    dt = dt or 0.016

    -- Only accept scripted close command
    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil

        local cmd = cmdObj.cmd

        if cmd == "close" then
            targetY = closedAngle
            print(string.format("[DoorR] Scripted CLOSE by=%s", tostring(cmdObj.by)))
        else
            print("[DoorR] Ignored cmd:", tostring(cmd))
        end
    end

    -- Rotation
    local diff = targetY - currentY
    if math.abs(diff) > 0.05 then
        local dir = (diff > 0) and 1 or -1
        currentY = currentY + dir * rotateSpeed * dt

        if dir > 0 then
            if currentY > targetY then currentY = targetY end
        else
            if currentY < targetY then currentY = targetY end
        end

        local rx, _, rz = tr:GetRotation()
        tr:SetRotation(rx, currentY, rz)
    end
end
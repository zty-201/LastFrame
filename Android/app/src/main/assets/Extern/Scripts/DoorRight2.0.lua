-- Door_SharedControlled_Right.lua
-- Attach to RIGHT Door entity
-- Opens/closes when commanded via SHARED.DOOR_CMD[EntityID]

local closedAngle = 0.0
local openAngle   = -80.0      -- RIGHT door usually swings the opposite way
local rotateSpeed = 180.0

local isOpen = false
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

    -- Consume command from SHARED
    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil

        if cmdObj.cmd == "toggle" then
            isOpen = not isOpen
        elseif cmdObj.cmd == "open" then
            isOpen = true
        elseif cmdObj.cmd == "close" then
            isOpen = false
        end

        targetY = isOpen and openAngle or closedAngle

        print(string.format(
            "[DoorR] CMD=%s by=%s -> isOpen=%s targetY=%.2f",
            tostring(cmdObj.cmd),
            tostring(cmdObj.by),
            tostring(isOpen),
            targetY
        ))
    end

    -- Rotation logic (same as left door)
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

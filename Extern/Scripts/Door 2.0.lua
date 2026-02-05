-- Door_SharedControlled.lua
-- Attach to Door entity
-- Opens/closes when commanded via SHARED (from InteractionBox / DoorTrigger)

local closedAngle = 0.0
local openAngle   = 80.0
local rotateSpeed = 180.0

local isOpen = false
local currentY = 0.0
local targetY = 0.0

function on_create(Self)
    print("[Door] on_create EntityID:", EntityID)

    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[Door] ERROR: No Transform found")
        return
    end

    local _, ry, _ = tr:GetRotation()
    currentY = ry
    targetY = ry

    -- Ensure shared table exists
    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}

    print(string.format("[Door] Start RotY=%.2f", currentY))
end

function on_update(Self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end

    ----------------------------------------------------------------
    -- NEW: consume interaction command from SHARED
    ----------------------------------------------------------------
    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil   -- consume it (IMPORTANT)

        if cmdObj.cmd == "toggle" then
            isOpen = not isOpen
        elseif cmdObj.cmd == "open" then
            isOpen = true
        elseif cmdObj.cmd == "close" then
            isOpen = false
        end

        targetY = isOpen and openAngle or closedAngle

        print(string.format(
            "[Door] CMD=%s by=%s -> isOpen=%s targetY=%.2f",
            tostring(cmdObj.cmd),
            tostring(cmdObj.by),
            tostring(isOpen),
            targetY
        ))
    end

    ----------------------------------------------------------------
    -- Rotation logic (UNCHANGED from your original)
    ----------------------------------------------------------------
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

-- BedroomDoor_SharedControlled.lua
-- Attach to BEDROOM DOOR entity
-- Starts CLOSED.
-- Opens ONCE, then ignores future interaction forever.
-- Publishes SHARED.BEDROOM_DOOR_OPENED_ONCE[EntityID] for one-time icon logic.

local closedAngle = 0.0
local openAngle   = -100.0
local rotateSpeed = 50.0

local currentY = 0.0
local targetY  = 0.0
local hasOpenedOnce = false

local DOOR_OPEN_SFX_ID = "OpenDoor" -- optional slot id

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end
    return false
end

function on_create(Self)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[BedroomDoor] ERROR: No Transform found for EntityID=" .. tostring(EntityID))
        return
    end

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
    SHARED.BEDROOM_DOOR_OPENED_ONCE = SHARED.BEDROOM_DOOR_OPENED_ONCE or {}

    -- Always start CLOSED
    currentY = closedAngle
    targetY  = closedAngle
    hasOpenedOnce = false
    SHARED.BEDROOM_DOOR_OPENED_ONCE[EntityID] = false

    local rx, _, rz = tr:GetRotation()
    tr:SetRotation(rx, currentY, rz)

    print(string.format("[BedroomDoor] Start CLOSED EntityID=%d RotY=%.2f", EntityID, currentY))
end

function on_update(Self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end
    dt = dt or 0.016

    -- Consume interaction command (from InteractionBox)
    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil

        local cmd = cmdObj.cmd

        if not hasOpenedOnce then
            if cmd == "open" or cmd == "toggle" then
                hasOpenedOnce = true
                targetY = openAngle

                SHARED.BEDROOM_DOOR_OPENED_ONCE[EntityID] = true
                play_3d_sfx(DOOR_OPEN_SFX_ID)

                print(string.format("[BedroomDoor] OPENED ONCE by=%s -> now disabled", tostring(cmdObj.by)))
            end
        end
        -- if opened once: ignore forever
    end

    -- Rotate toward target
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

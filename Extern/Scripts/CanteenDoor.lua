-- CanteenDoor.lua
-- Attach to CANTEEN DOOR entity
-- Logic:
-- 1. Starts at the rotation set in editor
-- 2. That starting rotation is treated as the OPEN state
-- 3. When triggered, door CLOSES by rotating 90 degrees
-- 4. Door becomes LOCKED after closing
-- 5. After player unlocks and opens it once, it can no longer be closed by interaction
-- 6. Opening the door after unlock activates the upstairs ghost girl scare

local closeOffset = -90.0
local openSpeed  = 60.0
local closeSpeed = 200.0

local openY = 0.0
local closedY = 0.0
local currentY = 0.0
local targetY = 0.0

local isOpen = true
local isLocked = false
local hasClosedAndLocked = false
local hasBeenOpenedByPlayer = false

local DOOR_OPEN_SFX_ID   = "OpenDoor"
local DOOR_CLOSE_SFX_ID  = "CloseDoor"
local DOOR_LOCKED_SFX_ID = "DoorLocked"

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end
    return false
end

local function activate_sf_girl()
    SHARED.SF_GIRL_ACTIVE = true
    print("[CanteenDoor] Activated SF_GIRL_ACTIVE")
end

function on_create(self)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[CanteenDoor] ERROR: No Transform found for EntityID=" .. tostring(EntityID))
        return
    end

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
    SHARED.CANTEEN_DOOR_LOCKED = SHARED.CANTEEN_DOOR_LOCKED or {}
    SHARED.CANTEEN_DOOR_OPEN = SHARED.CANTEEN_DOOR_OPEN or {}
    SHARED.SF_GIRL_ACTIVE = SHARED.SF_GIRL_ACTIVE or false

    local rx, ry, rz = tr:GetRotation()

    openY = ry
    closedY = openY + closeOffset

    currentY = openY
    targetY = openY

    isOpen = true
    isLocked = false
    hasClosedAndLocked = false
    hasBeenOpenedByPlayer = false

    SHARED.CANTEEN_DOOR_LOCKED[EntityID] = false
    SHARED.CANTEEN_DOOR_OPEN[EntityID] = true
    SHARED.SF_GIRL_ACTIVE = false

    tr:SetRotation(rx, currentY, rz)

    print(string.format(
        "[CanteenDoor] Start OPEN EntityID=%d openY=%.2f closedY=%.2f",
        EntityID, openY, closedY
    ))
end

function on_update(self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end
    dt = dt or 0.016

    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil

        local cmd = cmdObj.cmd

        if cmd == "toggle" then
            if isLocked then
                play_3d_sfx(DOOR_LOCKED_SFX_ID)
                print(string.format("[CanteenDoor] TOGGLE blocked, door is LOCKED by=%s", tostring(cmdObj.by)))
            else
                if isOpen then
                    if hasBeenOpenedByPlayer then
                        print(string.format("[CanteenDoor] TOGGLE ignored, door cannot be closed anymore by=%s", tostring(cmdObj.by)))
                    else
                        targetY = closedY
                        isOpen = false
                        SHARED.CANTEEN_DOOR_OPEN[EntityID] = false

                        play_3d_sfx(DOOR_CLOSE_SFX_ID)

                        print(string.format("[CanteenDoor] TOGGLE -> CLOSE by=%s", tostring(cmdObj.by)))
                    end
                else
                    targetY = openY
                    isOpen = true
                    hasBeenOpenedByPlayer = true
                    SHARED.CANTEEN_DOOR_OPEN[EntityID] = true

                    activate_sf_girl()
                    play_3d_sfx(DOOR_OPEN_SFX_ID)

                    print(string.format("[CanteenDoor] TOGGLE -> OPEN by=%s", tostring(cmdObj.by)))
                end
            end

        elseif cmd == "open" then
            if isLocked then
                play_3d_sfx(DOOR_LOCKED_SFX_ID)
                print(string.format("[CanteenDoor] OPEN blocked, door is LOCKED by=%s", tostring(cmdObj.by)))
            else
                targetY = openY
                isOpen = true
                hasBeenOpenedByPlayer = true
                SHARED.CANTEEN_DOOR_OPEN[EntityID] = true

                activate_sf_girl()
                play_3d_sfx(DOOR_OPEN_SFX_ID)

                print(string.format("[CanteenDoor] OPEN command received by=%s", tostring(cmdObj.by)))
            end

        elseif cmd == "close" then
            if not hasClosedAndLocked and not hasBeenOpenedByPlayer then
                targetY = closedY
                isOpen = false
                SHARED.CANTEEN_DOOR_OPEN[EntityID] = false

                play_3d_sfx(DOOR_CLOSE_SFX_ID)

                print(string.format("[CanteenDoor] CLOSE command received by=%s", tostring(cmdObj.by)))
            end

        elseif cmd == "close_and_lock" then
            if not hasClosedAndLocked then
                targetY = closedY
                isOpen = false
                isLocked = true
                hasClosedAndLocked = true

                SHARED.CANTEEN_DOOR_OPEN[EntityID] = false
                SHARED.CANTEEN_DOOR_LOCKED[EntityID] = true

                play_3d_sfx(DOOR_CLOSE_SFX_ID)

                print(string.format("[CanteenDoor] CLOSED AND LOCKED by=%s", tostring(cmdObj.by)))
            end

        elseif cmd == "unlock" then
            isLocked = false
            SHARED.CANTEEN_DOOR_LOCKED[EntityID] = false

            print(string.format("[CanteenDoor] UNLOCKED by=%s", tostring(cmdObj.by)))
        end
    end

    local diff = targetY - currentY
    if math.abs(diff) > 0.05 then
        local dir = (diff > 0) and 1 or -1
        local speed = closeSpeed

        if targetY == openY then
            speed = openSpeed
        else
            speed = closeSpeed
        end

        currentY = currentY + dir * speed * dt

        if dir > 0 then
            if currentY > targetY then currentY = targetY end
        else
            if currentY < targetY then currentY = targetY end
        end

        local rx, _, rz = tr:GetRotation()
        tr:SetRotation(rx, currentY, rz)
    end
end
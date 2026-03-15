-- Level2Gate_SharedControlled.lua
-- Attach to LEVEL 2 CHAIN GATE entity
-- Same logic pattern as PlayroomDoor_SharedControlled.lua
-- Requires "boltcutter" to unlock.

local DEBUG = true

-- ===== ROTATION CONFIG =====
local closedAngle = 0.0
local openAngle   = 4.5
local rotateSpeed = 50.0

local isOpen     = false
local isUnlocked = false
local currentY   = 0.0
local targetY    = 0.0

-- ✅ once gate opened at least once, icons never show again
local iconsDisabledForever = false

-- ===== 3D SFX IDs =====
local SFX_GATE_LOCKED  = "DoorLocked"
local SFX_GATE_UNLOCK  = "DoorUnlock"
local SFX_GATE_OPEN    = "DoorOpen"

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and type(slotId) == "string" then
        Audio_PlaySlot(EntityID, slotId) -- 3D from the GATE entity
    end
end

-- ===== ITEM CONFIG =====
local REQUIRED_KEY_ID = "boltcutter"   -- ✅ your inventory item id
local consumeOnFirstUse = true         -- set false if boltcutter should NOT be consumed

-- ===== INTERACTABLE ID =====
local INTERACTABLE_ID = 185 -- ✅ set this to the Level2Gate interactable/trigger EntityID

-- -------------------------
-- Inventory helpers
-- -------------------------
local function inv_has_item(itemId)
    if not SHARED or not SHARED.Inventory or not SHARED.Inventory.Count then
        return false
    end
    return (SHARED.Inventory.Count(itemId) or 0) >= 1
end

local function inv_consume_item(itemId, qty)
    qty = qty or 1
    if not SHARED or not SHARED.Inventory or not SHARED.Inventory.Remove then
        return false
    end
    return SHARED.Inventory.Remove(itemId, qty) == true
end

-- -------------------------
-- Publish unlock state for UI decision
-- -------------------------
local function publish_can_unlock()
    SHARED.LEVEL2GATE_CAN_UNLOCK = SHARED.LEVEL2GATE_CAN_UNLOCK or {}
    local v = (isUnlocked == true) or inv_has_item(REQUIRED_KEY_ID)
    SHARED.LEVEL2GATE_CAN_UNLOCK[INTERACTABLE_ID] = v
end

-- -------------------------
-- Icon command helpers (same cmd tables as Playroom)
-- -------------------------
local function send_lock_cmd(cmd)
    SHARED.LOCK_ICON_CMD = SHARED.LOCK_ICON_CMD or {}
    SHARED.LOCK_ICON_CMD[INTERACTABLE_ID] = { cmd = cmd, by = EntityID }
end

local function send_unlock_cmd(cmd)
    SHARED.UNLOCK_ICON_CMD = SHARED.UNLOCK_ICON_CMD or {}
    SHARED.UNLOCK_ICON_CMD[INTERACTABLE_ID] = { cmd = cmd, by = EntityID }
end

local function in_range()
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[INTERACTABLE_ID] == true
end

local function hide_icons_forever()
    send_lock_cmd("hide")
    send_unlock_cmd("hide")
end

local function update_icons()
    if iconsDisabledForever then
        hide_icons_forever()
        return
    end

    if not in_range() then
        send_lock_cmd("hide")
        send_unlock_cmd("hide")
        return
    end

    local canUnlock = (isUnlocked == true) or inv_has_item(REQUIRED_KEY_ID)

    if canUnlock then
        send_lock_cmd("hide")
        send_unlock_cmd("show")
    else
        send_lock_cmd("show")
        send_unlock_cmd("hide")
    end
end

-- -------------------------
-- Lifecycle
-- -------------------------
function on_create(self)
    print("[Level2Gate] on_create EntityID=" .. tostring(EntityID) .. " INTERACTABLE_ID=" .. tostring(INTERACTABLE_ID))

    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[Level2Gate] ERROR: No Transform found on gate entity")
        return
    end

    local _, ry, _ = tr:GetRotation()
    currentY = ry
    targetY  = ry

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED.LEVEL2GATE_CAN_UNLOCK = SHARED.LEVEL2GATE_CAN_UNLOCK or {}
    SHARED.LOCK_ICON_CMD = SHARED.LOCK_ICON_CMD or {}
    SHARED.UNLOCK_ICON_CMD = SHARED.UNLOCK_ICON_CMD or {}

    publish_can_unlock()

    -- start state
    if isOpen then
        iconsDisabledForever = true
        hide_icons_forever()
    else
        update_icons()
    end
end

function on_update(self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end
    dt = dt or 0.016

    local isRotating = math.abs(targetY - currentY) > 0.05

    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil

        local cmd = cmdObj.cmd

        -- Ignore interaction commands while the door is still rotating
        if not isRotating then
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
                        SHARED.CANTEEN_DOOR_OPENED_ONCE[EntityID] = true

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
                    SHARED.CANTEEN_DOOR_OPENED_ONCE[EntityID] = true

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
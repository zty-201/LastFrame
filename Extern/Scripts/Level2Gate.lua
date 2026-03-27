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

-- Entity to hide (scale to 0) when the gate unlocks
local HIDE_ON_UNLOCK_ID = 401

local function hide_entity_401()
    local tr = Query.GetTransform(HIDE_ON_UNLOCK_ID)
    if tr then
        tr:SetScale(0, 0, 0)
        if DEBUG then print("[Level2Gate] Entity " .. tostring(HIDE_ON_UNLOCK_ID) .. " scaled to 0") end
    else
        print("[Level2Gate] WARNING: Could not find transform for entity " .. tostring(HIDE_ON_UNLOCK_ID))
    end
end

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

    -- Update icons every frame so they show/hide as player enters/leaves range
    update_icons()

    local isRotating = math.abs(targetY - currentY) > 0.05

    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil

        local cmd = cmdObj.cmd

        if not isRotating then
            if cmd == "toggle" or cmd == "open" then
                if isOpen then
                    print("[Level2Gate] Already open, ignoring command")
                elseif not isUnlocked then
                    -- Try to unlock with boltcutter
                    if inv_has_item(REQUIRED_KEY_ID) then
                        if consumeOnFirstUse then
                            inv_consume_item(REQUIRED_KEY_ID)
                        end
                        isUnlocked = true
                        play_3d_sfx(SFX_GATE_UNLOCK)
                        print("[Level2Gate] Unlocked with boltcutter")

                        -- Open immediately after unlocking
                        targetY = openAngle
                        isOpen  = true
                        iconsDisabledForever = true
                        play_3d_sfx(SFX_GATE_OPEN)
                        hide_entity_401()
                        publish_can_unlock()
                        print("[Level2Gate] Gate opening")
                    else
                        play_3d_sfx(SFX_GATE_LOCKED)
                        print("[Level2Gate] Locked - need boltcutter")
                    end
                else
                    targetY = openAngle
                    isOpen  = true
                    iconsDisabledForever = true
                    play_3d_sfx(SFX_GATE_OPEN)
                    hide_entity_401()
                    print("[Level2Gate] Gate opening")
                end
            end
        end
    end

    -- Rotate gate towards target
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
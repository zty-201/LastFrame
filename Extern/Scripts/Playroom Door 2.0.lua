-- PlayroomDoor_SharedControlled.lua
-- Attach to PLAYROOM DOOR entity

local DEBUG = true

-- ===== ROTATION CONFIG =====
local closedAngle = 0.0
local openAngle   = -240.0
local rotateSpeed = 50.0

local isOpen     = false
local isUnlocked = false
local currentY   = 0.0
local targetY    = 0.0

-- ✅ NEW: once door has opened at least once, icons never show again
local iconsDisabledForever = false

-- ===== 3D SFX IDs =====
local SFX_DOOR_LOCKED  = "DoorLocked"
local SFX_DOOR_UNLOCK  = "DoorUnlock"
local SFX_DOOR_OPEN    = "DoorOpen"

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and type(slotId) == "string" then
        Audio_PlaySlot(EntityID, slotId) -- plays from the DOOR entity (3D)
    end
end

-- ===== KEY CONFIG =====
local REQUIRED_KEY_ID = "playroom_key"
local consumeOnFirstUse = true

-- ===== INTERACTABLE ID =====
local INTERACTABLE_ID = 33

-- -------------------------
-- Inventory helpers
-- -------------------------
local function inv_has_key(keyId)
    if not SHARED or not SHARED.Inventory or not SHARED.Inventory.Count then
        return false
    end
    return (SHARED.Inventory.Count(keyId) or 0) >= 1
end

local function inv_consume_key(keyId, qty)
    qty = qty or 1
    if not SHARED or not SHARED.Inventory or not SHARED.Inventory.Remove then
        return false
    end
    return SHARED.Inventory.Remove(keyId, qty) == true
end

-- -------------------------
-- Publish unlock state for UI decision
-- -------------------------
local function publish_can_unlock()
    SHARED.PLAYROOM_CAN_UNLOCK = SHARED.PLAYROOM_CAN_UNLOCK or {}
    local v = (isUnlocked == true) or inv_has_key(REQUIRED_KEY_ID)
    SHARED.PLAYROOM_CAN_UNLOCK[INTERACTABLE_ID] = v
end

-- -------------------------
-- Icon command helpers
-- -------------------------
local function send_lock_cmd(cmd)
    SHARED.LOCK_ICON_CMD = SHARED.LOCK_ICON_CMD or {}
    SHARED.LOCK_ICON_CMD[INTERACTABLE_ID] = { cmd = cmd, by = EntityID }
end

local function send_unlock_cmd(cmd)
    SHARED.UNLOCK_ICON_CMD = SHARED.UNLOCK_ICON_CMD or {}
    SHARED.UNLOCK_ICON_CMD[INTERACTABLE_ID] = { cmd = cmd, by = EntityID }
end

local function send_handle_cmd(cmd)
    SHARED.HANDLE_ICON_CMD = SHARED.HANDLE_ICON_CMD or {}
    SHARED.HANDLE_ICON_CMD[INTERACTABLE_ID] = { cmd = cmd, by = EntityID }
end

local function in_range()
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[INTERACTABLE_ID] == true
end

-- Force-hide both icons and keep them hidden
local function hide_icons_forever()
    send_lock_cmd("hide")
    send_unlock_cmd("hide")
    send_handle_cmd("hide")
end

local function update_icons()
    if iconsDisabledForever then
        hide_icons_forever()
        return
    end

    if not in_range() then
        send_lock_cmd("hide")
        send_unlock_cmd("hide")
        send_handle_cmd("hide")
        return
    end

    if isUnlocked then
        send_lock_cmd("hide")
        send_unlock_cmd("hide")
        send_handle_cmd("show")
        return
    end

    if inv_has_key(REQUIRED_KEY_ID) then
        send_lock_cmd("hide")
        send_unlock_cmd("show")
        send_handle_cmd("hide")
    else
        send_lock_cmd("show")
        send_unlock_cmd("hide")
        send_handle_cmd("hide")
    end
end

-- -------------------------
-- Unlock logic
-- -------------------------
local function can_unlock_and_maybe_consume()
    if isUnlocked then
        return true
    end

    if not inv_has_key(REQUIRED_KEY_ID) then
        if DEBUG then
            print("[PlayroomDoor] Locked: missing key -> " .. tostring(REQUIRED_KEY_ID))
        end
        publish_can_unlock()
        return false
    end

    if consumeOnFirstUse then
        local ok = inv_consume_key(REQUIRED_KEY_ID, 1)
        if not ok then
            print("[PlayroomDoor] ERROR: failed to consume key -> " .. tostring(REQUIRED_KEY_ID))
            publish_can_unlock()
            return false
        end
        if DEBUG then
            print("[PlayroomDoor] Consumed key -> " .. tostring(REQUIRED_KEY_ID))
        end
    end

    isUnlocked = true
    if DEBUG then
        print("[PlayroomDoor] UNLOCKED!")
    end

    publish_can_unlock()
    return true
end

-- -------------------------
-- Lifecycle
-- -------------------------
function on_create(self)
    print("[PlayroomDoor] on_create EntityID=" .. tostring(EntityID) .. " INTERACTABLE_ID=" .. tostring(INTERACTABLE_ID))

    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[PlayroomDoor] ERROR: No Transform found on door entity")
        return
    end

    local _, ry, _ = tr:GetRotation()
    currentY = ry
    targetY  = ry

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED.PLAYROOM_CAN_UNLOCK = SHARED.PLAYROOM_CAN_UNLOCK or {}
    SHARED.HANDLE_ICON_CMD = SHARED.HANDLE_ICON_CMD or {}
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

    publish_can_unlock()
    update_icons()

    local isRotating = math.abs(targetY - currentY) > 0.05

    -- consume interaction command
    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[INTERACTABLE_ID] then
        local cmdObj = SHARED.DOOR_CMD[INTERACTABLE_ID]
        SHARED.DOOR_CMD[INTERACTABLE_ID] = nil

        local cmd = cmdObj.cmd

        -- Once opened, ignore all further interaction commands
        if iconsDisabledForever then
            -- command already consumed/cleared above; just let rotation finish
        -- Ignore interaction while the door is still rotating
        elseif not isRotating then
            -- ========================================================
            -- STATE 1/2: LOCKED or UNLOCKABLE -> handle unlock attempt
            -- ========================================================
            if (cmd == "toggle" or cmd == "open") and (not isUnlocked) then
                if not inv_has_key(REQUIRED_KEY_ID) then
                    -- LOCKED
                    play_3d_sfx(SFX_DOOR_LOCKED)
                    if DEBUG then
                        print("[PlayroomDoor] LOCKED -> DoorLocked SFX")
                    end
                    publish_can_unlock()
                    return
                end

                -- UNLOCKABLE -> unlock now (and optionally consume key), but do NOT open yet
                if consumeOnFirstUse then
                    local ok = inv_consume_key(REQUIRED_KEY_ID, 1)
                    if not ok then
                        print("[PlayroomDoor] ERROR: failed to consume key -> " .. tostring(REQUIRED_KEY_ID))
                        publish_can_unlock()
                        return
                    end
                end

                isUnlocked = true
                play_3d_sfx(SFX_DOOR_UNLOCK)

                if DEBUG then
                    print("[PlayroomDoor] UNLOCKED -> DoorUnlock SFX (door still closed)")
                end

                publish_can_unlock()
                update_icons()
                return
            end

            -- ========================================================
            -- STATE 3: UNLOCKED -> allow open/toggle
            -- ========================================================
            local wasOpen = isOpen

            if cmd == "toggle" then
                isOpen = not isOpen
            elseif cmd == "open" then
                isOpen = true
            elseif cmd == "close" then
                isOpen = false
            end

            targetY = isOpen and openAngle or closedAngle

            -- Play DoorOpen only when transitioning CLOSED -> OPEN
            if (not wasOpen) and isOpen then
                play_3d_sfx(SFX_DOOR_OPEN)
                if DEBUG then
                    print("[PlayroomDoor] OPEN -> DoorOpen SFX")
                end
            end

            -- once opened, disable icons forever
            if isOpen and (not iconsDisabledForever) then
                iconsDisabledForever = true
                hide_icons_forever()
            end

            if DEBUG then
                print(string.format(
                    "[PlayroomDoor] CMD=%s by=%s -> unlocked=%s isOpen=%s targetY=%.2f iconsDisabledForever=%s",
                    tostring(cmd),
                    tostring(cmdObj.by),
                    tostring(isUnlocked),
                    tostring(isOpen),
                    targetY,
                    tostring(iconsDisabledForever)
                ))
            end
        end
    end

    -- rotation
    dt = dt or 0.016
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
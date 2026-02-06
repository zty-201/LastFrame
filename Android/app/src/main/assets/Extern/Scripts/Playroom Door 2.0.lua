-- PlayroomDoor_SharedControlled.lua
-- Attach to PLAYROOM DOOR entity
-- Requires playroom_key ONCE to unlock, then behaves like normal Door_SharedControlled.

local closedAngle = 0.0
local openAngle   = 50.0
local rotateSpeed = 180.0

local isOpen = false
local currentY = 0.0
local targetY = 0.0

-- ===== KEY CONFIG =====
local REQUIRED_KEY_ID = "playroom_key"  -- must match your key script ITEM_ID
local consumeOnFirstUse = true
local isUnlocked = false

local DEBUG = true

local function s(v) return (v == nil) and "nil" or tostring(v) end

-- Uses YOUR inventory.lua API: Count + Remove
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

local function can_unlock_and_maybe_consume()
    if isUnlocked then return true end

    if not inv_has_key(REQUIRED_KEY_ID) then
        if DEBUG then
            print("[PlayroomDoor] Locked: missing key -> " .. REQUIRED_KEY_ID)
        end
        return false
    end

    if consumeOnFirstUse then
        local ok = inv_consume_key(REQUIRED_KEY_ID, 1)
        if not ok then
            print("[PlayroomDoor] ERROR: failed to consume key -> " .. REQUIRED_KEY_ID)
            return false
        end
        if DEBUG then
            print("[PlayroomDoor] Consumed key -> " .. REQUIRED_KEY_ID)
        end
    end

    isUnlocked = true
    if DEBUG then print("[PlayroomDoor] UNLOCKED!") end
    return true
end

function on_create(Self)
    print("[PlayroomDoor] on_create EntityID:", EntityID)

    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[PlayroomDoor] ERROR: No Transform found")
        return
    end

    local _, ry, _ = tr:GetRotation()
    currentY = ry
    targetY = ry

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}

    print(string.format("[PlayroomDoor] Start RotY=%.2f", currentY))
end

function on_update(Self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end

    ----------------------------------------------------------------
    -- consume interaction command from SHARED (same pattern as Door 2.0)
    ----------------------------------------------------------------
    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil   -- consume it (IMPORTANT)

        local cmd = cmdObj.cmd

        -- Require key only when trying to open/toggle (closing is allowed)
        if cmd == "toggle" or cmd == "open" then
            if not can_unlock_and_maybe_consume() then
                return
            end
        end

        if cmd == "toggle" then
            isOpen = not isOpen
        elseif cmd == "open" then
            isOpen = true
        elseif cmd == "close" then
            isOpen = false
        end

        targetY = isOpen and openAngle or closedAngle

        if DEBUG then
            print(string.format(
                "[PlayroomDoor] CMD=%s by=%s -> unlocked=%s isOpen=%s targetY=%.2f",
                tostring(cmd),
                tostring(cmdObj.by),
                tostring(isUnlocked),
                tostring(isOpen),
                targetY
            ))
        end
    end

    ----------------------------------------------------------------
    -- Rotation logic (UNCHANGED)
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

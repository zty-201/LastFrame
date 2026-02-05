----------------------------------------------------------------
-- Test_AddMusicDisc_Shared.lua  (MODIFIED)
-- + Show disc icon when in range
-- + After pickup, disc icon never shows again
----------------------------------------------------------------

local DEBUG = true

local ITEM_ID   = "music_disc"
local ITEM_NAME = "Music Disc"
local ITEM_QTY  = 1

-- If the interactable ID is not the same entity this script is on,
-- set this (otherwise leave as 0).
local TARGET_ENTITY_ID = 0

local pickedUp = false

-- 3D pickup SFX (slot ID)
local DISC_PICKUP_SFX_ID = "DiscPickup"

local function safePlaySlotFrom(entityId, slotId)
    if type(Audio_PlaySlot) == "function" and type(slotId) == "string" and entityId ~= nil then
        -- Force retrigger for one-shots (optional but helps some engines)
        if type(Audio_StopSlot) == "function" then
            Audio_StopSlot(entityId, slotId)
        end
        Audio_PlaySlot(entityId, slotId)
        return true
    end
    return false
end

local function s(v)
    if v == nil then return "nil" end
    return tostring(v)
end

local function consume_shared_cmd(cmdTable, id)
    if cmdTable == nil then return false end

    local cmd = cmdTable[id]
    if cmd == nil then return false end

    cmdTable[id] = nil -- consume one-shot

    if type(cmd) == "table" then
        local c = cmd.cmd
        return (c == nil) or (c == "toggle") or (c == "pickup") or (c == "use") or (c == "take")
    end

    if cmd == true then return true end
    if cmd == "toggle" or cmd == "pickup" or cmd == "use" or cmd == "take" then return true end

    return true
end

local function hide_entity(id)
    local tr = Query.GetTransform(id)
    if tr then
        tr:SetScale(0.0, 0.0, 0.0)
        return true
    end
    return false
end

-- ✅ NEW: icon command bus for disc pickup icon
local function send_disc_icon_cmd(discId, cmd)
    SHARED.DISC_ICON_CMD = SHARED.DISC_ICON_CMD or {}
    SHARED.DISC_ICON_CMD[discId] = { cmd = cmd, by = discId }
end

local function disc_in_range(discId)
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[discId] == true
end

function on_create(Self)
    if DEBUG then
        print("[Test_AddMusicDisc_Shared] on_create EntityID:", EntityID)
        if not SHARED.Inventory or not SHARED.Inventory.Add then
            print("[Test_AddMusicDisc_Shared] WARNING: Inventory not ready (SHARED.Inventory.Add missing)")
        end
    end

    SHARED.DISC_ICON_CMD = SHARED.DISC_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
end

function on_update(Self, dt)
    local pickupId = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    ----------------------------------------------------------------
    -- ✅ NEW: icon show/hide while not picked up
    ----------------------------------------------------------------
    if not pickedUp then
        if disc_in_range(pickupId) then
            send_disc_icon_cmd(pickupId, "show")
        else
            send_disc_icon_cmd(pickupId, "hide")
        end
    else
        -- picked up: force hidden forever
        send_disc_icon_cmd(pickupId, "hide")
        return
    end

    ----------------------------------------------------------------
    -- Wait for InteractionBox command
    ----------------------------------------------------------------
    if SHARED.DOOR_CMD == nil then
        return
    end

    local triggered = consume_shared_cmd(SHARED.DOOR_CMD, pickupId)
    if not triggered then
        return
    end

    if DEBUG then
        print("[Test_AddMusicDisc_Shared] Received interact cmd for id=", s(pickupId))
    end

    -- Play 3D pickup sound FROM the disc entity
    local played = safePlaySlotFrom(pickupId, DISC_PICKUP_SFX_ID)
    if DEBUG then
        print("[Test_AddMusicDisc_Shared] DiscPickup played=", tostring(played), " from id=", s(pickupId))
    end

    -- 1) Add item
    if SHARED.Inventory and SHARED.Inventory.Add then
        SHARED.Inventory.Add(ITEM_ID, ITEM_NAME, ITEM_QTY)
        if DEBUG then
            print("[Test_AddMusicDisc_Shared] Added:", ITEM_NAME, "x" .. ITEM_QTY)
        end
    else
        print("[Test_AddMusicDisc_Shared] ERROR: Inventory not available")
        return
    end

    -- 2) Hide / shrink
    local ok = hide_entity(pickupId)
    if DEBUG then
        print("[Test_AddMusicDisc_Shared] Hide result=", tostring(ok), " id=", s(pickupId))
    end

    -- ✅ NEW: ensure icon is hidden and stays hidden
    send_disc_icon_cmd(pickupId, "hide")

    pickedUp = true
end

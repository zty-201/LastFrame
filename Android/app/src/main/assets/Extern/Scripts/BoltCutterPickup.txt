----------------------------------------------------------------
-- BoltCutter Pickup.lua  (MODIFIED from Playroom Key Pickup 2.0)
-- + Adds boltcutter to inventory
-- + Immediately hide entity 157
-- + ✅ Show pickup icon when in range
-- + ✅ After pickup, pickup icon never shows again
-- (Removed: slamming doors, light delay/fade)
----------------------------------------------------------------

local DEBUG = true

local ITEM_ID   = "boltcutter"
local ITEM_NAME = "Bolt Cutter"
local ITEM_QTY  = 1

-- Immediately hide this entity on pickup
local IMMEDIATE_HIDE_ENTITY_ID = 157

-- If your pickup entity is NOT the same entity this script is on,
-- set this to that entity id. Otherwise leave as 0 (auto = EntityID).
local TARGET_ENTITY_ID = 0

-- 3D SFX IDs (change these to your actual slot IDs)
local PICKUP_SFX_ID = "KeyPickup"

-- Internal
local pickedUp = false

local function consume_shared_cmd(cmdTable, id)
    if cmdTable == nil then return false end

    local cmd = cmdTable[id]
    if cmd == nil then return false end

    cmdTable[id] = nil -- consume

    if type(cmd) == "table" then
        local c = cmd.cmd
        return (c == nil) or (c == "toggle") or (c == "pickup") or (c == "take") or (c == "use")
    end

    if cmd == true then return true end
    if cmd == "toggle" or cmd == "pickup" or cmd == "take" or cmd == "use" then return true end

    return true
end

local function hide_entity(id)
    if id == nil or id == 0 then return false end
    local tr = Query.GetTransform(id)
    if tr then
        tr:SetScale(0.0, 0.0, 0.0)
        return true
    end
    return false
end

-- ✅ icon command bus for pickup icon
local function send_pickup_icon_cmd(targetId, cmd)
    SHARED.KEY_ICON_CMD = SHARED.KEY_ICON_CMD or {}
    SHARED.KEY_ICON_CMD[targetId] = { cmd = cmd, by = targetId }
end

local function in_range(targetId)
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[targetId] == true
end

local function play_3d_sfx_from(entityId, slotId)
    if type(Audio_PlaySlot) == "function" and entityId ~= nil and slotId ~= nil then
        Audio_PlaySlot(entityId, slotId)
        return true
    end
    return false
end

function on_create(Self)
    if DEBUG then
        print("[BoltCutterPickup] on_create EntityID:", EntityID)
    end

    -- ensure tables exist
    SHARED.KEY_ICON_CMD = SHARED.KEY_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {} -- you reuse this bus for interact
end

function on_update(Self, dt)
    local pickupId = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID
    dt = dt or 0.016

    ----------------------------------------------------------------
    -- Drive pickup icon while not picked up
    ----------------------------------------------------------------
    if not pickedUp then
        if in_range(pickupId) then
            send_pickup_icon_cmd(pickupId, "show")
        else
            send_pickup_icon_cmd(pickupId, "hide")
        end
    else
        -- once picked up: force hidden forever
        send_pickup_icon_cmd(pickupId, "hide")
        return
    end

    ----------------------------------------------------------------
    -- Wait for InteractionBox "pickup/use" command
    -- (you currently reuse SHARED.DOOR_CMD for pickups)
    ----------------------------------------------------------------
    if not SHARED.DOOR_CMD then return end

    local triggered = consume_shared_cmd(SHARED.DOOR_CMD, pickupId)
    if not triggered then return end

    if DEBUG then
        print("[BoltCutterPickup] Picked up boltcutter")
    end

    ----------------------------------------------------------------
    -- 1) Add boltcutter to inventory
    ----------------------------------------------------------------
    if SHARED.Inventory and SHARED.Inventory.Add then
        SHARED.Inventory.Add(ITEM_ID, ITEM_NAME, ITEM_QTY)
    else
        print("[BoltCutterPickup] WARNING: Inventory.Add not available")
    end

    ----------------------------------------------------------------
    -- 2) Immediately hide entity 157
    ----------------------------------------------------------------
    hide_entity(IMMEDIATE_HIDE_ENTITY_ID)

    ----------------------------------------------------------------
    -- 3) Pickup SFX
    ----------------------------------------------------------------
    play_3d_sfx_from(pickupId, PICKUP_SFX_ID)

    ----------------------------------------------------------------
    -- 4) Hide pickup entity + permanently hide icon
    ----------------------------------------------------------------
    hide_entity(pickupId)
    send_pickup_icon_cmd(pickupId, "hide")

    pickedUp = true
end

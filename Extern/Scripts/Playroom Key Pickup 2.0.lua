----------------------------------------------------------------
-- Playroom Key Pickup 2.0.lua  (MODIFIED)
-- + Slam mainhall doors shut
-- + Immediately hide entity 157
-- + WAIT 3s, then fade hall lights intensity from 0 -> 500
-- + ✅ Show key pickup icon when in range
-- + ✅ After pickup, key icon never shows again
----------------------------------------------------------------

local DEBUG = true

local ITEM_ID   = "playroom_key"
local ITEM_NAME = "Playroom Key"
local ITEM_QTY  = 1

-- After key pickup, slam these doors shut
local MAINHALL_DOOR_IDS = { 509, 26 }

-- Immediately hide this entity on pickup
local IMMEDIATE_HIDE_ENTITY_ID = 157

-- Lights to turn on (fade 0 -> 500 after a delay)
local HALL_LIGHT_IDS = { 486, 488 }
local LIGHT_TARGET_INTENSITY = 500.0
local LIGHT_FADE_SECONDS = 1.0
local LIGHT_DELAY_SECONDS = 3.0

-- If your pickup entity is NOT the same entity this script is on,
-- set this to that entity id. Otherwise leave as 0 (auto = EntityID).
local TARGET_ENTITY_ID = 0

-- 3D SFX IDs (change these to your actual slot IDs)
local KEY_PICKUP_SFX_ID = "KeyPickup"

-- Internal
local pickedUp = false

-- Delay + fade state
local lightDelayActive = false
local lightDelayTime = 0.0
local lightFading = false
local lightTime = 0.0

local function has_any_lights()
    if HALL_LIGHT_IDS == nil then return false end
    for _, lid in ipairs(HALL_LIGHT_IDS) do
        if lid ~= 0 then return true end
    end
    return false
end

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

local function send_light_set_all(value, byId)
    SHARED.LIGHT_CMD = SHARED.LIGHT_CMD or {}
    for _, lid in ipairs(HALL_LIGHT_IDS) do
        if lid ~= 0 then
            SHARED.LIGHT_CMD[lid] = { cmd = "set", by = byId, value = value }
        end
    end
end

-- ✅ NEW: icon command bus for key pickup icon
local function send_key_icon_cmd(keyId, cmd)
    SHARED.KEY_ICON_CMD = SHARED.KEY_ICON_CMD or {}
    SHARED.KEY_ICON_CMD[keyId] = { cmd = cmd, by = keyId }
end

local function key_in_range(keyId)
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[keyId] == true
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
        print("[Key_Shared] on_create EntityID:", EntityID)
    end

    -- ensure tables exist
    SHARED.KEY_ICON_CMD = SHARED.KEY_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
end

function on_update(Self, dt)
    local pickupId = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    ----------------------------------------------------------------
    -- ✅ NEW: drive pickup icon while not picked up
    ----------------------------------------------------------------
    if not pickedUp then
        if key_in_range(pickupId) then
            send_key_icon_cmd(pickupId, "show")
        else
            send_key_icon_cmd(pickupId, "hide")
        end
    else
        -- once picked up: force hidden forever
        send_key_icon_cmd(pickupId, "hide")
    end

    ----------------------------------------------------------------
    -- After pickup: wait, then fade lights
    ----------------------------------------------------------------
    if pickedUp then
        if lightDelayActive and has_any_lights() then
            lightDelayTime = lightDelayTime + dt
            if lightDelayTime >= LIGHT_DELAY_SECONDS then
                lightDelayActive = false
                lightFading = true
                lightTime = 0.0

                send_light_set_all(0.0, pickupId)

                if DEBUG then
                    print("[Key_Shared] Light delay finished -> starting fade")
                end
            end
        end

        if lightFading and has_any_lights() then
            lightTime = lightTime + dt
            local t = lightTime / math.max(LIGHT_FADE_SECONDS, 0.001)
            if t > 1.0 then t = 1.0 end

            send_light_set_all(LIGHT_TARGET_INTENSITY * t, pickupId)

            if t >= 1.0 then
                lightFading = false
                if DEBUG then
                    print("[Key_Shared] Light fade complete")
                end
            end
        end
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
        print("[Key_Shared] Key picked up")
    end

    ----------------------------------------------------------------
    -- 1) Add key to inventory
    ----------------------------------------------------------------
    if SHARED.Inventory and SHARED.Inventory.Add then
        SHARED.Inventory.Add(ITEM_ID, ITEM_NAME, ITEM_QTY)
    end

    ----------------------------------------------------------------
    -- 2) Immediately hide entity 157
    ----------------------------------------------------------------
    hide_entity(IMMEDIATE_HIDE_ENTITY_ID)
    play_3d_sfx_from(pickupId, KEY_PICKUP_SFX_ID)

    ----------------------------------------------------------------
    -- 3) Slam the mainhall doors shut + show lock icons
    ----------------------------------------------------------------
    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
    SHARED.LOCK_ICON_CMD = SHARED.LOCK_ICON_CMD or {}

    for _, doorId in ipairs(MAINHALL_DOOR_IDS) do
        if doorId ~= 0 then
            SHARED.DOOR_CMD[doorId] = { cmd = "close", by = pickupId }
            SHARED.LOCK_ICON_CMD[doorId] = { cmd = "show", by = pickupId }
        end
    end

    ----------------------------------------------------------------
    -- 4) Start LIGHT DELAY
    ----------------------------------------------------------------
    if has_any_lights() then
        lightDelayActive = true
        lightDelayTime = 0.0
        lightFading = false
        lightTime = 0.0
        send_light_set_all(0.0, pickupId)
    end

    ----------------------------------------------------------------
    -- 5) Hide key entity + permanently hide icon
    ----------------------------------------------------------------
    hide_entity(pickupId)
    send_key_icon_cmd(pickupId, "hide")

    pickedUp = true
end

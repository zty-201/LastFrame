----------------------------------------------------------------
-- Playroom Key Pickup 2.0.lua  (MODIFIED: Shared-controlled)
-- + Slam mainhall doors shut
-- + Fade hall lights intensity from 0 -> 500 after pickup
----------------------------------------------------------------

local DEBUG = true

local ITEM_ID   = "playroom_key"
local ITEM_NAME = "Playroom Key"
local ITEM_QTY  = 1

-- After key pickup, slam these doors shut
local MAINHALL_DOOR_IDS = { 509, 26 }

-- After key pickup, turn on these lights (fade 0 -> 500)
local HALL_LIGHT_IDS = { 486, 488 }   -- put all your light EntityIDs here
local LIGHT_TARGET_INTENSITY = 500.0
local LIGHT_FADE_SECONDS = 1.0

-- If your pickup entity is NOT the same entity this script is on,
-- set this to that entity id. Otherwise leave as 0 (auto = EntityID).
local TARGET_ENTITY_ID = 0

-- Internal
local pickedUp = false

-- Light fade internal
local lightFading = false
local lightTime = 0.0

local function s(v)
    if v == nil then return "nil" end
    return tostring(v)
end

local function has_any_lights()
    if HALL_LIGHT_IDS == nil then return false end
    for _, lid in ipairs(HALL_LIGHT_IDS) do
        if lid ~= 0 then return true end
    end
    return false
end

-- Returns true if a command exists for this entity in the shared table
local function consume_shared_cmd(cmdTable, id)
    if cmdTable == nil then return false end

    local cmd = cmdTable[id]
    if cmd == nil then return false end

    cmdTable[id] = nil -- consume

    -- accept multiple payload styles:
    -- true / "toggle" / {cmd="toggle"} / {cmd="pickup"} etc.
    if type(cmd) == "table" then
        local c = cmd.cmd
        return (c == nil) or (c == "toggle") or (c == "pickup") or (c == "take") or (c == "use")
    end

    if cmd == true then return true end
    if cmd == "toggle" or cmd == "pickup" or cmd == "take" or cmd == "use" then return true end

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

local function send_light_set_all(value, byId)
    SHARED.LIGHT_CMD = SHARED.LIGHT_CMD or {}
    for _, lid in ipairs(HALL_LIGHT_IDS) do
        if lid ~= 0 then
            SHARED.LIGHT_CMD[lid] = { cmd = "set", by = byId, value = value }
        end
    end
end

function on_create(Self)
    if DEBUG then
        print("[Key_Shared] on_create EntityID:", EntityID)
        if not SHARED.Inventory or not SHARED.Inventory.Add then
            print("[Key_Shared] WARNING: Inventory not ready (SHARED.Inventory.Add missing)")
        end
    end
end

function on_update(Self, dt)
    local pickupId = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    ----------------------------------------------------------------
    -- If already picked up, continue light fade
    ----------------------------------------------------------------
    if pickedUp then
        if lightFading and has_any_lights() then
            lightTime = lightTime + dt

            local t = 1.0
            if LIGHT_FADE_SECONDS > 0.001 then
                t = lightTime / LIGHT_FADE_SECONDS
            end
            if t > 1.0 then t = 1.0 end

            local intensity = LIGHT_TARGET_INTENSITY * t
            send_light_set_all(intensity, pickupId)

            if t >= 1.0 then
                lightFading = false
                if DEBUG then
                    print("[Key_Shared] Light fade complete -> " .. tostring(LIGHT_TARGET_INTENSITY))
                end
            end
        end
        return
    end

    ----------------------------------------------------------------
    -- Wait for InteractionBox command (same as before)
    ----------------------------------------------------------------
    if SHARED.DOOR_CMD == nil then
        return
    end

    local triggered = consume_shared_cmd(SHARED.DOOR_CMD, pickupId)
    if not triggered then
        return
    end

    if DEBUG then
        print("[Key_Shared] Received shared interact cmd for id=", s(pickupId))
    end

    ----------------------------------------------------------------
    -- 1) Add key to inventory
    ----------------------------------------------------------------
    if SHARED.Inventory and SHARED.Inventory.Add then
        SHARED.Inventory.Add(ITEM_ID, ITEM_NAME, ITEM_QTY)
        if DEBUG then
            print("[Key_Shared] Added:", ITEM_NAME, "x" .. ITEM_QTY)
        end
    else
        print("[Key_Shared] ERROR: Inventory not available")
        return
    end

    ----------------------------------------------------------------
    -- 2) Slam the mainhall doors shut
    ----------------------------------------------------------------
    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
    for _, doorId in ipairs(MAINHALL_DOOR_IDS) do
        if doorId ~= 0 then
            SHARED.DOOR_CMD[doorId] = { cmd = "close", by = pickupId }
            if DEBUG then
                print("[Key_Shared] Slam close sent to mainhall door:", doorId)
            end
        end
    end

    ----------------------------------------------------------------
    -- 3) Start light fade (0 -> 500)
    ----------------------------------------------------------------
    if has_any_lights() then
        lightFading = true
        lightTime = 0.0

        -- Force start at 0 immediately
        send_light_set_all(0.0, pickupId)

        if DEBUG then
            print("[Key_Shared] Light fade started for hall lights")
        end
    else
        if DEBUG then
            print("[Key_Shared] Light fade skipped (HALL_LIGHT_IDS empty)")
        end
    end

    ----------------------------------------------------------------
    -- 4) Hide / shrink the key entity
    ----------------------------------------------------------------
    local okHide = hide_entity(pickupId)
    if DEBUG then
        print("[Key_Shared] Hide result=", tostring(okHide), " id=", s(pickupId))
    end

    pickedUp = true
end

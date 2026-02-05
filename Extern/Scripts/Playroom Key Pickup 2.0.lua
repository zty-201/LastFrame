----------------------------------------------------------------
-- Playroom Key Pickup 2.0.lua  (MODIFIED: Shared-controlled)
-- Door2.0 works -> so we listen to the SAME shared command table
-- When our EntityID is the current target and E is pressed,
-- PlayerInteract/InteractionBox should be setting:
--   SHARED.DOOR_CMD[EntityID] = something
-- We consume it and add the key to inventory.
----------------------------------------------------------------

local DEBUG = true

local ITEM_ID   = "playroom_key"
local ITEM_NAME = "Playroom Key"
local ITEM_QTY  = 1

-- After key pickup, slam these doors shut
local MAINHALL_DOOR_IDS = { 509, 26 }  -- <-- replace with the 2 door EntityIDs

-- If your pickup entity is NOT the same entity this script is on,
-- set this to that entity id. Otherwise leave as 0 (auto = EntityID).
local TARGET_ENTITY_ID = 0

-- Internal: prevent double pickup
local pickedUp = false

local function s(v)
    if v == nil then return "nil" end
    return tostring(v)
end

-- Returns true if a command exists for this entity in the shared table
local function consume_shared_cmd(cmdTable, id)
    if cmdTable == nil then return false end

    local cmd = cmdTable[id]
    if cmd == nil then return false end

    -- consume so it only triggers once
    cmdTable[id] = nil

    -- accept multiple payload styles:
    -- true / "toggle" / {cmd="toggle"} / {cmd="pickup"} etc.
    if type(cmd) == "table" then
        -- If you *know* your door cmd is always "toggle", you can just return true here.
        -- But leaving flexible is safer.
        local c = cmd.cmd
        return (c == nil) or (c == "toggle") or (c == "pickup") or (c == "take") or (c == "use")
    end

    if cmd == true then return true end
    if cmd == "toggle" or cmd == "pickup" or cmd == "take" or cmd == "use" then return true end

    -- unknown payload but it was targeted at us -> treat as interact
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

function on_create(Self)
    if DEBUG then
        print("[Key_Shared] on_create EntityID:", EntityID)
        if not SHARED.Inventory or not SHARED.Inventory.Add then
            print("[Key_Shared] WARNING: Inventory not ready (SHARED.Inventory.Add missing)")
        end
    end
end

function on_update(Self, dt)
    if pickedUp then return end

    local pickupId = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    -- 🔑 Door2.0-style: listen to the SAME shared command table
    -- If Door2.0 works, your player/interaction system is setting SHARED.DOOR_CMD[targetId]
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

    -- 1️⃣ Add key to inventory
    if SHARED.Inventory and SHARED.Inventory.Add then
        SHARED.Inventory.Add(ITEM_ID, ITEM_NAME, ITEM_QTY)
        if DEBUG then
            print("[Key_Shared] Added:", ITEM_NAME, "x" .. ITEM_QTY)
        end

        -- ✅ NEW: slam the 2 mainhall doors shut
        SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
        for _, doorId in ipairs(MAINHALL_DOOR_IDS) do
            if doorId ~= 0 then
                SHARED.DOOR_CMD[doorId] = { cmd = "close", by = pickupId }
                if DEBUG then
                    print("[Key_Shared] Slam close sent to mainhall door:", doorId)
                end
            end
        end

    else
        print("[Key_Shared] ERROR: Inventory not available")
        return
    end


    -- 2️⃣ Hide / shrink the key entity
    local okHide = hide_entity(pickupId)
    if DEBUG then
        print("[Key_Shared] Hide result=", tostring(okHide), " id=", s(pickupId))
    end

    pickedUp = true
end

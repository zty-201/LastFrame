----------------------------------------------------------------
-- Test_AddMusicDisc_Shared.lua
-- InteractionZone driven:
-- When InteractionBox sends SHARED.DOOR_CMD[EntityID], pick up disc.
----------------------------------------------------------------

local DEBUG = true

local ITEM_ID   = "music_disc"
local ITEM_NAME = "Music Disc"
local ITEM_QTY  = 1

-- If the interactable ID is not the same entity this script is on,
-- set this (otherwise leave as 0).
local TARGET_ENTITY_ID = 0

local pickedUp = false

local function s(v)
    if v == nil then return "nil" end
    return tostring(v)
end

local function consume_shared_cmd(cmdTable, id)
    if cmdTable == nil then return false end

    local cmd = cmdTable[id]
    if cmd == nil then return false end

    -- consume one-shot
    cmdTable[id] = nil

    -- accept the common payloads your InteractionBox sends
    if type(cmd) == "table" then
        local c = cmd.cmd
        return (c == nil) or (c == "toggle") or (c == "pickup") or (c == "use")
    end

    if cmd == true then return true end
    if cmd == "toggle" or cmd == "pickup" or cmd == "use" then return true end

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
        print("[Test_AddMusicDisc_Shared] on_create EntityID:", EntityID)
        if not SHARED.Inventory or not SHARED.Inventory.Add then
            print("[Test_AddMusicDisc_Shared] WARNING: Inventory not ready (SHARED.Inventory.Add missing)")
        end
    end
end

function on_update(Self, dt)
    if pickedUp then return end

    local pickupId = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    if SHARED.DOOR_CMD == nil then
        return
    end

    -- ✅ Driven by InteractionBox target + E
    local triggered = consume_shared_cmd(SHARED.DOOR_CMD, pickupId)
    if not triggered then
        return
    end

    if DEBUG then
        print("[Test_AddMusicDisc_Shared] Received interact cmd for id=", s(pickupId))
    end

    -- 1️⃣ Add item
    if SHARED.Inventory and SHARED.Inventory.Add then
        SHARED.Inventory.Add(ITEM_ID, ITEM_NAME, ITEM_QTY)
        if DEBUG then
            print("[Test_AddMusicDisc_Shared] Added:", ITEM_NAME, "x" .. ITEM_QTY)
        end
    else
        print("[Test_AddMusicDisc_Shared] ERROR: Inventory not available")
        return
    end

    -- 2️⃣ Hide / shrink
    local ok = hide_entity(pickupId)
    if DEBUG then
        print("[Test_AddMusicDisc_Shared] Hide result=", tostring(ok), " id=", s(pickupId))
    end

    pickedUp = true
end

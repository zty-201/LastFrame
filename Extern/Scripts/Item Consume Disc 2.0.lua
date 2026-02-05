----------------------------------------------------------------
-- Test_ConsumeMusicDisc_Shared.lua
-- Uses InteractionBox (shared command table) instead of direct Q key.
-- When InteractionBox targets THIS entity and player presses E:
--   - consume 1 music_disc
--   - trigger StateController CP1
----------------------------------------------------------------

local DEBUG = true

local ITEM_ID  = "music_disc"
local ITEM_QTY = 1

-- If InteractionBox targets a DIFFERENT entity (e.g., a trigger),
-- set this to that entity id. Otherwise leave 0 (auto uses EntityID).
local TARGET_ENTITY_ID = 0

local function s(v) return (v == nil) and "nil" or tostring(v) end

local function consume_shared_cmd(cmdTable, id)
    if cmdTable == nil then return nil end
    local cmd = cmdTable[id]
    if cmd == nil then return nil end
    cmdTable[id] = nil -- consume
    return cmd
end

local function cmd_is_interact(cmd)
    -- Accept the same payloads your system uses
    if cmd == nil then return false end
    if type(cmd) == "table" then
        local c = cmd.cmd
        return (c == nil) or (c == "toggle") or (c == "use") or (c == "interact")
    end
    if cmd == true then return true end
    if cmd == "toggle" or cmd == "use" or cmd == "interact" then return true end
    return true
end

function on_create(Self)
    if DEBUG then
        print("[Test_ConsumeDisc_Shared] on_create EntityID:", EntityID)
        if not SHARED.Inventory or not SHARED.Inventory.Remove then
            print("[Test_ConsumeDisc_Shared] WARNING: Inventory.Remove not found")
        end
    end
end

function on_update(Self, dt)
    local targetId = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    if not SHARED.DOOR_CMD then return end

    -- Triggered only when InteractionBox sends a command to this entity
    local cmd = consume_shared_cmd(SHARED.DOOR_CMD, targetId)
    if not cmd_is_interact(cmd) then
        return
    end

    if DEBUG then
        local by = (type(cmd) == "table") and cmd.by or nil
        print(string.format(
            "[Test_ConsumeDisc_Shared] Interact received for id=%s by=%s",
            s(targetId), s(by)
        ))
    end

    -- Inventory checks
    if not SHARED.Inventory or not SHARED.Inventory.Remove then
        print("[Test_ConsumeDisc_Shared] ERROR: Inventory not available")
        return
    end

    local count = 0
    if SHARED.Inventory.Count then
        count = SHARED.Inventory.Count(ITEM_ID) or 0
    end

    if count <= 0 then
        if DEBUG then
            print("[Test_ConsumeDisc_Shared] No Music Disc to consume")
        end
        return
    end

    local ok = SHARED.Inventory.Remove(ITEM_ID, ITEM_QTY)
    if not ok then
        print("[Test_ConsumeDisc_Shared] ERROR: Remove failed")
        return
    end

    if DEBUG then
        print("[Test_ConsumeDisc_Shared] Consumed:", ITEM_ID, "x" .. ITEM_QTY)
        if SHARED.Inventory.Print then
            SHARED.Inventory.Print()
        end
    end

    -- Trigger CP1
    if SHARED.StateController and SHARED.StateController.TriggerCP1 then
        SHARED.StateController.TriggerCP1()
    else
        print("[Test_ConsumeDisc_Shared] WARNING: StateController.TriggerCP1 not found")
    end
end

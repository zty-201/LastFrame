----------------------------------------------------------------
-- Test_ConsumeMusicDisc_Shared.lua
-- + If player is in range:
--     - show ICON1 if disc exists in inventory
--     - else show ICON2
-- + Hide both when out of range
-- + ✅ After consuming FIRST disc successfully, hide both icons forever
----------------------------------------------------------------

local DEBUG = true

local ITEM_ID  = "music_disc"
local ITEM_QTY = 1

local TARGET_ENTITY_ID = 0

-- ✅ NEW: once we consume one disc, never show any icons again
local iconsDisabledForever = false

local function s(v) return (v == nil) and "nil" or tostring(v) end

local function consume_shared_cmd(cmdTable, id)
    if cmdTable == nil then return nil end
    local cmd = cmdTable[id]
    if cmd == nil then return nil end
    cmdTable[id] = nil
    return cmd
end

local function cmd_is_interact(cmd)
    if cmd == nil then return false end
    if type(cmd) == "table" then
        local c = cmd.cmd
        return (c == nil) or (c == "toggle") or (c == "use") or (c == "interact")
    end
    if cmd == true then return true end
    if cmd == "toggle" or cmd == "use" or cmd == "interact" then return true end
    return true
end

-- ===== ICON BUS HELPERS =====
local function send_have_icon_cmd(id, cmd)
    SHARED.DISC_USE_HAVE_ICON_CMD = SHARED.DISC_USE_HAVE_ICON_CMD or {}
    SHARED.DISC_USE_HAVE_ICON_CMD[id] = { cmd = cmd, by = id }
end

local function send_nodisc_icon_cmd(id, cmd)
    SHARED.DISC_USE_NODISC_ICON_CMD = SHARED.DISC_USE_NODISC_ICON_CMD or {}
    SHARED.DISC_USE_NODISC_ICON_CMD[id] = { cmd = cmd, by = id }
end

local function in_range(id)
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[id] == true
end

local function disc_count()
    if not SHARED or not SHARED.Inventory or not SHARED.Inventory.Count then
        return 0
    end
    return SHARED.Inventory.Count(ITEM_ID) or 0
end

local function hide_icons_forever(targetId)
    send_have_icon_cmd(targetId, "hide")
    send_nodisc_icon_cmd(targetId, "hide")
end

local function update_icons(targetId)
    -- ✅ NEW: once disabled, always hide
    if iconsDisabledForever then
        hide_icons_forever(targetId)
        return
    end

    if not in_range(targetId) then
        hide_icons_forever(targetId)
        return
    end

    if disc_count() > 0 then
        send_have_icon_cmd(targetId, "show")
        send_nodisc_icon_cmd(targetId, "hide")
    else
        send_have_icon_cmd(targetId, "hide")
        send_nodisc_icon_cmd(targetId, "show")
    end
end

function on_create(Self)
    if DEBUG then
        print("[Test_ConsumeDisc_Shared] on_create EntityID:", EntityID)
    end

    SHARED.DISC_USE_HAVE_ICON_CMD = SHARED.DISC_USE_HAVE_ICON_CMD or {}
    SHARED.DISC_USE_NODISC_ICON_CMD = SHARED.DISC_USE_NODISC_ICON_CMD or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
end

function on_update(Self, dt)
    local targetId = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    -- always drive icons
    update_icons(targetId)

    if iconsDisabledForever then
        return
    end

    if not SHARED.DOOR_CMD then return end

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

    if not SHARED.Inventory or not SHARED.Inventory.Remove then
        print("[Test_ConsumeDisc_Shared] ERROR: Inventory not available")
        return
    end

    if disc_count() <= 0 then
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
    end

    -- Trigger CP1
    if SHARED.StateController and SHARED.StateController.TriggerCP1 then
        SHARED.StateController.TriggerCP1()
    else
        print("[Test_ConsumeDisc_Shared] WARNING: StateController.TriggerCP1 not found")
    end

    -- ✅ NEW: after first successful consume, disable icons forever
    iconsDisabledForever = true
    hide_icons_forever(targetId)
end

----------------------------------------------------------------
-- Test_ConsumeMusicDisc_Shared.lua
-- + If player is in range:
--     - show ICON1 if disc exists in inventory
--     - else show ICON2
-- + Hide both when out of range
-- + After consuming FIRST disc successfully, hide both icons forever
-- + Play DiscInsert immediately
-- + Delay disc visual placement
-- + Delay music trigger
-- + Delay disc spinning separately
----------------------------------------------------------------

local DEBUG = true

local ITEM_ID  = "music_disc"
local ITEM_QTY = 1

local TARGET_ENTITY_ID = 0

-- ID of the nearby notes interactable — forward unused E presses to it
local NOTES_FORWARD_ID = 822

-- Once we consume one disc, never show any icons again
local iconsDisabledForever = false

-- Audio / timing
local DISC_INSERT_SFX_ID = "DiscInsert"
local DISC_INSERT_DELAY_SECONDS = 1.0

-- Disc visual timing
local DISC_VISUAL_DELAY = 0.4
local discVisualPending = false
local discVisualTimer = 0.0

-- Disc spin timing
local DISC_SPIN_DELAY = 1.7
local discSpinPending = false
local discSpinTimer = 0.0

-- Music trigger timing
local pendingTrigger = false
local pendingTimer = 0.0

local function s(v)
    return (v == nil) and "nil" or tostring(v)
end

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

-- Forward an E press to the notes interactable if the player is in its zone
local function try_forward_to_notes(cmd)
    if not (SHARED and SHARED.INTERACT_IN_ZONE and SHARED.INTERACT_IN_ZONE[NOTES_FORWARD_ID]) then
        return
    end
    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}
    SHARED.DOOR_CMD[NOTES_FORWARD_ID] = cmd
    if DEBUG then
        print("[Test_ConsumeDisc_Shared] Forwarding command to notes id=" .. tostring(NOTES_FORWARD_ID))
    end
end

local function update_icons(targetId)
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

    SHARED.MUSICBOX_DISC_PLACED = false
    SHARED.MUSICBOX_DISC_SPIN = false

    discVisualPending = false
    discVisualTimer = 0.0

    discSpinPending = false
    discSpinTimer = 0.0

    pendingTrigger = false
    pendingTimer = 0.0
end

function on_update(Self, dt)
    dt = dt or 0.016
    local targetId = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    -- Always drive icons
    update_icons(targetId)

    ------------------------------------------------
    -- Disc visual delay
    ------------------------------------------------
    if discVisualPending then
        discVisualTimer = discVisualTimer + dt

        if discVisualTimer >= DISC_VISUAL_DELAY then
            discVisualPending = false
            discVisualTimer = 0.0

            SHARED.MUSICBOX_DISC_PLACED = true

            if DEBUG then
                print("[Test_ConsumeDisc_Shared] Disc visual placed after delay")
            end
        end
    end

    ------------------------------------------------
    -- Disc spin delay
    ------------------------------------------------
    if discSpinPending then
        discSpinTimer = discSpinTimer + dt

        if discSpinTimer >= DISC_SPIN_DELAY then
            discSpinPending = false
            discSpinTimer = 0.0

            SHARED.MUSICBOX_DISC_SPIN = true

            if DEBUG then
                print("[Test_ConsumeDisc_Shared] Disc spin enabled")
            end
        end
    end

    ------------------------------------------------
    -- Delayed music trigger
    ------------------------------------------------
    if pendingTrigger then
        pendingTimer = pendingTimer + dt

        if pendingTimer >= DISC_INSERT_DELAY_SECONDS then
            pendingTrigger = false
            pendingTimer = 0.0

            if SHARED.StateController and SHARED.StateController.TriggerCP1 then
                SHARED.StateController.TriggerCP1()
                if DEBUG then
                    print("[Test_ConsumeDisc_Shared] TriggerCP1 fired after DiscInsert delay")
                end
            else
                print("[Test_ConsumeDisc_Shared] WARNING: StateController.TriggerCP1 not found")
            end
        end
        return
    end

    if not SHARED.DOOR_CMD then return end

    local cmd = consume_shared_cmd(SHARED.DOOR_CMD, targetId)
    if not cmd_is_interact(cmd) then
        return
    end

    -- Disc already consumed — pass E through to the notes if player is in that zone
    if iconsDisabledForever then
        try_forward_to_notes(cmd)
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
            print("[Test_ConsumeDisc_Shared] No Music Disc to consume — forwarding to notes")
        end
        try_forward_to_notes(cmd)
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

    -- Play DiscInsert immediately
    if type(Audio_PlaySlot) == "function" then
        Audio_PlaySlot(EntityID, DISC_INSERT_SFX_ID)
        if DEBUG then
            print("[Test_ConsumeDisc_Shared] Played DiscInsert")
        end
    end

    -- Start delayed disc visual placement
    discVisualPending = true
    discVisualTimer = 0.0

    -- Start delayed disc spinning
    discSpinPending = true
    discSpinTimer = 0.0

    -- Start delayed music trigger
    pendingTrigger = true
    pendingTimer = 0.0

    -- After first successful consume, disable icons forever
    -- and remove from the interaction zone so nearby interactables (e.g. playroom notes)
    -- can receive E-press commands without the disc entity stealing them
    iconsDisabledForever = true
    hide_icons_forever(targetId)
    if SHARED.INTERACT_IN_ZONE then
        SHARED.INTERACT_IN_ZONE[targetId] = false
    end
end
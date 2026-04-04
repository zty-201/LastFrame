----------------------------------------------------------------
-- ToolboxOpenMechanic.lua
-- When in InteractionBox range and E is pressed:
--   - If LockScreen is closed -> open it
--   - If LockScreen is open   -> close it
--
-- Drives a UI icon via:
--    SHARED.TRIGGER_ICON_CMD[id] = { cmd = "show"/"hide", by = ... }
----------------------------------------------------------------

local DEBUG = true

-- If the interactable id is NOT the entity this script is on,
-- set this. Otherwise leave 0 (uses EntityID).
local TARGET_ENTITY_ID = 0

local MENU_NAME       = "LockScreen"
local UI_TABLE        = "LOCKSCREEN_OPEN"

local VIEW_LOCK_SFX_ID = "ViewLock"
local UNLOCK_SFX_ID = "Unlock"
local DISCOVERY_SFX_ID = "Discovery"

local VOICEOVER_SFX_ID = "VoiceOver"
local VOICEOVER_DELAY = 0.6

local hasPlayedFirstPadlockVO = false
local pendingPadlockVO = false
local padlockVOTimer = 0.0

local isOpen = false

-- ---------------------------------------
-- Helpers
-- ---------------------------------------
local function consume_shared_cmd(cmdTable, id)
    if cmdTable == nil then return false end

    local cmd = cmdTable[id]
    if cmd == nil then return false end
    cmdTable[id] = nil -- consume

    if type(cmd) == "table" then
        local c = cmd.cmd
        return (c == nil) or (c == "toggle") or (c == "pickup")
            or (c == "take") or (c == "use") or (c == "open")
    end

    if cmd == true then return true end
    if cmd == "toggle" or cmd == "pickup" or cmd == "take"
        or cmd == "use" or cmd == "open" then
        return true
    end

    return true
end

local function in_range(id)
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[id] == true
end

local function send_trigger_icon_cmd(targetId, cmd)
    SHARED.TRIGGER_ICON_CMD = SHARED.TRIGGER_ICON_CMD or {}
    SHARED.TRIGGER_ICON_CMD[targetId] = { cmd = cmd, by = targetId }
end

-- ---------------------------------------
-- Lifecycle
-- ---------------------------------------
function on_create(self)
    if DEBUG then
        print("[ToolboxOpenMechanic] on_create EntityID=" .. tostring(EntityID))
    end

    SHARED.DOOR_CMD                  = SHARED.DOOR_CMD         or {}
    SHARED.INTERACT_IN_ZONE          = SHARED.INTERACT_IN_ZONE or {}
    SHARED.TRIGGER_ICON_CMD          = SHARED.TRIGGER_ICON_CMD or {}
    SHARED[UI_TABLE]                 = false
    SHARED.LOCK_DISABLED             = false
    SHARED.LOCK_SUCCESS              = false
    SHARED.TOOLBOX_UNLOCK_AUDIO_PLAYED = false
end

function on_update(self, dt)
    dt = dt or 0.016

    local id = (TARGET_ENTITY_ID ~= 0) and TARGET_ENTITY_ID or EntityID

    -- Sync isOpen from SHARED in case it was closed externally (e.g. LockManager)
    if SHARED and SHARED[UI_TABLE] == false and isOpen then
        isOpen = false
        if DEBUG then print("[ToolboxOpenMechanic] isOpen synced to false from SHARED") end
    end

    -- Auto-close if player walked out of the zone while LockScreen was open
    if isOpen and not in_range(id) then
        if UI_CloseAllMenus ~= nil then UI_CloseAllMenus() end
        SHARED[UI_TABLE] = false
        isOpen = false
        if DEBUG then print("[ToolboxOpenMechanic] Auto-closed: player left zone") end
    end

    ----------------------------------------------------------------
    -- Delayed first-time padlock VoiceOver
    ----------------------------------------------------------------
    if pendingPadlockVO then
        padlockVOTimer = padlockVOTimer + dt

        if padlockVOTimer >= VOICEOVER_DELAY then
            pendingPadlockVO = false
            padlockVOTimer = 0.0
            hasPlayedFirstPadlockVO = true

            if type(Audio_PlaySlot) == "function" then
                Audio_PlaySlot(id, VOICEOVER_SFX_ID)
            end

            ------------------------------------------------
            -- Trigger subtitle
            ------------------------------------------------
            SHARED.SUBTITLE_CMD = SHARED.SUBTITLE_CMD or {}
            SHARED.SUBTITLE_CMD["TOOLBOX_PADLOCK_REACTION"] = {
                trigger = true,
                by = id
            }

            if DEBUG then
                print("[ToolboxOpenMechanic] VoiceOver + subtitle played")
            end
        end
    end

    ----------------------------------------------------------------
    -- If lock has just been disabled (success), play unlock audio
    ----------------------------------------------------------------
    if SHARED and SHARED.LOCK_SUCCESS and not SHARED.TOOLBOX_UNLOCK_AUDIO_PLAYED then
        SHARED.TOOLBOX_UNLOCK_AUDIO_PLAYED = true

        if type(Audio_PlaySlot) == "function" then
            Audio_PlaySlot(id, UNLOCK_SFX_ID)
            Audio_PlaySlot(id, DISCOVERY_SFX_ID)
        end

        if DEBUG then
            print("[ToolboxOpenMechanic] Unlock audio triggered")
        end
    end

    ----------------------------------------------------------------
    -- Drive icon (hidden while LockScreen is open, else show in range)
    ----------------------------------------------------------------
    if isOpen or not in_range(id) then
        send_trigger_icon_cmd(id, "hide")
    else
        send_trigger_icon_cmd(id, "show")
    end

    ----------------------------------------------------------------
    -- Must be in interaction range
    ----------------------------------------------------------------
    if not in_range(id) then return end

    ----------------------------------------------------------------
    -- Must receive E-press command
    ----------------------------------------------------------------
    if not consume_shared_cmd(SHARED.DOOR_CMD, id) then return end

    ----------------------------------------------------------------
    -- Toggle LockScreen open/close
    ----------------------------------------------------------------
    if not isOpen then
        -- Opening (view lock)
        if SHARED.LOCK_DISABLED then return end

        if type(Audio_PlaySlot) == "function" then
            Audio_PlaySlot(id, VIEW_LOCK_SFX_ID)
        end

        if UI_DisplayMenu ~= nil then
            UI_DisplayMenu(MENU_NAME)
        else
            print("[ToolboxOpenMechanic] ERROR: UI_DisplayMenu binding missing")
        end

        SHARED[UI_TABLE] = true
        isOpen = true

        -- First-time delayed VO
        if not hasPlayedFirstPadlockVO and not pendingPadlockVO then
            pendingPadlockVO = true
            padlockVOTimer = 0.0

            if DEBUG then
                print("[ToolboxOpenMechanic] First padlock open -> scheduling VoiceOver")
            end
        end

        if DEBUG then print("[ToolboxOpenMechanic] Opened: " .. MENU_NAME) end
    else
        -- Closing
        if UI_CloseAllMenus ~= nil then
            UI_CloseAllMenus()
        end

        SHARED[UI_TABLE] = false
        isOpen = false

        if DEBUG then print("[ToolboxOpenMechanic] Closed: " .. MENU_NAME) end
    end
end

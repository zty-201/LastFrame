----------------------------------------------------------------
-- UI_SubtitleManager.lua
-- Attach to an empty entity.
--
-- Shows subtitle UI entities in response to SHARED.SUBTITLE_CMD keys.
-- Each subtitle stays visible for its configured duration, then hides.
--
-- IMPORTANT:
-- Subtitle UI entities live inside the "SubtitlesMenu" overlay, so
-- the menu itself must be displayed for the subtitle to render.
----------------------------------------------------------------

local DEBUG = true

local SUBTITLE_CMD_TABLE = "SUBTITLE_CMD"
local SUBTITLE_MENU_NAME = "SubtitlesMenu"

----------------------------------------------------------------
-- Subtitle definitions
-- key -> { entity = <ui entity id>, duration = <seconds> }
----------------------------------------------------------------
local SUBTITLES = {
    PLAYROOM_DOOR_LOCKED = { entity = 1041, duration = 2.0 },
    MAIN_DOOR_REACTION = { entity = 1042, duration = 1.0 },
    MUSICBOX_NO_DISC = { entity = 1043, duration = 1.7 },
    CP1_DETECTIVE_REACTION = { entity = 1044, duration = 0.7 },
    CP2_WOAH = { entity = 1045, duration = 0.5 },
    CANTEEN_DOOR_LOCKED_SEQUENCE = { entity = 1046, duration = 3.0 },
    SF_GIRL_REACTION = { entity = 1047, duration = 1.0 },
    LOBBY_CHAIR_DRAG_REACTION = { entity = 1048, duration = 1.0 },
    LEVEL2_GATE_LOCKED = { entity = 1049, duration = 1.5 },
    BEDROOM_ENTRY_REACTION = { entity = 1050, duration = 0.8 },
    BEDROOM_RETURN_REACTION = { entity = 1051, duration = 1.5 },
    TOOLBOX_PADLOCK_REACTION = { entity = 1052, duration = 3.0 },
    GRAVES_REVEAL_REACTION = { entity = 1054, duration = 1.5 },
}

----------------------------------------------------------------
-- Runtime state
----------------------------------------------------------------
local activeKey = nil
local activeEntityId = nil
local activeTimer = 0.0

----------------------------------------------------------------
-- Helpers
----------------------------------------------------------------
local function get_ui(entityId)
    return GetEntityUI(entityId)
end

local function hide_subtitle(entityId)
    if entityId == nil or entityId == 0 then return end

    local ui = get_ui(entityId)
    if ui then
        ui:SetVisible(false)
    end
end

local function show_subtitle(entityId)
    if entityId == nil or entityId == 0 then return end

    local ui = get_ui(entityId)
    if ui then
        ui:SetVisible(true)
    end
end

local function hide_all_subtitles()
    for _, data in pairs(SUBTITLES) do
        if data.entity ~= nil and data.entity ~= 0 then
            hide_subtitle(data.entity)
        end
    end
end

local function open_subtitle_menu()
    if UI_DisplayMenu ~= nil then
        UI_DisplayMenu(SUBTITLE_MENU_NAME)
    end
end

local function close_subtitle_menu()
    if UI_CloseMenu ~= nil then
        UI_CloseMenu(SUBTITLE_MENU_NAME)
    end
end

local function start_subtitle(key)
    local data = SUBTITLES[key]
    if data == nil then
        if DEBUG then
            print("[SubtitleManager] Unknown subtitle key: " .. tostring(key))
        end
        return
    end

    ----------------------------------------------------------------
    -- IMPORTANT:
    -- Open the menu first, because UI_DisplayMenu may force all
    -- overlay children visible again. Then explicitly hide all
    -- subtitles, then show only the one we want.
    ----------------------------------------------------------------
    open_subtitle_menu()
    hide_all_subtitles()

    activeKey = key
    activeEntityId = data.entity
    activeTimer = data.duration or 2.0

    show_subtitle(activeEntityId)

    if DEBUG then
        print(string.format(
            "[SubtitleManager] SHOW key=%s entity=%s duration=%.2f menu=%s",
            tostring(key),
            tostring(activeEntityId),
            activeTimer,
            SUBTITLE_MENU_NAME
        ))
    end
end

local function stop_active_subtitle()
    if activeEntityId ~= nil then
        hide_subtitle(activeEntityId)
    end

    close_subtitle_menu()

    if DEBUG and activeKey ~= nil then
        print(string.format(
            "[SubtitleManager] HIDE key=%s entity=%s menu=%s",
            tostring(activeKey),
            tostring(activeEntityId),
            SUBTITLE_MENU_NAME
        ))
    end

    activeKey = nil
    activeEntityId = nil
    activeTimer = 0.0
end

----------------------------------------------------------------
-- Lifecycle
----------------------------------------------------------------
function on_create(self)
    SHARED = SHARED or {}
    SHARED[SUBTITLE_CMD_TABLE] = SHARED[SUBTITLE_CMD_TABLE] or {}

    activeKey = nil
    activeEntityId = nil
    activeTimer = 0.0

    hide_all_subtitles()
    close_subtitle_menu()

    if DEBUG then
        print("[SubtitleManager] Ready")
    end
end

function on_update(self, dt)
    dt = dt or 0.016

    SHARED = SHARED or {}
    SHARED[SUBTITLE_CMD_TABLE] = SHARED[SUBTITLE_CMD_TABLE] or {}

    ----------------------------------------------------------------
    -- Consume incoming subtitle requests
    ----------------------------------------------------------------
    for key, cmdObj in pairs(SHARED[SUBTITLE_CMD_TABLE]) do
        local shouldTrigger = false

        if cmdObj == true then
            shouldTrigger = true
        elseif type(cmdObj) == "table" and cmdObj.trigger == true then
            shouldTrigger = true
        end

        if shouldTrigger then
            SHARED[SUBTITLE_CMD_TABLE][key] = nil
            start_subtitle(key)
            break
        end
    end

    ----------------------------------------------------------------
    -- While a subtitle is active, enforce that ONLY the active one
    -- is visible. This protects against the overlay/menu system
    -- turning other subtitle entities visible.
    ----------------------------------------------------------------
    if activeEntityId ~= nil then
        for _, data in pairs(SUBTITLES) do
            if data.entity ~= nil and data.entity ~= 0 then
                if data.entity == activeEntityId then
                    show_subtitle(data.entity)
                else
                    hide_subtitle(data.entity)
                end
            end
        end

        activeTimer = activeTimer - dt
        if activeTimer <= 0.0 then
            stop_active_subtitle()
        end
    end
end

function on_destroy(self)
    hide_all_subtitles()
    close_subtitle_menu()

    activeKey = nil
    activeEntityId = nil
    activeTimer = 0.0
end
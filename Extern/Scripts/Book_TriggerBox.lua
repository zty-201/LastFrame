----------------------------------------------------------------
-- Book_TriggerBox.lua
-- Attach to the Book interactable trigger
----------------------------------------------------------------

print("[Book_TriggerBox] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

local BOOK_ID   = 431
local MENU_NAME = "BookInterface"

local DOOR_CMD_TABLE = "DOOR_CMD"
local UI_TABLE       = "BOOK_UI_OPEN"

local PAGE_FLIP_SFX_ID = "PageFlip"
local DISCOVERY_SFX_ID = "Discovery"

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end
    return false
end

local function consume_toggle(id)
    if SHARED == nil then return false end
    local t = SHARED[DOOR_CMD_TABLE]
    if t == nil then return false end
    if t[id] == nil then return false end
    t[id] = nil
    return true
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[DOOR_CMD_TABLE] = SHARED[DOOR_CMD_TABLE] or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}
    SHARED[UI_TABLE][BOOK_ID] = false

    if DEBUG then
        print("[Book_TriggerBox] created. BOOK_ID=", BOOK_ID, " MENU=", MENU_NAME)
    end
end

function on_update(self, dt)
    -- Auto-close if player walked out of the zone while the UI was open
    if SHARED[UI_TABLE][BOOK_ID] == true
       and not (SHARED.INTERACT_IN_ZONE and SHARED.INTERACT_IN_ZONE[BOOK_ID] == true) then
        if UI_CloseMenu ~= nil then UI_CloseMenu(MENU_NAME) end
        SHARED[UI_TABLE][BOOK_ID] = false
        if DEBUG then print("[Book_TriggerBox] Auto-closed: player left zone") end
    end

    if not consume_toggle(BOOK_ID) then
        return
    end

    if DEBUG then
        print("[Book_TriggerBox] Interact received for", BOOK_ID)
    end

    -- Ensure menu is shown
    if UI_DisplayMenu ~= nil then
        UI_DisplayMenu(MENU_NAME)
    else
        print("[Book_TriggerBox] ERROR: UI_DisplayMenu binding missing")
    end

    -- Toggle UI flag
    local wasOpen = (SHARED[UI_TABLE][BOOK_ID] == true)
    local isNowOpen = not wasOpen
    SHARED[UI_TABLE][BOOK_ID] = isNowOpen

    -- Play page flip sound when opening
    if isNowOpen then
        play_3d_sfx(PAGE_FLIP_SFX_ID)
        play_3d_sfx(DISCOVERY_SFX_ID)

        if DEBUG then
            print("[Book_TriggerBox] Page flipped")
        end
    end

    if DEBUG then
        print("[Book_TriggerBox] UI open =", tostring(SHARED[UI_TABLE][BOOK_ID]))
    end
end
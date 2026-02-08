----------------------------------------------------------------
-- Book_TriggerBox.lua
-- Attach to the Book interactable trigger (BOOK_ID)
--
-- InteractionBox sends:
--   E -> sent toggle to door BOOK_ID
-- so we consume SHARED.DOOR_CMD[BOOK_ID] and toggle our UI open flag.
--
-- Ensures menu is displayed via UI_DisplayMenu("BookInterface")
----------------------------------------------------------------

print("[Book_TriggerBox] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

local BOOK_ID    = 431              -- <<<<<< CHANGE THIS
local MENU_NAME  = "BookInterface"  -- <<<<<< ensure your menu is named exactly this

local DOOR_CMD_TABLE = "DOOR_CMD"
local UI_TABLE       = "BOOK_UI_OPEN"

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
    if not consume_toggle(BOOK_ID) then
        return
    end

    if DEBUG then
        print("[Book_TriggerBox] Interact received for", BOOK_ID)
    end

    -- Ensure menu is shown/loaded
    if UI_DisplayMenu ~= nil then
        UI_DisplayMenu(MENU_NAME)
    else
        print("[Book_TriggerBox] ERROR: UI_DisplayMenu binding missing")
    end

    -- Toggle open flag
    SHARED[UI_TABLE][BOOK_ID] = not (SHARED[UI_TABLE][BOOK_ID] == true)

    if DEBUG then
        print("[Book_TriggerBox] UI open =", tostring(SHARED[UI_TABLE][BOOK_ID]))
    end
end

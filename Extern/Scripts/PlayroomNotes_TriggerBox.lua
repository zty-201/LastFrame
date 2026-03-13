----------------------------------------------------------------
-- PlayroomNotes_TriggerBox.lua
-- Attach to the Playroom Notes interactable trigger
--
-- When player presses E on this note:
--   - open the notes UI/menu
--   - toggle its open state
----------------------------------------------------------------

print("[PlayroomNotes_TriggerBox] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

local NOTES_ID   = 432                    -- <<<<<< CHANGE THIS to your actual interactable ID
local MENU_NAME  = "PlayroomNotesInterface" -- <<<<<< CHANGE THIS to your actual menu name

local DOOR_CMD_TABLE = "DOOR_CMD"
local UI_TABLE       = "PLAYROOM_NOTES_UI_OPEN"

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
    SHARED[UI_TABLE][NOTES_ID] = false

    if DEBUG then
        print("[PlayroomNotes_TriggerBox] created. NOTES_ID=", NOTES_ID, " MENU=", MENU_NAME)
    end
end

function on_update(self, dt)
    if not consume_toggle(NOTES_ID) then
        return
    end

    if DEBUG then
        print("[PlayroomNotes_TriggerBox] Interact received for", NOTES_ID)
    end

    if UI_DisplayMenu ~= nil then
        UI_DisplayMenu(MENU_NAME)
    else
        print("[PlayroomNotes_TriggerBox] ERROR: UI_DisplayMenu binding missing")
    end

    SHARED[UI_TABLE][NOTES_ID] = not (SHARED[UI_TABLE][NOTES_ID] == true)

    if DEBUG then
        print("[PlayroomNotes_TriggerBox] UI open =", tostring(SHARED[UI_TABLE][NOTES_ID]))
    end
end
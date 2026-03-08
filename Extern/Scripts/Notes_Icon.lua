----------------------------------------------------------------
-- PlayroomNotes_IconUI.lua
-- Attach to the Notes ICON UI entity
--
-- Shows icon when:
--   - player is in range of notes
--   - optionally hides icon while note UI is open
----------------------------------------------------------------

print("[PlayroomNotes_IconUI] ===== SCRIPT FILE LOADED =====")

local NOTES_ID  = 432                 -- <<<<<< must match trigger script
local UI_TABLE  = "PLAYROOM_NOTES_UI_OPEN"
local HIDE_WHEN_UI_OPEN = true

local UI = nil

local function in_range()
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[NOTES_ID] == true
end

local function ui_open()
    if not HIDE_WHEN_UI_OPEN then return false end
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][NOTES_ID] == true
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[PlayroomNotes_IconUI] ERROR: No UIComponent 'EntityUI' on entity", EntityID)
        return
    end

    SHARED = SHARED or {}
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}

    UI:SetVisible(false)
end

function on_update(self, dt)
    if not UI then return end
    UI:SetVisible(in_range() and (not ui_open()))
end
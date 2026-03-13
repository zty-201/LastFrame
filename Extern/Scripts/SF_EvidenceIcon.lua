----------------------------------------------------------------
-- SF_EvidenceIcon.lua
-- Attach to ICON UI entity (must have EntityUI)
--
-- Shows icon when:
--   - in range: SHARED.INTERACT_IN_ZONE[BOOK_ID] == true
--   - optional: hide icon if interface open
----------------------------------------------------------------

print("[Book_IconUI] ===== SCRIPT FILE LOADED =====")

local BOOK_ID  = 764          -- <<<<<< CHANGE THIS
local UI_TABLE = "EVIDENCE_UI_OPEN"
local HIDE_WHEN_UI_OPEN = true

local UI = nil

local function in_range()
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[BOOK_ID] == true
end

local function ui_open()
    if not HIDE_WHEN_UI_OPEN then return false end
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][BOOK_ID] == true
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[Book_IconUI] ERROR: No UIComponent 'EntityUI' on entity", EntityID)
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

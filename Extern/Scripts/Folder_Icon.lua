----------------------------------------------------------------
-- Folder_Icon.lua
-- Attach to the folder interaction icon UI entity.
-- Shows when player is in range and folder is not already open.
----------------------------------------------------------------

local FOLDER_ID = 764       -- <<< world folder entity ID
local UI_TABLE  = "FOLDER_UI_OPEN"

local UI = nil

local function in_range()
    return SHARED.INTERACT_IN_ZONE ~= nil
        and SHARED.INTERACT_IN_ZONE[FOLDER_ID] == true
end

local function folder_open()
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][FOLDER_ID] == true
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[Folder_Icon] ERROR: No UIComponent on entity", EntityID)
        return
    end
    SHARED.INTERACT_IN_ZONE = SHARED.INTERACT_IN_ZONE or {}
    UI:SetVisible(false)
end

function on_update(self, dt)
    if not UI then return end
    UI:SetVisible(in_range() and not folder_open())
end

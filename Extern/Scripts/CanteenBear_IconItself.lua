print("[CanteenToy_IconUI] ===== SCRIPT FILE LOADED =====")

local TOY_ID   = 376
local UI_TABLE = "CANTEEN_TOY_UI_OPEN"

-- Hide icon while UI open (optional)
local HIDE_WHEN_UI_OPEN = true

local UI = nil

local function in_range()
    return SHARED
        and SHARED.INTERACT_IN_ZONE
        and SHARED.INTERACT_IN_ZONE[TOY_ID] == true
end

local function ui_open()
    if not HIDE_WHEN_UI_OPEN then return false end
    return SHARED and SHARED[UI_TABLE] and SHARED[UI_TABLE][TOY_ID] == true
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[CanteenToy_IconUI] ERROR: No EntityUI on", EntityID, "(attach to UI icon entity)")
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

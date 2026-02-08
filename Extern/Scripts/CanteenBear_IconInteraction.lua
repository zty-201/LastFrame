print("[CanteenToy_TriggerBox] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

-- MUST match InteractionBox ID for the canteen toy
local TOY_ID = 376

local MENU_NAME = "CanteenToyInterface"

local DOOR_CMD_TABLE = "DOOR_CMD"
local UI_TABLE       = "CANTEEN_TOY_UI_OPEN"

function on_create(self)
    SHARED = SHARED or {}
    SHARED[DOOR_CMD_TABLE] = SHARED[DOOR_CMD_TABLE] or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}
    SHARED[UI_TABLE][TOY_ID] = false

    if DEBUG then
        print("[CanteenToy_TriggerBox] created TOY_ID=", TOY_ID, " MENU=", MENU_NAME)
    end
end

local function consume_toggle(id)
    local t = SHARED and SHARED[DOOR_CMD_TABLE]
    if not t then return false end
    if t[id] == nil then return false end
    t[id] = nil
    return true
end

function on_update(self, dt)
    if not consume_toggle(TOY_ID) then return end

    if UI_DisplayMenu then
        UI_DisplayMenu(MENU_NAME)
    else
        print("[CanteenToy_TriggerBox] ERROR: UI_DisplayMenu missing")
    end

    SHARED[UI_TABLE][TOY_ID] = not (SHARED[UI_TABLE][TOY_ID] == true)

    if DEBUG then
        print("[CanteenToy_TriggerBox] UI_OPEN =", tostring(SHARED[UI_TABLE][TOY_ID]))
    end
end

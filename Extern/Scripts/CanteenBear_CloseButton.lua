print("[CanteenToyInterface_CrossButton] ===== SCRIPT FILE LOADED =====")

local DEBUG = true

-- MUST match the canteen interactable id
local TOY_ID = 376

-- MUST match what your interface root reads
local UI_TABLE = "CANTEEN_TOY_UI_OPEN"

-- Optional: if you want to explicitly close the current menu layer too
local CLOSE_MENUS_TOO = true

local function close_everything()
    SHARED = SHARED or {}
    SHARED[UI_TABLE] = SHARED[UI_TABLE] or {}
    SHARED[UI_TABLE][TOY_ID] = false

    if DEBUG then
        print("[CanteenToyInterface_CrossButton] set", UI_TABLE, TOY_ID, "-> false")
    end

    -- This guarantees the menu disappears even if something else is keeping it alive
    if CLOSE_MENUS_TOO and UI_CloseAllMenus ~= nil then
        UI_CloseAllMenus()
        if DEBUG then print("[CanteenToyInterface_CrossButton] UI_CloseAllMenus() called") end
    end

    -- Restore gameplay mouse mode
    if Input ~= nil and Input.SetCursorVisible ~= nil then
        Input.SetCursorVisible(false)
    end
    if Input ~= nil and Input.SetCursorLocked ~= nil then
        Input.SetCursorLocked(true)
    end
end

-- IMPORTANT: ButtonComponent.event_trigger must be "CloseCanteenToyUI"
function on_button_CloseCanteenToyUI(self)
    if DEBUG then
        print("[CanteenToyInterface_CrossButton] ===== X CLICK EVENT RECEIVED =====")
    end
    close_everything()
end

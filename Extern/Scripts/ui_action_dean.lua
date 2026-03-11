-- ============================================================
-- ui_flashlight.lua
-- Flashlight toggle icon (bottom-right, left of action icon)
-- ============================================================

local DEBUG = true

local UI = nil

local function debug_print(...)
    if DEBUG then
        print("[UI_Inventory_ANDROID]", ...)
    end
end

local function set_visible(v)
    if EntityUI == nil then
        print("[UI_Inventory] ERROR: EntityUI is nil on entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensureUIShared()
    local Shared = getShared()
    if not Shared then return nil end
    Shared.UI = Shared.UI or {}
    return Shared.UI
end

function on_button_InteractAction(self)
    SHARED.UI_FLASHLIGHT = SHARED.UI_FLASHLIGHT or true

    SHARED.UI_FLASHLIGHT = true
    debug_print("on_button_InteractAction")
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ui_action] ERROR: No UIComponent")
        return
    end

    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM
    local uiShared = ensureUIShared()
    -- Windows: hide the button UI
    if platform == "windows" then
        --UI:SetVisible(false)
        --return
    end

    --SHARED.UI_INVENTORY_OPEN = SHARED.UI_INVENTORY_OPEN or false

    SHARED.UI_FLASHLIGHT = false

    if not UI_is_MenuOpen("Android_Inventory_Buttons") then
        UI:SetVisible(false)
    else
        UI:SetVisible(true)
    end

end


function on_update(self, dt)
    
    -- Windows: nothing (UI hidden, flashlight probably on a key)
    if platform == "windows" then
        --SetVisible(false)
        --return
    end

    --entering to android
    --update itself is the menu is active, else return
    if not UI_is_MenuOpen("Android_Inventory_Buttons") then
        set_visible(false) --if not active, then make it gones
        return
    end

    set_visible(true)

end

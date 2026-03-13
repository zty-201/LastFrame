-- ============================================================
-- ui_flashlight.lua
-- Flashlight toggle icon (bottom-right, left of action icon)
-- ============================================================

local DEBUG = true

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

function on_button_ToggleFlashlight(self)

    local Shared = getShared()
    --local platform = (Shared and Shared.PLATFORM) or PLATFORM
    
    if Shared then
        Shared.FLASHLIGHT = Shared.FLASHLIGHT or {}
        Shared.FLASHLIGHT.requestToggle = true
    end

    debug_print("on_button_ToggleFlashlight")
end

function on_create(self)

    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM

    -- Windows: hide the button UI
    if platform == "windows" then
        UI:SetVisible(false)
        return
    end

    if not UI_is_MenuOpen("Android_Inventory_Buttons") then
        set_visible(false)
    else
        set_visible(true)
    end

end


function on_update(self, dt)
    
    -- Windows: nothing (UI hidden, flashlight probably on a key)
    --local Shared = getShared()
    --local platform = (Shared and Shared.PLATFORM) or PLATFORM
    if PLATFORM == "windows" then
        set_visible(false)
        return
    end

    --entering to android
    --update itself is the menu is active, else return
    if not UI_is_MenuOpen("Android_Inventory_Buttons") then
        set_visible(false) --if not active, then make it gones
        return
    end

    set_visible(true)

end

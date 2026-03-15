-- ============================================================
-- androidpausebutton.lua
-- Flashlight toggle icon (bottom-right, left of action icon)
-- ============================================================

local DEBUG = true

local UI = nil

local function debug_print(...)
    if DEBUG then
        print("[Android Pause Button]", ...)
    end
end

local function getShared()
    return rawget(_G, "SHARED")
end

local function set_visible(v)
    if EntityUI == nil then
        print("[UI_Inventory] ERROR: EntityUI is nil on entity " .. tostring(EntityID))
        return
    end
    EntityUI:SetVisible(v)
end

function on_button_AndroidPause(self)
    SHARED.ANDROID_PAUSE_BUTTON = SHARED.ANDROID_PAUSE_BUTTON or true

    SHARED.ANDROID_PAUSE_BUTTON = true
    debug_print("on_button_AndroidPause")
    StateMachine_SetTrigger("PauseGame")
end

function on_create(self)
    
    UI = EntityUI
    if not UI then
        print("[ui_action, on_button_AndroidPause] ERROR: No UIComponent")
        return
    end

    -- Windows: hide the button UI
    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM
    if platform == "windows" then
        UI:SetVisible(false)
        return
    end

    SHARED.ANDROID_PAUSE_BUTTON = false

    if not UI_is_MenuOpen("Android_Inventory_Buttons") then
        UI:SetVisible(false)
    else
        UI:SetVisible(true)
    end

end


function on_update(self, dt)
    local state_name = StateMachine_GetCurrentStateName()
    if state_name ~= "Gameplay" then
        return
    end
    -- Windows: nothing (UI hidden, flashlight probably on a key)
    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM
    if platform == "windows" then
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

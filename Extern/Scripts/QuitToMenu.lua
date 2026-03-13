-- QuitButton.lua
-- For now: "Quit" returns to StartMenu (not exiting app)

print("[QuitButton] SCRIPT LOADED")

local function set_ui_nav_lock(seconds)
    _G.__UI_NAV_LOCK = seconds or 0.25
end

function on_create(self)
    print("[QuitButton] on_create")
end

-- ButtonComponent.event_trigger MUST be "QuitButton"
function on_button_QuitToMenu(self)
    print("[QuitButton] Returning to Start Menu")

    if SHARED ~= nil then
        SHARED.UI_INVENTORY_OPEN = false
        SHARED.UI_INVENTORY_ANDROID_STATE = false
    end
    StateMachine_SetTrigger("ReturnToMenu")
    Scene_RequestRestart()
    UI_CloseAllMenus()

if PLATFORM == "android" then
    UI_DisplayMenu_InvisibleLayer("Android_Inventory_Buttons")
    UI_DisplayMenu("AndroidInputOverlay")
end
    _G.GAME_STATE = "MENU"
end

function on_destroy(self)
    print("[QuitButton] on_destroy")
end

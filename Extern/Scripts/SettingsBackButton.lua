-- SettingsBackButton.lua
-- Closes the Settings menu and returns to the Start Menu.
-- ButtonComponent.event_trigger must be: "SettingsBack"

print("[SettingsBackButton] SCRIPT LOADED")

function on_create(self)
end

function on_button_SettingsBack(self)
    print("[SettingsBackButton] Back -> StartMenu")
    UI_CloseAllMenus()
    UI_DisplayMenu("StartMenu")
    _G.__UI_NAV_LOCK = 0.2
end

function on_destroy(self)
end

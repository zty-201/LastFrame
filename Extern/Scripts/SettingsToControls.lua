-- SettingsToControls.lua
-- Opens the Controls menu from Settings.
-- ButtonComponent.event_trigger must be: "SettingsToControls"

print("[SettingsToControls] SCRIPT LOADED")

function on_create(self)
end

function on_button_SettingsToControls(self)
    print("[SettingsToControls] -> ControlsMenu")
    UI_CloseAllMenus()
    UI_DisplayMenu("ControlsMenu")
    _G.__UI_NAV_LOCK = 0.2
end

function on_destroy(self)
end

-- ControlsToSettings.lua
-- Returns from Controls menu back to Settings.
-- ButtonComponent.event_trigger must be: "ControlsToSettings"

print("[ControlsToSettings] SCRIPT LOADED")

function on_create(self)
end

function on_button_ControlsToSettings(self)
    print("[ControlsToSettings] -> Settings")
    UI_CloseAllMenus()
    UI_DisplayMenu("Settings")
    _G.__UI_NAV_LOCK = 0.2
end

function on_destroy(self)
end

-- SettingsButton.lua
-- Opens the PauseMenu when Settings button is clicked

print("[SettingsButton] SCRIPT LOADED")

function on_create(self)
    print("[SettingsButton] on_create")
end

function on_button_Settings(self)
    print("[SettingsButton] Settings button clicked -> opening PauseMenu")

    -- Optional: close all menus first
    UI_CloseAllMenus()

    -- Open PauseMenu
    UI_DisplayMenu("PauseMenu")
end

function on_destroy(self)
    print("[SettingsButton] on_destroy")
end

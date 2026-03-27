-- ControlsToMenuButton.lua
-- Opens the PauseMenu when Settings button is clicked

print("[ControlsToMenuButton] SCRIPT LOADED")

local function ui_nav_locked()
    return (_G.__UI_NAV_LOCK ~= nil and _G.__UI_NAV_LOCK > 0.0)
end

function on_create(self)
    print("[ControlsToMenuButton] on_create")
end

function on_update(self, dt)
    if _G.__UI_NAV_LOCK ~= nil and _G.__UI_NAV_LOCK > 0.0 then
        dt = dt or 0.016
        _G.__UI_NAV_LOCK = _G.__UI_NAV_LOCK - dt
        if _G.__UI_NAV_LOCK <= 0.0 then
            _G.__UI_NAV_LOCK = 0.0
        end
    end
end

function on_button_ControlsToMenu(self)
    if ui_nav_locked() then
        print("[ControlsToMenuButton] Ignored click (UI nav lock active)")
        return
    end

    print("[ControlsToMenuButton] Return button called -> Back to main menu")
    UI_CloseAllMenus()
    UI_DisplayMenu("StartMenu")
end

function on_destroy(self)
    print("[ControlsToMenuButton] on_destroy")
end

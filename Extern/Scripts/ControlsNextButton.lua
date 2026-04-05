----------------------------------------------------------------
-- ControlsNextButton.lua
-- Attach to entity 1056 (Next button).
-- ButtonComponent event_trigger must be: ControlsNextPage
--
-- Advances controls from page 1 -> page 2.
-- Hides itself on page 2 (no page 3 exists).
----------------------------------------------------------------

local function ui_nav_locked()
    return (_G.__UI_NAV_LOCK ~= nil and _G.__UI_NAV_LOCK > 0.0)
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED.CONTROLS_PAGE = SHARED.CONTROLS_PAGE or 1
    if EntityUI then EntityUI:SetVisible(true) end
end

function on_update(self, dt)
    if _G.__UI_NAV_LOCK ~= nil and _G.__UI_NAV_LOCK > 0.0 then
        _G.__UI_NAV_LOCK = _G.__UI_NAV_LOCK - (dt or 0.016)
        if _G.__UI_NAV_LOCK <= 0.0 then _G.__UI_NAV_LOCK = 0.0 end
    end

    -- Only visible when controls menu is open and on page 1
    if EntityUI then
        local menuOpen = UI_IsMenuActive and UI_IsMenuActive("ControlsMenu")
        local page = (SHARED and SHARED.CONTROLS_PAGE) or 1
        EntityUI:SetVisible(menuOpen and page == 1)
    end
end

function on_button_ControlsNextButton(self)
    if ui_nav_locked() then return end

    SHARED = SHARED or {}
    SHARED.CONTROLS_PAGE = 2

    _G.__UI_NAV_LOCK = 0.2   -- brief lock to prevent double-click

    print("[ControlsNextButton] -> page 2")
end

function on_destroy(self)
end

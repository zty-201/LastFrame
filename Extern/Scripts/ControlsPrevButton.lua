----------------------------------------------------------------
-- ControlsPrevButton.lua
-- Attach to entity 1067 (Prev page button).
-- ButtonComponent event_trigger must be: ControlsPrevButton
--
-- Goes from page 2 -> page 1.
-- Hidden (size_NDC 0,0) on page 1; shown with real size on page 2.
----------------------------------------------------------------

local REAL_W = -0.10
local REAL_H = 0.10

local function ui_nav_locked()
    return (_G.__UI_NAV_LOCK ~= nil and _G.__UI_NAV_LOCK > 0.0)
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED.CONTROLS_PAGE = SHARED.CONTROLS_PAGE or 1
    if EntityUI then
        EntityUI:SetSize_NDC(0.0, 0.0)
        EntityUI:SetVisible(false)
    end
end

function on_update(self, dt)
    if _G.__UI_NAV_LOCK ~= nil and _G.__UI_NAV_LOCK > 0.0 then
        _G.__UI_NAV_LOCK = _G.__UI_NAV_LOCK - (dt or 0.016)
        if _G.__UI_NAV_LOCK <= 0.0 then _G.__UI_NAV_LOCK = 0.0 end
    end

    if EntityUI then
        local menuOpen = UI_IsMenuActive and UI_IsMenuActive("ControlsMenu")
        local page = (SHARED and SHARED.CONTROLS_PAGE) or 1
        local show = menuOpen and page == 2
        if show then
            EntityUI:SetSize_NDC(REAL_W, REAL_H)
            EntityUI:SetVisible(true)
        else
            EntityUI:SetSize_NDC(0.0, 0.0)
            EntityUI:SetVisible(false)
        end
    end
end

function on_button_ControlsPrevButton(self)
    if ui_nav_locked() then return end

    SHARED = SHARED or {}
    SHARED.CONTROLS_PAGE = 1

    _G.__UI_NAV_LOCK = 0.2

    print("[ControlsPrevButton] -> page 1")
end

function on_destroy(self)
end

----------------------------------------------------------------
-- ControlsPage2.lua
-- Attach to entity 1055 (controls page 2).
-- Visible when the controls menu is on page 2.
----------------------------------------------------------------

function on_create(self)
    SHARED = SHARED or {}
    SHARED.CONTROLS_PAGE = SHARED.CONTROLS_PAGE or 1
    if EntityUI then EntityUI:SetVisible(false) end
end

function on_update(self, dt)
    if not EntityUI then return end
    local menuOpen = UI_IsMenuActive and UI_IsMenuActive("ControlsMenu")
    if not menuOpen then
        EntityUI:SetVisible(false)
        return
    end
    local page = (SHARED and SHARED.CONTROLS_PAGE) or 1
    EntityUI:SetVisible(page == 2)
end

function on_destroy(self)
end

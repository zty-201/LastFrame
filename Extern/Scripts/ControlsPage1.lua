----------------------------------------------------------------
-- ControlsPage1.lua
-- Attach to entity 10 (controls page 1).
-- Visible when the controls menu is on page 1.
----------------------------------------------------------------

function on_create(self)
    SHARED = SHARED or {}
    SHARED.CONTROLS_PAGE = 1
    if EntityUI then EntityUI:SetVisible(false) end
end

function on_update(self, dt)
    if not EntityUI then return end
    local menuOpen = UI_IsMenuActive and UI_IsMenuActive("ControlsMenu")
    if not menuOpen then
        -- Reset page when menu is closed so it starts fresh next time
        if SHARED then SHARED.CONTROLS_PAGE = 1 end
        EntityUI:SetVisible(false)
        return
    end
    local page = (SHARED and SHARED.CONTROLS_PAGE) or 1
    EntityUI:SetVisible(page == 1)
end

function on_destroy(self)
end

-- ============================================================
-- ui_calibrate_yt.lua
-- Calibrate / re-center camera icon (centre bottom)
-- ============================================================

local prevCDown = false

local UI = nil

local function getShared()
    return rawget(_G, "SHARED")
end

local function Calibrate_View_Center()
    local Shared = getShared()
    if not Shared then
        print("[ui_calibrate] WARNING: SHARED missing")
        return
    end

    Shared.UI = Shared.UI or {}
    Shared.UI.recenterRequested = true

    print("[ui_calibrate] Recenter requested")
end

function on_button_AndroidCalibrate(self)
    ------------------------------------------------------------
    -- Android: tap icon only
    ------------------------------------------------------------
    if not UI then UI = EntityUI end
    if not UI then return end
    if not UI:IsVisible() then return end

    Calibrate_View_Center()
    debug_print("on_button_AndroidPause")

end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ui_calibrate] ERROR: No UIComponent")
        return
    end

    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM

    -- Windows: hide the button UI (keyboard handles this)
    if platform == "windows" then
        --UI:SetVisible(false)
        --return
    end
    UI:SetVisible(true)
end

function on_update(self, dt)

    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM

    ------------------------------------------------------------
    -- Windows: keyboard only (UI hidden)
    ------------------------------------------------------------
    if platform == "windows" then
        local isCDown = Input.GetKey(GLFW_KEY_C)

        if isCDown and not prevCDown then
            Calibrate_View_Center()
        end

        prevCDown = isCDown
        --return
    end

    --if Android
    if not UI_is_MenuOpen("Android_Inventory_Buttons") then
        UI:SetVisible(false)
    else
        UI:SetVisible(true)
    end


end

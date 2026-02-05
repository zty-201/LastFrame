-- ============================================================
-- ui_calibrate.lua
-- Calibrate / re-center camera icon (centre bottom)
-- ============================================================

local ICON_SIZE = 75.0   -- very small icon
local ANCHOR_X  = 0.5    -- centre horizontally
local ANCHOR_Y  = 0.1  -- close to bottom
local prevCDown = false

local UI = nil

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensurePOVShared()
    local Shared = getShared()
    if not Shared then return nil end
    Shared.POV = Shared.POV or {}
    return Shared.POV
end

local function ensureUIShared()
    local Shared = getShared()
    if not Shared then return nil end
    Shared.UI = Shared.UI or {}
    return Shared.UI
end

local function getCanvasSize(ui)
    local w = (ui and ui.screenW) or (ui and ui.joyScreenWidth) or 2560.0
    local h = (ui and ui.screenH) or (ui and ui.joyScreenHeight) or 1440.0
    return w, h
end

local function isPointerOver()
    if not UI then
        UI = EntityUI
    end
    if not UI then return false end

    local uiShared = ensureUIShared()
    if not uiShared then return false end

    local mx, rawMy = Input.GetMousePosition()
    local _, screenH = getCanvasSize(uiShared)

    local my
    if uiShared.ToUIY then
        my = uiShared.ToUIY(rawMy)
    else
        my = screenH - rawMy
    end

    local x, y = UI:GetPosition()
    local w, h = UI:GetSize()

    return mx >= x and mx <= x + w
       and my >= y and my <= y + h
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
        UI:SetVisible(false)
        return
    end

    -- Android: show + position
    local uiShared = ensureUIShared()
    local screenW, screenH = getCanvasSize(uiShared)

    local centerX = ANCHOR_X * screenW
    local centerY = ANCHOR_Y * screenH

    UI:SetSize(ICON_SIZE, ICON_SIZE)
    UI:SetPosition(centerX - ICON_SIZE * 0.5, centerY - ICON_SIZE * 0.5)
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
        return
    end

    ------------------------------------------------------------
    -- Android: tap icon only
    ------------------------------------------------------------
    if not UI then UI = EntityUI end
    if not UI then return end
    if not UI:IsVisible() then return end

    local downMouse = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
    if downMouse and isPointerOver() then
        Calibrate_View_Center()
    end
end

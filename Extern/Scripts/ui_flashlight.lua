-- ============================================================
-- ui_flashlight.lua
-- Flashlight toggle icon (bottom-right, left of action icon)
-- ============================================================

local ICON_SIZE = 100.0   -- smaller than action icon
local ANCHOR_X  = 0.8    -- slightly left of action_icon
local ANCHOR_Y  = 0.1    -- align vertically with action_icon

local function getCanvasSize(ui)
    local w = (ui and ui.screenW) or (ui and ui.joyScreenWidth) or 2560.0
    local h = (ui and ui.screenH) or (ui and ui.joyScreenHeight) or 1440.0
    return w, h
end

local UI = nil

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensureUIShared()
    local Shared = getShared()
    if not Shared then return nil end
    Shared.UI = Shared.UI or {}
    return Shared.UI
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


function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ui_flashlight] ERROR: No UIComponent")
        return
    end

    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM

    -- Windows: hide the button UI
    if platform == "windows" then
        UI:SetVisible(false)
        return
    end

    -- Android: show + position
    --local uiShared = ensureUIShared()
    --local screenW, screenH = getCanvasSize(uiShared)

    --local centerX = ANCHOR_X * screenW
    --local centerY = ANCHOR_Y * screenH

    --UI:SetSize(ICON_SIZE, ICON_SIZE)
    --UI:SetPosition(centerX - ICON_SIZE * 0.5, centerY - ICON_SIZE * 0.5)
    --UI:SetVisible(true)
end


function on_update(self, dt)

    -- UI = EntityUI

    -- local Shared = getShared()
    -- local platform = (Shared and Shared.PLATFORM) or PLATFORM

    -- -- Windows: nothing (UI hidden, flashlight probably on a key)
    -- if platform == "windows" then
    --     UI:SetVisible(false)
    --     return
    -- end

    -- if not UI then UI = EntityUI end
    -- if not UI then return end
    -- if not UI:IsVisible() then return end

    -- local down = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
    -- if down and isPointerOver() then
    --     Shared = getShared()
    --     if Shared then
    --         Shared.FLASHLIGHT = Shared.FLASHLIGHT or {}
    --         Shared.FLASHLIGHT.requestToggle = true
    --     end
    -- end
end

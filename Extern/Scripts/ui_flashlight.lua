-- ============================================================
-- ui_flashlight.lua
-- Flashlight toggle icon (bottom-right, left of action icon)
-- ============================================================

local ICON_SIZE = 100.0   -- smaller than action icon
local ANCHOR_X  = 0.70    -- slightly left of action_icon
local ANCHOR_Y  = 0.17    -- align vertically with action_icon

local screenWidth, screenHeight = 2340, 1080

local UI = nil

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensureUIShared()
    local SH = getShared()
    if not SH then return nil end
    SH.UI = SH.UI or {}
    return SH.UI
end

local function isPointerOver()
    if not UI then
        UI = EntityUI
    end
    if not UI then return false end

    local uiShared = ensureUIShared()
    if not uiShared then return false end

    local mx, rawMy = Input.GetMousePosition()
    local virtualH   = uiShared.joyScreenHeight or 1440.0
    local my         = virtualH - rawMy

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

    local centerX = ANCHOR_X * screenWidth
    local centerY = ANCHOR_Y * screenHeight

    UI:SetSize(ICON_SIZE, ICON_SIZE)
    UI:SetPosition(centerX - ICON_SIZE * 0.5, centerY - ICON_SIZE * 0.5)
    UI:SetVisible(true)
end

function on_update(self, dt)
    if not UI then
        UI = EntityUI
    end
    if not UI then return end

    local down = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
    if down and isPointerOver() then
        -- Send a toggle request through SHARED.FLASHLIGHT
        local SH = getShared()
        if SH then
            SH.FLASHLIGHT = SH.FLASHLIGHT or {}
            SH.FLASHLIGHT.requestToggle = true
        end
    end
end
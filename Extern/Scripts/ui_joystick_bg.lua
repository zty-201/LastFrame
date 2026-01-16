-- ============================================================
-- ui_joystick_bg.lua (2D joystick background)
-- ============================================================

local ANCHOR_X = 0.1
local ANCHOR_Y = 0.1
local BG_SIZE  = 350.0

local UI = nil

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensureUIShared()
    local SH = getShared()
    SH.UI = SH.UI or {}
    return SH.UI
end

local function getCanvasSize(ui)
    local w = (ui and ui.screenW) or (ui and ui.joyScreenWidth) or 2560.0
    local h = (ui and ui.screenH) or (ui and ui.joyScreenHeight) or 1440.0
    return w, h
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ui_joystick_bg] ERROR: No UIComponent")
        return
    end

    local uiShared = ensureUIShared()
    local screenW, screenH = getCanvasSize(uiShared)

    local centerX = ANCHOR_X * screenW
    local centerY = ANCHOR_Y * screenH

    UI:SetPosition(centerX - BG_SIZE * 0.5, centerY - BG_SIZE * 0.5)
    UI:SetSize(BG_SIZE, BG_SIZE)
    UI:SetVisible(true)

    uiShared.joyCenterX      = centerX
    uiShared.joyCenterY      = centerY
    uiShared.joyRadius       = BG_SIZE * 0.5

    -- Keep these for all scripts that still reference them
    uiShared.joyScreenWidth  = screenW
    uiShared.joyScreenHeight = screenH
end

function on_update(self, dt)
    if not UI then return end

    -- Read current UI position/size (already adjusted by resolution / fullscreen)
    local x, y = UI:GetPosition()
    local w, h = UI:GetSize()

    -- Recompute center from the actual rect on screen
    local centerX = x + w * 0.5
    local centerY = y + h * 0.5

    -- Push updated center + radius into SHARED.UI every frame
    local uiShared = ensureUIShared()
    uiShared.joyCenterX = centerX
    uiShared.joyCenterY = centerY
    uiShared.joyRadius  = math.min(w, h) * 0.5
end

-- ============================================================
-- ui_joystick_bg.lua (2D joystick background)
-- ============================================================

local ANCHOR_X = 0.18
local ANCHOR_Y = 0.20
local BG_SIZE  = 150.0

local screenWidth, screenHeight = 3088, 1440

local UI = nil

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensureUIShared()
    local SH = getShared()
    SH.UI = SH.UI or {}
    return SH.UI
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ui_joystick_bg] ERROR: No UIComponent")
        return
    end

    local centerX = ANCHOR_X * screenWidth
    local centerY = ANCHOR_Y * screenHeight

    UI:SetPosition(centerX - BG_SIZE * 0.5, centerY - BG_SIZE * 0.5)
    UI:SetSize(BG_SIZE, BG_SIZE)
    UI:SetVisible(true)

    local uiShared = ensureUIShared()
    uiShared.joyCenterX = centerX
    uiShared.joyCenterY = centerY
    uiShared.joyRadius  = BG_SIZE * 0.5
end

function on_update(self, dt)
    -- background does nothing each frame
end

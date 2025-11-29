-- ============================================================
-- ui_joystick_bg.lua (2D joystick background)
-- ============================================================

local ANCHOR_X = 0.15
local ANCHOR_Y = 0.30
local BG_SIZE  = 550.0

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
    uiShared.joyCenterX       = centerX
    uiShared.joyCenterY       = centerY
    uiShared.joyRadius        = BG_SIZE * 0.5
    uiShared.joyScreenHeight  = screenHeight   -- <<< NEW: reference UI height
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

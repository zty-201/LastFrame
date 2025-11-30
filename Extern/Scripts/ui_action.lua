-- ============================================================
-- ui_action.lua  (Windows + Android, flashlight-style hitbox)
-- Big action icon (bottom-right)
-- ============================================================

local ICON_SIZE = 200.0
local ANCHOR_X  = 0.80
local ANCHOR_Y  = 0.22
local prevEDown = false

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

-- ============================================================
-- Hitbox: same as ui_flashlight
-- ============================================================
local function isPointerOver()
    if not UI then
        UI = EntityUI
    end
    if not UI then return false end

    local uiShared = ensureUIShared()
    if not uiShared then return false end

    local mx, rawMy = Input.GetMousePosition()
    local virtualH   = uiShared.joyScreenHeight or 1440.0
    local my         = virtualH - rawMy   -- SAME as ui_flashlight

    local x, y = UI:GetPosition()
    local w, h = UI:GetSize()

    return mx >= x and mx <= x + w and my >= y and my <= y + h
end

-- ============================================================
-- on_create
-- ============================================================
function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ui_action] ERROR: No UIComponent")
        return
    end

    local centerX = ANCHOR_X * screenWidth
    local centerY = ANCHOR_Y * screenHeight

    UI:SetSize(ICON_SIZE, ICON_SIZE)
    UI:SetPosition(centerX - ICON_SIZE * 0.5, centerY - ICON_SIZE * 0.5)
    UI:SetVisible(true)
end

-- ============================================================
-- on_update
-- ============================================================
function on_update(self, dt)
    if not UI then return end

    local uiShared = ensureUIShared()
    if not uiShared then return end

    ------------------------------------------------------------
    -- CLICK / TAP ON ICON (Windows + Android)
    ------------------------------------------------------------
    local downMouse = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)

    if downMouse and isPointerOver() then
        print("[ui_action] Icon pressed")
        uiShared.actionRequested = true
    end

    ------------------------------------------------------------
    -- KEYBOARD E (Windows)
    ------------------------------------------------------------
    local SH = getShared()
    if SH and SH.PLATFORM == "windows" then
        local isEDown = Input.GetKey(GLFW_KEY_E)

        if isEDown and not prevEDown then
            print("[ui_action] E pressed (action)")
            uiShared.actionRequested = true
        end

        prevEDown = isEDown
    end
end

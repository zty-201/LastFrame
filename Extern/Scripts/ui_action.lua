-- ============================================================
-- ui_action.lua  (FIXED FOR WINDOWS + ANDROID CLICK)
-- Big action icon (bottom-right)
-- ============================================================

local ICON_SIZE = 350.0
local ANCHOR_X  = 0.90
local ANCHOR_Y  = 0.22
local prevEDown = false

local screenWidth, screenHeight = 3088, 1440

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
-- FIXED: Pointer detection works on BOTH Windows + Android
-- ============================================================
local function isPointerOver()
    if not UI then UI = EntityUI end
    if not UI then return false end

    local uiShared = ensureUIShared()
    if not uiShared then return false end

    -- Raw OS mouse position
    local mx, rawMy = Input.GetMousePosition()

    -- FIX:
    -- Windows already uses bottom-left origin → DO NOT flip Y
    -- Android uses top-left → MUST flip
    local my

    if PLATFORM == "android" then
        local virtualH = uiShared.joyScreenHeight or 1440.0
        my = virtualH - rawMy
    else
        -- WINDOWS
        my = rawMy
    end

    -- Icon rectangle
    local x, y = UI:GetPosition()
    local w, h = UI:GetSize()

    return (mx >= x and mx <= x + w and my >= y and my <= y + h)
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

    -- initial layout
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
    -- CLICK / TAP ON ICON
    ------------------------------------------------------------
    local downMouse = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)

    if downMouse and isPointerOver() then
        print("[ui_action] Icon pressed")
        uiShared.actionRequested = true
    end

    ------------------------------------------------------------
    -- KEYBOARD E (Windows)
    ------------------------------------------------------------
    if PLATFORM == "windows" then
        local isEDown = Input.GetKey(GLFW_KEY_E)

        if isEDown and not prevEDown then
            print("[ui_action] E pressed (action)")
            uiShared.actionRequested = true
        end

        prevEDown = isEDown
    end
end

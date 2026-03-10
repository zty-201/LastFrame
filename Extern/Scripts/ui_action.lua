-- ============================================================
-- ui_action.lua  (Windows + Android, flashlight-style hitbox)
-- Big action icon (bottom-right)
-- ============================================================

local ICON_SIZE = 200.0
local ANCHOR_X  = 0.9
local ANCHOR_Y  = 0.1
local prevEDown = false

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

local function getCanvasSize(uiShared)
    local w = (uiShared and uiShared.screenW) or (uiShared and uiShared.joyScreenWidth) or 2560.0
    local h = (uiShared and uiShared.screenH) or (uiShared and uiShared.joyScreenHeight) or 1440.0
    return w, h
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
    local _, screenH = getCanvasSize(uiShared)

    local my
    if uiShared.ToUIY then
        my = uiShared.ToUIY(rawMy)
    else
        my = screenH - rawMy
    end

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

    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM

    -- Windows: hide the button UI (keyboard handles actions)
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

-- ============================================================
-- on_update
-- ============================================================
function on_update(self, dt)
    local uiShared = ensureUIShared()
    if not uiShared then return end

    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM

    ------------------------------------------------------------
    -- Windows: keyboard only (UI hidden)
    ------------------------------------------------------------
    if platform == "windows" then
        local isEDown = Input.GetKey(GLFW_KEY_E)

        if isEDown and not prevEDown then
            print("[ui_action] E pressed (action)")
            uiShared.actionRequested = true
        end

        local A_input = SHARED.UI_FLASHLIGHT

        if A_input then
            print("[ui_action] E pressed (action) YYYYYYOOOOOO")
            uiShared.actionRequested = true
            SHARED.UI_FLASHLIGHT = false
        end

        prevEDown = isEDown
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
        print("[ui_action] Icon pressed")
        uiShared.actionRequested = true
    end
end

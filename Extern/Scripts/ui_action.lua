-- ============================================================
-- ui_action.lua
-- Big action icon (bottom-right)
-- ============================================================

local ICON_SIZE = 200.0   -- big icon
local ANCHOR_X  = 0.80    -- 0 = left, 1 = right
local ANCHOR_Y  = 0.22    -- 0 = bottom, 1 = top
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

local function isPointerOver()
    if not UI then
        UI = EntityUI
    end
    if not UI then return false end

    local uiShared = ensureUIShared()
    if not uiShared then return false end

    -- Mouse position in input space (top-left origin)
    local mx, rawMy = Input.GetMousePosition()

    -- Convert Y into the same bottom-left space as UI (same as joystick)
    local virtualH = uiShared.joyScreenHeight or 1440.0
    local my       = virtualH - rawMy

    -- Rect of this icon in UI space (bottom-left origin)
    local x, y = UI:GetPosition()
    local w, h = UI:GetSize()

    return mx >= x and mx <= x + w
       and my >= y and my <= y + h
end


function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ui_action] ERROR: No UIComponent")
        return
    end

    -- initial layout in virtual screen space
    local centerX = ANCHOR_X * screenWidth
    local centerY = ANCHOR_Y * screenHeight

    UI:SetSize(ICON_SIZE, ICON_SIZE)
    UI:SetPosition(centerX - ICON_SIZE * 0.5, centerY - ICON_SIZE * 0.5)
    UI:SetVisible(true)
end

function on_update(self, dt)
    if not UI then return end

    -- ===========================
    -- MOUSE/TAP ON ICON
    -- ===========================
    local downMouse = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
    if downMouse and isPointerOver() then
        -- Placeholder – actual action will be wired later
        print("[ui_action] Icon pressed (no gameplay hooked yet)")
    end

    -- ===========================
    -- WINDOWS KEYBOARD: E to "press" action
    -- ===========================
    if PLATFORM == "windows" then
        local isEDown = Input.GetKey(GLFW_KEY_E)

        -- Edge-detect: only fire once per key press
        if isEDown and not prevEDown then
            -- Same behaviour as clicking the button
            print("[ui_action] E pressed (action button)")
            -- later you can also set SHARED.UI.actionRequested = true here
        end

        prevEDown = isEDown
    end
end


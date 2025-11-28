-- ============================================================
-- ui_joystick.lua (2D joystick knob)
-- ============================================================

local KNOB_SIZE = 90.0
local DEAD_ZONE = 0.05
local MAX_MAG = 1.0

local UI = nil

local isDragging = false
local dragStartX = 0
local dragStartY = 0

local knobCenterX = 0
local knobCenterY = 0

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensureUIShared()
    local SH = getShared()
    SH.UI = SH.UI or {}
    local u = SH.UI
    u.joystickX = u.joystickX or 0
    u.joystickY = u.joystickY or 0
    u.joystickActive = u.joystickActive or false
    return u
end

local function length(x, y)
    return math.sqrt(x*x + y*y)
end

function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ui_joystick] ERROR: No UIComponent")
        return
    end

    UI:SetSize(KNOB_SIZE, KNOB_SIZE)

    local uiShared = ensureUIShared()
    knobCenterX = uiShared.joyCenterX or 0
    knobCenterY = uiShared.joyCenterY or 0

    UI:SetPosition(knobCenterX - KNOB_SIZE * 0.5, knobCenterY - KNOB_SIZE * 0.5)
end

function on_update(self, dt)
    if not UI then return end

    local uiShared = ensureUIShared()
    local centerX = uiShared.joyCenterX
    local centerY = uiShared.joyCenterY
    local radius  = uiShared.joyRadius

    knobCenterX = centerX
    knobCenterY = centerY

    local mx, my = Input.GetMousePosition()
    local down = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
    local held = Input.GetMouseButton(GLFW_MOUSE_BUTTON_LEFT)
    local up   = Input.GetMouseButtonUp(GLFW_MOUSE_BUTTON_LEFT)

    if down then
        local dx = mx - centerX
        local dy = my - centerY
        if dx*dx + dy*dy <= radius*radius then
            isDragging = true
            dragStartX = mx
            dragStartY = my
        end
    end

    if held and isDragging then
        local dx = mx - centerX
        local dy = my - centerY

        local dist = length(dx, dy)
        local clampedDX = dx
        local clampedDY = dy

        if dist > radius then
            local k = radius / dist
            clampedDX = dx * k
            clampedDY = dy * k
        end

        UI:SetPosition(centerX + clampedDX - KNOB_SIZE * 0.5, centerY + clampedDY - KNOB_SIZE * 0.5)

        local nx = clampedDX / radius
        local ny = clampedDY / radius

        local mag = length(nx, ny)
        if mag < DEAD_ZONE then
            nx, ny = 0, 0
        elseif mag > MAX_MAG then
            nx = nx / mag
            ny = ny / mag
        end

        uiShared.joystickX = -nx
        uiShared.joystickY = ny
        uiShared.joystickActive = (nx ~= 0 or ny ~= 0)
    end

    if up then
        isDragging = false
        UI:SetPosition(centerX - KNOB_SIZE * 0.5,centerY - KNOB_SIZE * 0.5)
        uiShared.joystickX = 0
        uiShared.joystickY = 0
        uiShared.joystickActive = false
    end
end

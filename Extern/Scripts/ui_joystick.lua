-- ============================================================
-- ui_joystick.lua (2D joystick knob)
-- ============================================================

local KNOB_SIZE = 225.0
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
    local radius   = uiShared.joyRadius or 200.0  -- fallback radius

    -- Actual current knob rect on screen (already scaled / laid out by UI)
    local ux, uy = UI:GetPosition()
    local uw, uh = UI:GetSize()
    local visualCenterX = ux + uw * 0.5
    local visualCenterY = uy + uh * 0.5

    -- "Rest" center follows the icon when not dragging, so it moves with resolution changes.
    if not isDragging then
        knobCenterX = visualCenterX
        knobCenterY = visualCenterY
    end

    -- Mouse position: X is already correct, Y needs to be flipped to match UI's bottom-left origin
    local mx, rawMy = Input.GetMousePosition()
    local virtualH   = uiShared.joyScreenHeight or 1440.0  -- same as screenHeight in bg script
    local my         = virtualH - rawMy

    local down = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
    local held = Input.GetMouseButton(GLFW_MOUSE_BUTTON_LEFT)
    local up   = Input.GetMouseButtonUp(GLFW_MOUSE_BUTTON_LEFT)

    -- Start dragging only if we pressed inside the knob's visual area
    if down then
        local dx = mx - visualCenterX
        local dy = my - visualCenterY
        if dx*dx + dy*dy <= radius*radius then
            isDragging = true
            dragStartX = mx
            dragStartY = my
        end
    end

    if held and isDragging then
        -- Movement is relative to the rest center
        local dx = mx - knobCenterX
        local dy = my - knobCenterY

        local dist      = length(dx, dy)
        local clampedDX = dx
        local clampedDY = dy

        if dist > radius then
            local k = radius / dist
            clampedDX = dx * k
            clampedDY = dy * k
        end

        -- Position knob sprite
        UI:SetPosition(
            knobCenterX + clampedDX - KNOB_SIZE * 0.5,
            knobCenterY + clampedDY - KNOB_SIZE * 0.5
        )

        -- Normalised joystick direction
        local nx = clampedDX / radius
        local ny = clampedDY / radius

        local mag = length(nx, ny)
        if mag < DEAD_ZONE then
            nx, ny = 0, 0
        elseif mag > MAX_MAG then
            nx = nx / mag
            ny = ny / mag
        end

        uiShared.joystickX      = nx
        uiShared.joystickY      = -ny
        uiShared.joystickActive = (nx ~= 0 or ny ~= 0)
    end

    if up then
        isDragging = false

        -- Snap knob back to its rest center
        UI:SetPosition(
            knobCenterX - KNOB_SIZE * 0.5,
            knobCenterY - KNOB_SIZE * 0.5
        )

        uiShared.joystickX      = 0
        uiShared.joystickY      = 0
        uiShared.joystickActive = false
    end
end

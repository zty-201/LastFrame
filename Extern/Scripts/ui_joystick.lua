-- ============================================================
-- ui_joystick.lua (2D joystick knob) - Android compatible
-- ============================================================

local KNOB_SIZE = 180.0
local DEAD_ZONE = 0.05
local MAX_MAG   = 1.0

local UI = nil

local isDragging = false

local knobCenterX = 0
local knobCenterY = 0

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensureUIShared()
    local Shared = getShared()
    if not Shared then return nil end

    Shared.UI = Shared.UI or {}
    local u = Shared.UI

    u.joystickX      = u.joystickX      or 0
    u.joystickY      = u.joystickY      or 0
    u.joystickActive = u.joystickActive or false

    return u
end

local function length(x, y)
    return math.sqrt(x * x + y * y)
end

-- ============================================================
-- on_create
-- ============================================================
function on_create(self)
    UI = EntityUI
    if not UI then
        print("[ui_joystick] ERROR: No UIComponent")
        return
    end

    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM

    -- Windows: hide joystick UI
    if platform == "windows" then
        UI:SetVisible(false)
        return
    end

    -- Android: show + init
    UI:SetVisible(true)
    UI:SetSize(KNOB_SIZE, KNOB_SIZE)

    local uiShared = ensureUIShared()
    if not uiShared then return end

    -- joyCenterX / joyCenterY set by bg script
    knobCenterX = uiShared.joyCenterX or 0
    knobCenterY = uiShared.joyCenterY or 0

    UI:SetPosition(
        knobCenterX - KNOB_SIZE * 0.5,
        knobCenterY - KNOB_SIZE * 0.5
    )
end


-- ============================================================
-- on_update
-- ============================================================
function on_update(self, dt)
    local Shared = getShared()
    local platform = (Shared and Shared.PLATFORM) or PLATFORM

    -- Windows: do nothing (UI hidden)
    if platform == "windows" then
        return
    end

    if not UI then UI = EntityUI end
    if not UI then return end
    if not UI:IsVisible() then return end

    local uiShared = ensureUIShared()
    if not uiShared then return end

    local radius = uiShared.joyRadius or 200.0

    ------------------------------------------------------------
    -- Current visual centre of the knob
    ------------------------------------------------------------
    local ux, uy = UI:GetPosition()
    local uw, uh = UI:GetSize()
    local visualCenterX = ux + uw * 0.5
    local visualCenterY = uy + uh * 0.5

    -- When not dragging, the rest center follows the visual center
    if not isDragging then
        knobCenterX = visualCenterX
        knobCenterY = visualCenterY
    end

    ------------------------------------------------------------
    -- Pointer (mouse / first touch)
    ------------------------------------------------------------
    local mx, rawMy = Input.GetMousePosition()

    local virtualH = uiShared.screenH or uiShared.joyScreenHeight or 1440.0
    local my
    if uiShared.ToUIY then
        my = uiShared.ToUIY(rawMy)
    else
        my = virtualH - rawMy
    end

    local down = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
    local held = Input.GetMouseButton(GLFW_MOUSE_BUTTON_LEFT)
    local up   = false
    if Input.GetMouseButtonUp then
        up = Input.GetMouseButtonUp(GLFW_MOUSE_BUTTON_LEFT)
    end

    ------------------------------------------------------------
    -- Start dragging if press begins inside joystick radius
    ------------------------------------------------------------
    if down then
        local dx = mx - visualCenterX
        local dy = my - visualCenterY
        if dx * dx + dy * dy <= radius * radius then
            isDragging = true
        end
    end

    ------------------------------------------------------------
    -- While held, move knob and output axis
    ------------------------------------------------------------
    if held and isDragging then
        local dx = mx - knobCenterX
        local dy = my - knobCenterY

        local dist      = length(dx, dy)
        local clampedDX = dx
        local clampedDY = dy

        if dist > radius and dist > 0 then
            local k = radius / dist
            clampedDX = dx * k
            clampedDY = dy * k
        end

        UI:SetPosition(
            knobCenterX + clampedDX - KNOB_SIZE * 0.5,
            knobCenterY + clampedDY - KNOB_SIZE * 0.5
        )

        local nx = clampedDX / radius
        local ny = clampedDY / radius

        local mag = length(nx, ny)
        if mag < DEAD_ZONE then
            nx, ny = 0, 0
        elseif mag > MAX_MAG then
            nx = nx / mag
            ny = ny / mag
        end

        uiShared.joystickX      = -nx
        uiShared.joystickY      = ny
        uiShared.joystickActive = (nx ~= 0 or ny ~= 0)
    end

    ------------------------------------------------------------
    -- Release (Android-safe)
    ------------------------------------------------------------
    if (up or (not held and not down)) and isDragging then
        isDragging = false

        UI:SetPosition(
            knobCenterX - KNOB_SIZE * 0.5,
            knobCenterY - KNOB_SIZE * 0.5
        )

        uiShared.joystickX      = 0
        uiShared.joystickY      = 0
        uiShared.joystickActive = false
    end
end

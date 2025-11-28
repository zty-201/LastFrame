-- ============================================================
-- ui_element_follow_camera.lua
-- Attach this to a quad / sprite entity that should behave as a UI element.
-- It keeps the element in screen-space in front of the camera and
-- fires click callbacks when tapped/clicked.
-- ============================================================

-- ==== CONFIG (PER ELEMENT) ===================================
-- Bottom-left joystick
local ANCHOR_X = 0.18   -- 0 = left, 1 = right
local ANCHOR_Y = 0.20   -- 0 = bottom, 1 = top

local HIT_RADIUS_NORM = 0.12
local UI_DEPTH = 5.0

local ROLE = "joystick"

-- rotation offset so the flat quad faces the camera
local QUAD_PITCH_OFFSET = -90.0
local QUAD_YAW_OFFSET   = 0.0
local QUAD_ROLL_OFFSET  = 0.0

-- ============================================================
local transform = nil
local camera    = nil

-- You already use this in other scripts
local screenWidth, screenHeight = 3088, 1440   -- keep in sync with player_movement.lua :contentReference[oaicite:6]{index=6}

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensureUIShared()
    local SHARED = getShared()
    if not SHARED then return nil end
    SHARED.UI = SHARED.UI or {}
    local UI = SHARED.UI

    UI.joystickX        = UI.joystickX        or 0.0
    UI.joystickY        = UI.joystickY        or 0.0
    UI.joystickActive   = UI.joystickActive   or false
    UI.actionPressed    = UI.actionPressed    or false
    UI.flashPressed     = UI.flashPressed     or false
    UI.bagPressed       = UI.bagPressed       or false
    UI.recenterRequested= UI.recenterRequested or false

    return UI
end

local function sqr(x) return x * x end

local function inCircle(px, py, cx, cy, r)
    return sqr(px - cx) + sqr(py - cy) <= r * r
end

function on_create(self)
    transform = EntityTransform
    camera    = GetMainCamera()

    if not transform then print("[ui_element] ERROR: missing TransformComponent") end
    if not camera then print("[ui_element] ERROR: GetMainCamera returned nil") end

    ensureUIShared()
end

function on_update(self, dt)
    if not transform or not camera then return end

    -------------------------------------------------
    -- 1) Get camera basis from its position/target
    -------------------------------------------------
    local cx, cy, cz = camera:GetPosition()
    local tx, ty, tz = camera:GetTarget()

    -- forward = normalize(target - position)
    local fx = tx - cx
    local fy = ty - cy
    local fz = tz - cz
    local fLen = math.sqrt(fx*fx + fy*fy + fz*fz)
    if fLen < 0.0001 then return end
    fx, fy, fz = fx/fLen, fy/fLen, fz/fLen

    -- build right / up from worldUp
    local worldUpX, worldUpY, worldUpZ = 0.0, 1.0, 0.0

    -- Standard basis:
    --   right = normalize( forward × worldUp )
    --   up    = right × forward
    local rightX = fy * worldUpZ - fz * worldUpY
    local rightY = fz * worldUpX - fx * worldUpZ
    local rightZ = fx * worldUpY - fy * worldUpX

    local rLen = math.sqrt(rightX*rightX + rightY*rightY + rightZ*rightZ)
    if rLen > 0.0001 then
        rightX, rightY, rightZ = rightX/rLen, rightY/rLen, rightZ/rLen
    end

    local upX = rightY * fz - rightZ * fy
    local upY = rightZ * fx - rightX * fz
    local upZ = rightX * fy - rightY * fx


    -------------------------------------------------
    -- 2) Place the quad at the requested screen anchor
    -------------------------------------------------
    local fovDeg = camera:GetFOV()
    local aspect = screenWidth / screenHeight
    local halfHeight = UI_DEPTH * math.tan(math.rad(fovDeg * 0.5))
    local halfWidth  = halfHeight * aspect

    local ndcX = ANCHOR_X * 2.0 - 1.0 -- [-1,1]
    local ndcY = ANCHOR_Y * 2.0 - 1.0

    local offRight = ndcX * halfWidth
    local offUp    = ndcY * halfHeight

    local worldX = cx + fx * UI_DEPTH + rightX * offRight + upX * offUp
    local worldY = cy + fy * UI_DEPTH + rightY * offRight + upY * offUp
    local worldZ = cz + fz * UI_DEPTH + rightZ * offRight + upZ * offUp

    transform:SetPosition(worldX, worldY, worldZ)

    -------------------------------------------------
    -- 3) Make the quad follow the FPS camera rotation
    --    using SHARED.POV yaw/pitch + a fixed offset
    -------------------------------------------------
   local SHARED = getShared()
    local yaw, pitch = 0.0, 0.0
    if SHARED and SHARED.POV then
        yaw   = SHARED.POV.yaw   or 0.0
        pitch = SHARED.POV.pitch or 0.0
    else
        -- fallback if POV not available
        yaw   = math.deg(math.atan2(fx, fz))
        pitch = math.deg(math.asin(fy))
    end

    -- Now we *add* your mesh offsets on top of the camera angles
    local quadPitch = pitch + QUAD_PITCH_OFFSET
    local quadYaw   = yaw   + QUAD_YAW_OFFSET
    local quadRoll  = QUAD_ROLL_OFFSET
    
    -- invert pitch for some reason
    transform:SetRotation(-quadPitch, quadYaw, quadRoll)

    -------------------------------------------------
    -- 4) Existing click / touch handling (unchanged)
    -------------------------------------------------
    local UI = ensureUIShared()
    if not UI then return end

    -- clear per-frame flags on this element
    if ROLE == "action" then
        UI.actionPressed = false
    elseif ROLE == "flashlight" then
        UI.flashPressed = false
    elseif ROLE == "bag" then
        UI.bagPressed = false
    end

    local down = Input.GetMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT)
    local held = Input.GetMouseButton(GLFW_MOUSE_BUTTON_LEFT)
    local up   = Input.GetMouseButtonUp(GLFW_MOUSE_BUTTON_LEFT)
    local mx, my = Input.GetMousePosition()

    local minDim = math.min(screenWidth, screenHeight)
    local hitRadiusPx = HIT_RADIUS_NORM * minDim

    local centerXpx = ANCHOR_X * screenWidth
    local centerYpx = ANCHOR_Y * screenHeight

    if ROLE == "joystick" then
        -- (same joystick logic as before...)
        -- copy your old joystick block here
    else
        if down and inCircle(mx, my, centerXpx, centerYpx, hitRadiusPx) then
            if ROLE == "action" then
                UI.actionPressed = true
            elseif ROLE == "flashlight" then
                UI.flashPressed = true
                if Flashlight_Toggle then
                    Flashlight_Toggle()
                end
            elseif ROLE == "bag" then
                UI.bagPressed = true
            elseif ROLE == "recenter" then
                UI.recenterRequested = true
            end
        end
    end
end

function on_destroy()
end

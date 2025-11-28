-- ============================================================
-- ui_joystick.lua     (JOYSTICK KNOB)
-- ============================================================

local ANCHOR_X = 0.18
local ANCHOR_Y = 0.20
local UI_DEPTH = 4.9

local HIT_RADIUS_NORM = 0.12

local QUAD_PITCH_OFFSET = -90.0
local QUAD_YAW_OFFSET   = 0.0
local QUAD_ROLL_OFFSET  = 0.0

local screenWidth, screenHeight = 3088,1440

local transform = nil
local camera = nil

local function getShared()
    return rawget(_G, "SHARED")
end

local function ensureUIShared()
    local SH = getShared()
    SH.UI = SH.UI or {}
    local UI = SH.UI

    UI.joystickX = UI.joystickX or 0
    UI.joystickY = UI.joystickY or 0
    UI.joystickActive = UI.joystickActive or false

    return UI
end

function on_create(self)
    transform = EntityTransform
    camera    = GetMainCamera()
    ensureUIShared()
end

function on_update(self,dt)
    ------------------------------------
    -- SAME CAMERA POSITIONING LOGIC AS BG
    ------------------------------------
    if not transform or not camera then return end

    local UI = ensureUIShared()

    local cx,cy,cz = camera:GetPosition()

    local SHARED = getShared()
    local yaw = SHARED.POV and SHARED.POV.yaw or 0
    local pitch = SHARED.POV and SHARED.POV.pitch or 0

    -- build forward from yaw/pitch
    local ry = math.rad(yaw)
    local rp = math.rad(pitch)
    local fx = math.cos(rp)*math.sin(ry)
    local fy = math.sin(rp)
    local fz = math.cos(rp)*math.cos(ry)

    -- right/up
    local worldUpX,worldUpY,worldUpZ = 0,1,0
    local rightX = fy*worldUpZ - fz*worldUpY
    local rightY = fz*worldUpX - fx*worldUpZ
    local rightZ = fx*worldUpY - fy*worldUpX
    local rl = math.sqrt(rightX^2 + rightY^2 + rightZ^2)
    rightX,rightY,rightZ = rightX/rl,rightY/rl,rightZ/rl
    local upX = rightY*fz - rightZ*fy
    local upY = rightZ*fx - rightX*fz
    local upZ = rightX*fy - rightY*fx

    -- anchor positioning
    local fov = camera:GetFOV()

    -- Prefer the real camera aspect if available, otherwise fall back
    local aspect = (camera.GetAspectRatio and camera:GetAspectRatio())
        or (screenWidth / screenHeight)

    local hh = UI_DEPTH * math.tan(math.rad(fov * 0.5))
    local hw = hh * aspect


    local ndcX = ANCHOR_X*2 - 1
    local ndcY = ANCHOR_Y*2 - 1

    local offR = ndcX * hw
    local offU = ndcY * hh

    local wx = cx + fx*UI_DEPTH + rightX*offR + upX*offU
    local wy = cy + fy*UI_DEPTH + rightY*offR + upY*offU
    local wz = cz + fz*UI_DEPTH + rightZ*offR + upZ*offU

    transform:SetPosition(wx,wy,wz)
    transform:SetRotation(-(pitch + QUAD_PITCH_OFFSET), yaw + QUAD_YAW_OFFSET, QUAD_ROLL_OFFSET)

    ------------------------------------
    -- JOYSTICK DRAG INPUT
    ------------------------------------
    local mx,my = Input.GetMousePosition()
    local down  = Input.GetMouseButtonDown(0)
    local held  = Input.GetMouseButton(0)
    local up    = Input.GetMouseButtonUp(0)

    local centerX = ANCHOR_X * screenWidth
    local centerY = ANCHOR_Y * screenHeight
    local hitRadius = HIT_RADIUS_NORM * math.min(screenWidth,screenHeight)

    if down then
        local dx = mx-centerX
        local dy = my-centerY
        if dx*dx+dy*dy <= hitRadius*hitRadius then
            UI.joystickActive = true
        end
    end

    if held and UI.joystickActive then
        local dx = mx-centerX
        local dy = my-centerY
        UI.joystickX = dx / hitRadius
        UI.joystickY = -dy / hitRadius

        -- normalize
        local l = math.sqrt(UI.joystickX^2 + UI.joystickY^2)
        if l>1 then
            UI.joystickX = UI.joystickX / l
            UI.joystickY = UI.joystickY / l
        end
    end

    if up then
        UI.joystickActive = false
        UI.joystickX = 0
        UI.joystickY = 0
    end
end

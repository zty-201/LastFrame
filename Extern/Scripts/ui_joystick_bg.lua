-- ============================================================
-- ui_joystick_bg.lua     (BACKGROUND ONLY)
-- ============================================================

local ANCHOR_X = 0.18
local ANCHOR_Y = 0.20
local UI_DEPTH = 5.0

local QUAD_PITCH_OFFSET = -90.0
local QUAD_YAW_OFFSET   = 0.0
local QUAD_ROLL_OFFSET  = 0.0

local screenWidth, screenHeight = 3088,1440

local transform = nil
local camera = nil

local function getShared()
    return rawget(_G, "SHARED")
end

function on_create(self)
    transform = EntityTransform
    camera    = GetMainCamera()
end

function on_update(self, dt)
    if not transform or not camera then return end

    local cx,cy,cz = camera:GetPosition()

    -- use SHARED.POV for absolutely no lag
    local SHARED = getShared()
    local yaw = SHARED.POV and SHARED.POV.yaw or 0
    local pitch = SHARED.POV and SHARED.POV.pitch or 0

    -- build direction vector from yaw/pitch
    local ry = math.rad(yaw)
    local rp = math.rad(pitch)
    local fx = math.cos(rp) * math.sin(ry)
    local fy = math.sin(rp)
    local fz = math.cos(rp) * math.cos(ry)

    -- construct right/up from forward
    local worldUpX,worldUpY,worldUpZ = 0,1,0
    local rightX = fy*worldUpZ - fz*worldUpY
    local rightY = fz*worldUpX - fx*worldUpZ
    local rightZ = fx*worldUpY - fy*worldUpX
    local rl = math.sqrt(rightX*rightX + rightY*rightY + rightZ*rightZ)
    rightX,rightY,rightZ = rightX/rl, rightY/rl, rightZ/rl
    local upX = rightY*fz - rightZ*fy
    local upY = rightZ*fx - rightX*fz
    local upZ = rightX*fy - rightY*fx

    -- screen anchoring
    local fov = camera:GetFOV()

    -- Prefer the real camera aspect if available, otherwise fall back
    local aspect = (camera.GetAspectRatio and camera:GetAspectRatio())
        or (screenWidth / screenHeight)

    local hh = UI_DEPTH * math.tan(math.rad(fov * 0.5))
    local hw = hh * aspect

    local ndcX = ANCHOR_X*2-1
    local ndcY = ANCHOR_Y*2-1

    local offR = ndcX * hw
    local offU = ndcY * hh

    local wx = cx + fx*UI_DEPTH + rightX*offR + upX*offU
    local wy = cy + fy*UI_DEPTH + rightY*offR + upY*offU
    local wz = cz + fz*UI_DEPTH + rightZ*offR + upZ*offU

    transform:SetPosition(wx,wy,wz)
    transform:SetRotation(-(pitch + QUAD_PITCH_OFFSET), yaw + QUAD_YAW_OFFSET, QUAD_ROLL_OFFSET)
end

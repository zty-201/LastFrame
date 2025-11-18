-- ============================================================
-- player_pov_camera.lua
-- Handles FPS Look (mouse/gyro) only
-- ============================================================

local mouseSensitivity = 0.2
local gyroSensitivity  = 5.0
local pitchLimit       = 89.0
local gyroEnabled      = true

local camera      = nil
local transform   = nil
local yaw, pitch  = 0.0, 0.0

-- Expose to other scripts
POV = {
    yaw = function() return yaw end,
    pitch = function() return pitch end,
    transform = function() return transform end
}

-- ============================================================
function on_create(self)
    transform = self:GetTransform()
    camera = GetMainCamera()

    if not transform then print("[POV ERROR] Transform missing") end
    if not camera then print("[POV ERROR] Camera missing") end

    print("[POV] Look system initialized")
end

-- ============================================================
local function applyLook()
    if pitch > pitchLimit then pitch = pitchLimit end
    if pitch < -pitchLimit then pitch = -pitchLimit end

    transform:SetRotation(pitch, yaw, 0)

    local x, y, z = transform:GetPosition()

    local radYaw   = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local cosPitch = math.cos(radPitch)

    local fx = cosPitch * math.sin(radYaw)
    local fy = math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    camera:SetPosition(x, y, z)
    camera:LookAt(x + fx, y + fy, z + fz)
end

-- ============================================================
function on_update(self, dt)
    if not camera or not transform then return end

    ------------------------------------------------------------
    -- WINDOWS LOOK (mouse)
    ------------------------------------------------------------
    if PLATFORM == "windows" then
        local dx, dy = Input.GetMouseDelta()
        yaw   = yaw + dx * mouseSensitivity
        pitch = pitch - dy * mouseSensitivity

        applyLook()
        return
    end

    ------------------------------------------------------------
    -- ANDROID LOOK (gyro)
    ------------------------------------------------------------
    if PLATFORM == "android" and gyroEnabled then
        local gx, gy, gz = Input.GetGyroscopeRotatioRate()

        yaw   = yaw   + math.deg(gx) * dt * gyroSensitivity
        pitch = pitch - math.deg(gy) * dt * gyroSensitivity

        applyLook()
    end
end

-- ============================================================
function on_destroy()
    print("[POV] Destroyed")
end

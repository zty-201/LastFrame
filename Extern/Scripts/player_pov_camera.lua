-- ============================================================
-- player_pov_camera.lua
-- Handles FPS Look (mouse/gyro) only
-- MUST BE LOADED FIRST (before player_movement.lua)
-- ============================================================

local mouseSensitivity = 0.08
local gyroSensitivity  = 5.0
local pitchLimit       = 89.0
local gyroEnabled      = true

local camera      = nil
local transform   = nil
local yaw, pitch  = 0.0, 0.0

-- Recenter smoothing
local recenterActive        = false
local recenterTime          = 0.0
local recenterDuration      = 0.25   -- seconds for smooth return
local recenterStartYaw      = 0.0
local recenterStartPitch    = 0.0
local recenterTargetYaw     = 0.0    -- where "centre" faces
local recenterTargetPitch   = 0.0

-- ============================================================
-- Access true global table
-- ============================================================
local function getGlobalShared()
    return rawget(_G, "SHARED")
end

-- ============================================================
function on_create(self)
    transform = EntityTransform
    camera = GetMainCamera()

    if not transform then 
        print("[player_pov_camera ERROR] Transform missing")
    end
    if not camera then 
        print("[player_pov_camera ERROR] Camera missing")
    end

    -- Initialize yaw/pitch to zero (or from some initial setting)
    -- DO NOT read from transform rotation to avoid circular dependencies
    pitch = 0.0
    yaw = 0.0

    -- Setup SHARED.POV
    local SHARED = getGlobalShared()
    if not SHARED then
        print("[player_pov_camera ERROR] Missing SHARED global!")
        return
    end

    if not SHARED.POV then
        SHARED.POV = {}
        print("[player_pov_camera] Created SHARED.POV")
    end

    SHARED.POV.yaw = yaw
    SHARED.POV.pitch = pitch

    print("[player_pov_camera] Initialized yaw=", yaw, " pitch=", pitch)
end

-- ============================================================
-- Apply rotation + smoothing
-- ============================================================
local function applyLook()
    -- Clamp pitch
    if pitch > pitchLimit then pitch = pitchLimit end
    if pitch < -pitchLimit then pitch = -pitchLimit end

    -- CRITICAL: DO NOT touch transform rotation!
    -- Only update camera position and look-at direction

    local x, y, z = transform:GetPosition()

    -- Compute forward vector from yaw/pitch
    local radYaw   = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local cosPitch = math.cos(radPitch)

    local fx = cosPitch * math.sin(radYaw)
    local fy = math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    -- -------------------------------
    -- Smoothed camera follow
    -- -------------------------------
    local smooth = 1.0

    local cx, cy, cz = camera:GetPosition()

    -- new smoothed camera position
    local nx = cx + (x - cx) * smooth
    local ny = cy + (y - cy) * smooth
    local nz = cz + (z - cz) * smooth

    camera:SetPosition(nx, ny, nz)

    -- NO smoothing on look direction - instant response to mouse/gyro input
    local tx = nx + fx
    local ty = ny + fy
    local tz = nz + fz

    camera:LookAt(tx, ty, tz)

    -- Update global yaw/pitch for movement script
    local SHARED = getGlobalShared()
    if SHARED and SHARED.POV then
        SHARED.POV.yaw = yaw
        SHARED.POV.pitch = pitch
    end
end

function on_update(self, dt)
    if not camera or not transform then return end

    local SHARED = getGlobalShared()

    -- ======================================
    -- HANDLE RECENTER REQUEST FROM UI
    -- ======================================
    if SHARED and SHARED.UI and SHARED.UI.recenterRequested then
        -- Start a smooth recenter animation from current yaw/pitch
        recenterActive      = true
        recenterTime        = 0.0
        recenterStartYaw    = yaw
        recenterStartPitch  = pitch
        recenterTargetYaw   = 0.0   -- change these if your "centre" is different
        recenterTargetPitch = 0.0

        SHARED.UI.recenterRequested = false
    end

    -- ======================================
    -- WINDOWS LOOK CONTROL
    -- ======================================
    if PLATFORM == "windows" then

        if recenterActive then
            -- Progress smoothing
            recenterTime = recenterTime + dt
            local t = recenterTime / recenterDuration
            if t >= 1.0 then
                t = 1.0
                recenterActive = false
            end

            -- Smoothstep (nicer than linear)
            local s = t * t * (3.0 - 2.0 * t)

            yaw   = recenterStartYaw   + (recenterTargetYaw   - recenterStartYaw)   * s
            pitch = recenterStartPitch + (recenterTargetPitch - recenterStartPitch) * s

            applyLook()
            return
        end

        -- Normal mouse look when not recentering
        local dx, dy = Input.GetMouseDelta()
        if dx ~= 0 or dy ~= 0 then
            yaw   = yaw + dx * mouseSensitivity
            pitch = pitch - dy * mouseSensitivity
        end

        applyLook()
        return
    end

    -- ======================================
    -- ANDROID GYRO LOOK
    -- ======================================
    if PLATFORM == "android" and gyroEnabled then

        if recenterActive then
            recenterTime = recenterTime + dt
            local t = recenterTime / recenterDuration
            if t >= 1.0 then
                t = 1.0
                recenterActive = false
            end

            local s = t * t * (3.0 - 2.0 * t)

            yaw   = recenterStartYaw   + (recenterTargetYaw   - recenterStartYaw)   * s
            pitch = recenterStartPitch + (recenterTargetPitch - recenterStartPitch) * s

            applyLook()
            return
        end

        local gx, gy, gz = Input.GetGyroscopeRotatioRate()

        yaw   = yaw   + math.deg(gx) * dt * gyroSensitivity
        pitch = pitch - math.deg(gy) * dt * gyroSensitivity

        applyLook()
    end
end

-- ============================================================
function on_destroy()
    print("[player_pov_camera] Destroyed (entity " .. tostring(EntityID) .. ")")
end
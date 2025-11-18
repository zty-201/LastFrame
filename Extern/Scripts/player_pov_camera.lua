-- ============================================================
-- player_pov_camera.lua
-- Handles FPS Look (mouse/gyro) only
-- MUST BE LOADED FIRST (before player_movement.lua)
-- ============================================================

local mouseSensitivity = 0.2
local gyroSensitivity  = 5.0
local pitchLimit       = 89.0
local gyroEnabled      = true

local camera      = nil
local transform   = nil
local yaw, pitch  = 0.0, 0.0

-- ============================================================
-- Access the REAL global SHARED table (not environment-local)
-- Use rawget to bypass the environment and access true globals
-- ============================================================
local function getGlobalShared()
    return rawget(_G, "SHARED")
end

-- ============================================================
function on_create(self)
    -- Get components from injected entity data
    transform = EntityTransform
    camera = GetMainCamera()

    if not transform then 
        print("[player_pov_camera ERROR] Transform missing for entity " .. tostring(EntityID))
    else
        print("[player_pov_camera] Transform bound successfully for entity " .. tostring(EntityID))
        local x, y, z = transform:GetPosition()
        print("[player_pov_camera] Position: " .. tostring(x) .. ", " .. tostring(y) .. ", " .. tostring(z))
    end
    
    if not camera then 
        print("[player_pov_camera ERROR] Camera missing") 
    else
        print("[player_pov_camera] Camera bound successfully")
    end

    -- Initialize yaw and pitch from current transform rotation
    local rx, ry, rz = transform:GetRotation()
    pitch = rx
    yaw = ry

    -- Access the true global SHARED table
    local SHARED = getGlobalShared()
    if not SHARED then
        print("[player_pov_camera ERROR] Global SHARED table not found!")
        return
    end
    
    -- Initialize POV in the global SHARED table
    if not SHARED.POV then
        SHARED.POV = {}
        print("[player_pov_camera] Created SHARED.POV table")
    end
    
    SHARED.POV.yaw = yaw
    SHARED.POV.pitch = pitch

    print("[player_pov_camera] Look system initialized (yaw=" .. tostring(yaw) .. ", pitch=" .. tostring(pitch) .. ")")
    print("[player_pov_camera] Global POV table created for other scripts")
end

-- ============================================================
local function applyLook()
    -- Clamp pitch
    if pitch > pitchLimit then pitch = pitchLimit end
    if pitch < -pitchLimit then pitch = -pitchLimit end

    -- Update transform rotation
    transform:SetRotation(pitch, yaw, 0)

    local x, y, z = transform:GetPosition()

    -- Calculate forward vector
    local radYaw   = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local cosPitch = math.cos(radPitch)

    local fx = cosPitch * math.sin(radYaw)
    local fy = math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    ------------------------------------------------------------------
    -- CAMERA SMOOTHING (LERP)
    ------------------------------------------------------------------
    local smooth = 0.15 -- Lower = smoother, higher = snappier

    local cx, cy, cz = camera:GetPosition()

    -- Smooth camera follow
    local nx = cx + (x - cx) * smooth
    local ny = cy + (y - cy) * smooth
    local nz = cz + (z - cz) * smooth
    camera:SetPosition(nx, ny, nz)

    -- Smooth LookAt target
    local tx = x + fx
    local ty = y + fy
    local tz = z + fz

    local sx = cx + (tx - cx) * smooth
    local sy = cy + (ty - cy) * smooth
    local sz = cz + (tz - cz) * smooth

    camera:LookAt(sx, sy, sz)
    
    ------------------------------------------------------------------

    -- Update SHARED POV
    local SHARED = getGlobalShared()
    if SHARED and SHARED.POV then
        SHARED.POV.yaw = yaw
        SHARED.POV.pitch = pitch
    end
end


-- ============================================================
function on_update(self, dt)
    if not camera or not transform then 
        if math.random() < 0.01 then
            print("[player_pov_camera] Skipping update - camera or transform is nil")
        end
        return 
    end

    -- Rest of your update code stays the same...
    if PLATFORM == "windows" then
        local dx, dy = Input.GetMouseDelta()
        
        if math.abs(dx) > 0.001 or math.abs(dy) > 0.001 then
            yaw   = yaw + dx * mouseSensitivity
            pitch = pitch - dy * mouseSensitivity
            applyLook()
        end
        return
    end

    if PLATFORM == "android" and gyroEnabled then
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
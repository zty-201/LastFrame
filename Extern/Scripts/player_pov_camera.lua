-- ============================================================
-- player_pov_camera.lua
-- Handles FPS Look (mouse/gyro) only
-- MUST BE LOADED FIRST (before player_movement.lua)
-- ============================================================

local mouseSensitivity = 0.08
local gyroSensitivity  = 1.0
local pitchLimit       = 89.0
local gyroEnabled      = true

local camera      = nil
local transform   = nil
local yaw, pitch  = 0.0, 0.0
local lensMenuName = "DirtyLens"
local lensVisible  = false
local lensSuppressed = false -- true when player manually hides the overlay

-- Camera shake
local shakeTimer       = 0.0
local shakeDuration    = 0.0
local shakeAmplitude   = 0.0
local shakeSpeed       = 0.0
local shakeOffsetYaw   = 0.0
local shakeOffsetPitch = 0.0
local shakeTickTimer   = 0.0
local SHAKE_TICK       = 0.04

local function tickShake(dt)
    if shakeTimer >= shakeDuration then
        shakeOffsetYaw = 0.0
        shakeOffsetPitch = 0.0
        return
    end

    shakeTimer     = shakeTimer + dt
    shakeTickTimer = shakeTickTimer + dt

    local t = shakeTimer / shakeDuration
    if t > 1.0 then t = 1.0 end

    local decay = 1.0 - t * t

    if shakeTickTimer >= shakeSpeed then
        shakeTickTimer = 0.0
        shakeOffsetYaw   = (math.random() * 2.0 - 1.0) * shakeAmplitude * decay * 0.7
        shakeOffsetPitch = (math.random() * 2.0 - 1.0) * shakeAmplitude * decay * 0.7
    end
end

-- Camera pull (toward lock target during shake)
local pullActive   = false
local pullDuration = 0.0
local pullTimer    = 0.0
local pullAmount   = 0.0
local pullStartX,  pullStartY,  pullStartZ  = 0.0, 0.0, 0.0
local pullTargetX, pullTargetY, pullTargetZ = 0.0, 0.0, 0.0
-- Holds the last pulled position so camera stays there after pull ends (while still locked)
local lockedCamX, lockedCamY, lockedCamZ   = nil, nil, nil

-- Recenter smoothing
local recenterActive        = false
local recenterTime          = 0.0
local recenterDuration      = 0.25   -- seconds for smooth return
local recenterStartYaw      = 0.0
local recenterStartPitch    = 0.0
local recenterTargetYaw     = 0.0    -- where "centre" faces
local recenterTargetPitch   = 0.0

-- FPS mouse look (desktop)
local holdToLook   = true
local lookButton   = GLFW_MOUSE_BUTTON_RIGHT
local prevLookHeld = false

-- ============================================================
-- Gyro smoothing: dead zone + exponential moving average (EMA)
--
-- Two-stage pipeline:
--   1. Dead zone: readings below GYRO_DEAD_ZONE rad/s are clamped to zero.
--      The threshold is subtracted from surviving values so there is no
--      sudden jump at the boundary.
--   2. EMA: blends the dead-zoned value toward the accumulator each frame.
--      This cuts out the remaining frame-to-frame variance without
--      introducing any trigonometric functions.
--
-- Tuning:
--   GYRO_DEAD_ZONE  raise if still jittery at rest (try 0.03), lower if
--                   small intentional tilts feel unresponsive (try 0.01)
--   GYRO_ALPHA      0.10 = very smooth/laggy, 0.25 = balanced, 0.40 = snappy
-- ============================================================
local GYRO_DEAD_ZONE = 0.02
local GYRO_ALPHA     = 0.20

local smoothedGx = 0.0
local smoothedGy = 0.0

local function applyDeadZone(rate)
    local sign = 1.0
    if rate < 0.0 then sign = -1.0 end
    local abs = rate * sign
    if abs < GYRO_DEAD_ZONE then
        return 0.0
    end
    -- Subtract dead zone so output starts from 0, no pop at threshold
    return sign * (abs - GYRO_DEAD_ZONE)
end

-- ============================================================
-- Access true global table
-- ============================================================
local function getGlobalShared()
    return rawget(_G, "SHARED")
end

local function setMouseLookActive(active)
    Input.SetCursorLocked(active)       -- FPS: lock + unlimited deltas
    Input.SetCursorVisible(not active)  -- hide cursor while locked
end

local function syncLensOverlayState()
    if UI_IsMenuActive == nil then
        return
    end

    local actualActive = UI_IsMenuActive(lensMenuName)
    if lensVisible and not actualActive then
        lensVisible = false
        lensSuppressed = true
    elseif (not lensVisible) and actualActive then
        lensVisible = true
        lensSuppressed = false
    end
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

    pitch = 0.0
    yaw   = 0.0

    -- Setup SHARED.POV
    local SHARED = rawget(_G, "SHARED")
    if not SHARED then
        print("[player_pov_camera ERROR] Missing SHARED global!")
        return
    end

    if not SHARED.POV then
        SHARED.POV = {}
        print("[player_pov_camera] Created SHARED.POV")
    end

    SHARED.POV.yaw    = yaw
    SHARED.POV.pitch  = pitch
    SHARED.POV.cam    = camera
    SHARED.POV.locked = false

    print("[player_pov_camera] Initialized. yaw=", yaw, " pitch=", pitch)
end

-- ============================================================
-- Apply rotation + smoothing
-- ============================================================
local function applyLook(overrideX, overrideY, overrideZ)
    -- Clamp base pitch first
    if pitch > pitchLimit then pitch = pitchLimit end
    if pitch < -pitchLimit then pitch = -pitchLimit end

    local finalYaw = yaw + shakeOffsetYaw
    local finalPitch = pitch + shakeOffsetPitch

    if finalPitch > pitchLimit then finalPitch = pitchLimit end
    if finalPitch < -pitchLimit then finalPitch = -pitchLimit end

    if Transform_SetRotation then
        Transform_SetRotation(EntityID, 0.0, finalYaw, 0.0)
    else
        transform:SetRotation(0.0, finalYaw, 0.0)
    end

    -- Compute forward vector
    local radYaw   = math.rad(finalYaw)
    local radPitch = math.rad(finalPitch)
    local cosPitch = math.cos(radPitch)

    local fx = cosPitch * math.sin(radYaw)
    local fy = math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    -- Use override position if provided (pull sequence), otherwise follow transform
    local nx, ny, nz
    if overrideX then
        nx, ny, nz = overrideX, overrideY, overrideZ
    else
        local x, y, z = transform:GetPosition()
        local smooth = 1.0
        local cx, cy, cz = camera:GetPosition()
        nx = cx + (x - cx) * smooth
        ny = cy + (y - cy) * smooth
        nz = cz + (z - cz) * smooth
    end

    camera:SetPosition(nx, ny, nz)
    camera:LookAt(nx + fx, ny + fy, nz + fz)

    -- Update SHARED.POV every frame
    local SHARED = rawget(_G, "SHARED")
    if SHARED and SHARED.POV then
        SHARED.POV.yaw   = yaw
        SHARED.POV.pitch = pitch
        SHARED.POV.cam   = camera
    end
end

function on_update(self, dt)
    if not camera or not transform then return end


    local state_name = StateMachine_GetCurrentStateName()
    if state_name ~= "Gameplay" then
        return
    end
    

    local SHARED = getGlobalShared()

    -- ============================================================
    -- MENU CAMERA MODE
    -- When main menu is active, do NOT let the player camera fight
    -- with the dedicated MainMenuCamera entity.
    -- ============================================================
    if _G.GAME_STATE == "MENU" then
        return
    end

    -- ======================================
    -- FREE SHAKE (no camera lock required)
    -- ======================================
    if SHARED and SHARED.POV and SHARED.POV.freeShake then
        SHARED.POV.freeShake = false
        shakeTimer     = 0.0
        shakeTickTimer = 0.0
        shakeDuration  = SHARED.POV.shakeDuration  or 0.8
        shakeAmplitude = SHARED.POV.shakeAmplitude or 6.0
        shakeSpeed     = SHARED.POV.shakeSpeed     or 0.04
        shakeOffsetYaw   = 0.0
        shakeOffsetPitch = 0.0
    end

    -- ======================================
    -- HANDLE RECENTER REQUEST FROM UI
    -- ======================================
    if SHARED and SHARED.UI and SHARED.UI.recenterRequested then
        recenterActive      = true
        recenterTime        = 0.0
        recenterStartYaw    = yaw
        recenterStartPitch  = pitch
        recenterTargetYaw   = 0.0
        recenterTargetPitch = 0.0

        SHARED.UI.recenterRequested = false
    end

    -- ======================================
    -- CAMERA LOCK (scare sequences)
    -- ======================================
    if SHARED and SHARED.POV and SHARED.POV.locked then
        local targetEntityID = SHARED.POV.lockTargetEntity
        if targetEntityID then
            local targetTr = Query.GetTransform(targetEntityID)
            if targetTr and transform then
                local tx, ty, tz = targetTr:GetPosition()
                local px, py, pz = transform:GetPosition()

                local dx = tx - px
                local dy = (ty + 60.0) - py
                local dz = tz - pz
                local hlen = math.sqrt(dx * dx + dz * dz)

                local targetYaw   = math.deg(math.atan(dx, dz))
                local targetPitch = math.deg(math.atan(dy, hlen))

                local speed = 5.0
                local function lerpAngle(a, b, t)
                    local diff = (b - a + 540.0) % 360.0 - 180.0
                    return a + diff * t
                end
                yaw   = lerpAngle(yaw,   targetYaw,   math.min(speed * dt, 1.0))
                pitch = lerpAngle(pitch, targetPitch, math.min(speed * dt, 1.0))
            end
        end

        if SHARED.POV.shake then
            SHARED.POV.shake = false
            shakeTimer     = 0.0
            shakeTickTimer = 0.0
            shakeDuration  = SHARED.POV.shakeDuration  or 1.2
            shakeAmplitude = SHARED.POV.shakeAmplitude or 8.0
            shakeSpeed     = SHARED.POV.shakeSpeed     or 0.04
            shakeOffsetYaw   = 0.0
            shakeOffsetPitch = 0.0

            pullActive   = true
            pullTimer    = 0.0
            pullDuration = shakeDuration
            pullAmount   = SHARED.POV.shakePullAmount or 150.0
            local px, py, pz = transform:GetPosition()
            pullStartX, pullStartY, pullStartZ = px, py, pz

            if targetEntityID then
                local tTr = Query.GetTransform(targetEntityID)
                if tTr then
                    local tx, ty, tz = tTr:GetPosition()
                    ty = ty + 80.0
                    local dx, dy, dz = tx - px, ty - py, tz - pz
                    local len = math.sqrt(dx*dx + dy*dy + dz*dz)
                    if len > 0.001 then
                        local move = math.min(pullAmount, len * 0.96)
                        pullTargetX = px + (dx/len) * move
                        pullTargetY = py + (dy/len) * move
                        pullTargetZ = pz + (dz/len) * move
                    else
                        pullTargetX, pullTargetY, pullTargetZ = px, py, pz
                    end
                end
            end
        end

        tickShake(dt)

        -- Drive the pull animation toward the lock target
        if pullActive then
            pullTimer = pullTimer + dt
            local t   = math.min(pullTimer / pullDuration, 1.0)
            local ease = t * t * (3.0 - 2.0 * t)
            lockedCamX = pullStartX + (pullTargetX - pullStartX) * ease
            lockedCamY = pullStartY + (pullTargetY - pullStartY) * ease
            lockedCamZ = pullStartZ + (pullTargetZ - pullStartZ) * ease
            if t >= 1.0 then
                pullActive = false
            end
        end

        if lockedCamX then
            applyLook(lockedCamX, lockedCamY, lockedCamZ)
        else
            applyLook()
        end

        return
    end

    -- ======================================
    -- WINDOWS LOOK CONTROL
    -- ======================================
    if PLATFORM == "windows" then
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

            tickShake(dt)
            applyLook()
            return
        end

        local lookHeld = true
        if holdToLook then
            lookHeld = Input.GetMouseButton(lookButton)
        end

        if lookHeld ~= prevLookHeld then
            setMouseLookActive(lookHeld)
            prevLookHeld = lookHeld
        end

        if lookHeld then
            local dx, dy = Input.GetMouseDelta()
            yaw   = yaw + dx * mouseSensitivity
            pitch = pitch - dy * mouseSensitivity
        end

        tickShake(dt)
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

        local dzGx = applyDeadZone(gx)
        local dzGy = applyDeadZone(gy)

        smoothedGx = smoothedGx + (dzGx - smoothedGx) * GYRO_ALPHA
        smoothedGy = smoothedGy + (dzGy - smoothedGy) * GYRO_ALPHA

        yaw   = yaw   + math.deg(smoothedGx) * dt * gyroSensitivity
        pitch = pitch - math.deg(smoothedGy) * dt * gyroSensitivity

        tickShake(dt)
        applyLook()
    end
end

-- ============================================================
function on_destroy()
    local SHARED = getGlobalShared()
    if SHARED and SHARED.POV then
        SHARED.POV.locked           = false
        SHARED.POV.lockTargetEntity = nil
    end
    lockedCamX, lockedCamY, lockedCamZ = nil, nil, nil
    if PLATFORM == "windows" then
        Input.SetCursorLocked(false)
        Input.SetCursorVisible(true)
    end
    print("[player_pov_camera] Destroyed (entity " .. tostring(EntityID) .. ")")
end
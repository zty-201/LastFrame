-- ============================================================
-- camera_touch_control.lua (FIXED VERSION)
-- Author: Teng Jun Rong Jerron
-- Date: October 2025
-- 
-- Rotates camera around a fixed pivot point when dragging.
-- Works on both Desktop (mouse) and Android (touch).
-- ============================================================

-- Simple camera control configuration
local sensitivity = 0.25   -- rotation speed multiplier
local gyroSensitivity = 5000.0
local pitchLimit = 89.0    -- to prevent flipping
local yaw = 0.0
local pitch = 0.0
local distance = 5.0       -- fixed distance from target
local target = { x = 0.0, y = 0.0, z = 0.0 } -- camera look-at point
local heightOffset = 1.2

-- Gryoscope config
local GYRO_SENSITIVITY = 15.0
local GYRO_ACTIVE = false; --flag to track if device has gyroscope
local RAD_TO_DEG = 57.29578

-- Camera reference (provided by engine binding)
local camera = nil

-- State tracking
local dragging = false
local lastX, lastY = 0.0, 0.0

function on_create()
    --print("[camera_touch_control.lua] Initialized")
    camera = GetMainCamera()
    if camera then
        --print("[Lua] Camera successfully retrieved")
    else
        --print("[Lua] ERROR: GetMainCamera() returned nil!")
    end
end

local counter = 0
function on_update(self, dt)
    --print(string.format("[Lua] Current dt: %.6f", dt))
    counter = counter + 1
    
    if not camera then return end

    if not camera then 
        if counter % 60 == 0 then
            --print("[Lua] ERROR: Camera is nil, skipping update")
        end
        return 
    end

    -- Handle recenter request from UI
    local SHARED = rawget(_G, "SHARED")
    if SHARED and SHARED.UI and SHARED.UI.recenterRequested then
        -- Reset yaw/pitch to zero
        yaw = 0.0
        pitch = 0.0

        -- Reposition camera instantly
        local radYaw   = math.rad(yaw)
        local radPitch = math.rad(pitch)
        local cosPitch = math.cos(radPitch)

        local x = target.x + distance * cosPitch * math.sin(radYaw)
        local y = (target.y + heightOffset) + distance * math.sin(radPitch)
        local z = target.z + distance * cosPitch * math.cos(radYaw)

        camera:SetPosition(x, y, z)
        camera:LookAt(target.x, target.y + heightOffset, target.z)

        -- Also update SHARED.POV for flashlight / movement
        SHARED.POV = SHARED.POV or {}
        SHARED.POV.yaw   = yaw
        SHARED.POV.pitch = pitch

        -- Clear the one-frame flag
        SHARED.UI.recenterRequested = false
    end

    -- ---Input Accumulation Initialization ---
    local deltaYaw = 0.0
    local deltaPitch = 0.0

    -- Use the Input table directly (NOT Input())
    local mouseDown = Input.GetMouseButtonDown(0)
    local mx, my = Input.GetMousePosition()
    local dx, dy = Input.GetMouseDelta()
    
    -- Only log when there's actual input
    if mouseDown or (dx ~= 0 or dy ~= 0) then
        --print(string.format("[Lua] Input: mouseDown=%s Pos=(%.1f, %.1f) Delta=(%.3f, %.3f)", 
            --tostring(mouseDown), mx, my, dx, dy))
    end

    -- ============================================================
    -- 1. GYROSCOPE CONTROL (Runs only on Android AND when NOT dragging)
    -- ============================================================
    local gyroRateX, gyroRateY, gyroRateZ = Input.GetGyroRotationRate()

    -- Check for Android/Gyroscope availability
    if (gyroRateX or 0) ~= 0 or (gyroRateY or 0) ~= 0 or (gyroRateZ or 0) ~= 0 then
        GYRO_ACTIVE = true
    end
    
    -- Gyroscope logic takes effect if active and the screen is not being touched.
    if GYRO_ACTIVE and not isDraggingTouch then
        -- Gyro data is rad/s. Multiply by dt to get the displacement in radians.
        -- We multiply by GYRO_SENSITIVITY and RAD_TO_DEG to get a usable degree delta.
        
        -- Gyro Y-rate (Rotation around Y-axis) affects Yaw
        local gyroYawDelta = -gyroRateY * dt * GYRO_SENSITIVITY
        
        -- Gyro X-rate (Rotation around X-axis) affects Pitch
        local gyroPitchDelta = -gyroRateX * dt * GYRO_SENSITIVITY

        yaw = yaw + gyroYawDelta
        pitch = pitch + gyroPitchDelta
        
        -- Clear the delta to prevent touch drag from being processed below
        dx, dy = 0, 0 
    end
    
    -- ============================================================
    -- 2. MOUSE/TOUCH DELTA CONTROL (Takes priority when touching)
    -- ============================================================
    if mouseDown then
        dragging = true
        lastX, lastY = mx, my
        --print("[Lua] Started dragging")
    end
    
    -- Handle drag motion
    if dragging and Input.GetMouseButton(0) then
        local deltaX = mx - lastX
        local deltaY = my - lastY

        if deltaX ~= 0 or deltaY ~= 0 then
            -- Invert rotation direction here 👇
            yaw = yaw - deltaX * sensitivity
            pitch = pitch + deltaY * sensitivity

            -- Clamp pitch
            if pitch > pitchLimit then pitch = pitchLimit end
            if pitch < -pitchLimit then pitch = -pitchLimit end

            --print(string.format("[Lua] Camera rotation: yaw=%.2f pitch=%.2f", yaw, pitch))

            -- Update camera position (spherical to Cartesian)
            local radYaw = math.rad(yaw)
            local radPitch = math.rad(pitch)

            local x = target.x + distance * math.cos(radPitch) * math.sin(radYaw)
            local y = target.y + distance * math.sin(radPitch)
            local z = target.z + distance * math.cos(radPitch) * math.cos(radYaw)

            camera:SetPosition(x, y, z)
            camera:LookAt(target.x, target.y + heightOffset, target.z)
        end

        lastX, lastY = mx, my
    end

    -- Handle release
    if dragging and not isDraggingTouch then
        dragging = false
        --print("[Lua] Stopped dragging")
    end

    -- --- GYROSCOPE CONTROL ---
    -- Only apply gyroscope movement if the user is not dragging the screen
    if not dragging then
        local gyroX, gyroY, gyroZ = Input.GetGyroscopeRotatioRate()
        
        -- Log the raw gyro data that Lua sees (must match C++ logs)
       -- print(string.format("[Lua] RAW Gyro: X=%.4f Y=%.4f", gyroX, gyroY))

        -- Calculate intermediate deltas
        local tempYaw = -math.deg(gyroY) * dt * gyroSensitivity 
        local tempPitch = -math.deg(gyroX) * dt * gyroSensitivity
        
        -- Log the calculated rotation from the gyroscope
        --print(string.format("[Lua] Gyro Delta Calc: dY=%.6f dP=%.6f", tempYaw, tempPitch))
        
        deltaYaw = deltaYaw + tempYaw
        deltaPitch = deltaPitch + tempPitch
    end

    -- --- APPLY ROTATION (UNIFIED) ---
    if deltaYaw ~= 0 or deltaPitch ~= 0 then
        -- Log the calculated rotation from both touch/mouse AND gyro
        --print(string.format("[Lua] Final Delta Applied: dY=%.4f dP=%.4f", deltaYaw, deltaPitch))
        
        -- 🌟 Apply accumulated local change to the GLOBAL variables 🌟
        yaw = yaw + deltaYaw
        pitch = pitch + deltaPitch
        -- -----------------------------------------------------------
        
        -- Clamp pitch
        if pitch > pitchLimit then pitch = pitchLimit end
        if pitch < -pitchLimit then pitch = -pitchLimit end
        
        -- Update camera position (Convert spherical coordinates to Cartesian)
        local radYaw = math.rad(yaw)
        local radPitch = math.rad(pitch)
        
        local cosPitch = math.cos(radPitch)
        local x = target.x + distance * cosPitch * math.sin(radYaw)
        local y = target.y + distance * math.sin(radPitch)
        local z = target.z + distance * cosPitch * math.cos(radYaw)
        
        camera:SetPosition(x, y, z)
        camera:LookAt(target.x, target.y + heightOffset, target.z)
    end

        -- --- UPDATE SHARED POV (for flashlight, UI, movement) ---
        local SHARED = rawget(_G, "SHARED") or {}
        _G.SHARED = SHARED

        SHARED.POV = SHARED.POV or {}
        SHARED.POV.yaw   = yaw
        SHARED.POV.pitch = pitch
    end
end

function on_destroy()
    --print("[camera_touch_control.lua] Destroyed")
end
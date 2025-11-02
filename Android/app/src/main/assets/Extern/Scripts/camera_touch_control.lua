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
local pitchLimit = 89.0    -- to prevent flipping
local yaw = 0.0
local pitch = 0.0
local distance = 5.0       -- fixed distance from target
local target = { x = 0.0, y = 0.0, z = 0.0 } -- camera look-at point

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
    print("[camera_touch_control.lua] Initialized")
    camera = GetMainCamera()
    if camera then
        print("[Lua] Camera successfully retrieved")
    else
        print("[Lua] ERROR: GetMainCamera() returned nil!")
    end
end

local counter = 0
function on_update(self, dt)
    counter = counter + 1
    
    if not camera then return end

    local mx, my = Input.GetMousePosition()
    local dx, dy = Input.GetMouseDelta()
    
    local isDraggingTouch = Input.GetMouseButton(0)
    local mouseDown = Input.GetMouseButtonDown(0)

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
        print("[Lua] Started dragging")
    end
    
    if dragging and isDraggingTouch then
        local deltaX = mx - lastX
        local deltaY = my - lastY
        
        if deltaX ~= 0 or deltaY ~= 0 then
            -- Accumulate rotation based on pixel delta and touch sensitivity
            yaw = yaw + deltaX * sensitivity
            pitch = pitch - deltaY * sensitivity
            
            print(string.format("[Lua] Touch rotation: yaw=%.2f pitch=%.2f", yaw, pitch))
        end
        
        lastX, lastY = mx, my
    end
    
    -- Handle release
    if dragging and not isDraggingTouch then
        dragging = false
        print("[Lua] Stopped dragging")
    end
    
    -- ============================================================
    -- 3. FINAL CAMERA UPDATE
    -- ============================================================

    -- Clamp pitch (Necessary after both touch and gyro updates)
    if pitch > pitchLimit then pitch = pitchLimit end
    if pitch < -pitchLimit then pitch = -pitchLimit end

    -- Apply rotation: Convert spherical coordinates to Cartesian
    local radYaw = math.rad(yaw)
    local radPitch = math.rad(pitch)
    
    local x = target.x + distance * math.cos(radPitch) * math.sin(radYaw)
    local y = target.y + distance * math.sin(radPitch)
    local z = target.z + distance * math.cos(radPitch) * math.cos(radYaw)
    
    camera:SetPosition(x, y, z)
    camera:LookAt(target.x, target.y, target.z)
end

function on_destroy()
    print("[camera_touch_control.lua] Destroyed")
end
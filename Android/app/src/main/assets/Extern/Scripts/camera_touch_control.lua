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
    
    -- Only print every 60 frames to avoid spam
    if counter % 60 == 0 then
        --print(string.format("[Lua] on_update frame=%d dt=%.4f", counter, dt))
    end

    if not camera then 
        if counter % 60 == 0 then
            print("[Lua] ERROR: Camera is nil, skipping update")
        end
        return 
    end

    -- Use the Input table directly (NOT Input())
    local mouseDown = Input.GetMouseButtonDown(0)
    local mx, my = Input.GetMousePosition()
    local dx, dy = Input.GetMouseDelta()
    
    -- Only log when there's actual input
    if mouseDown or (dx ~= 0 or dy ~= 0) then
        print(string.format("[Lua] Input: mouseDown=%s Pos=(%.1f, %.1f) Delta=(%.3f, %.3f)", 
            tostring(mouseDown), mx, my, dx, dy))
    end

    -- Handle touch/mouse down
    if mouseDown then
        dragging = true
        lastX, lastY = mx, my
        print("[Lua] Started dragging")
    end
    
    -- Handle drag motion
    if dragging and Input.GetMouseButton(0) then
        local deltaX = mx - lastX
        local deltaY = my - lastY
        
        if deltaX ~= 0 or deltaY ~= 0 then
            yaw = yaw + deltaX * sensitivity
            pitch = pitch - deltaY * sensitivity
            
            -- Clamp pitch
            if pitch > pitchLimit then pitch = pitchLimit end
            if pitch < -pitchLimit then pitch = -pitchLimit end
            
            print(string.format("[Lua] Camera rotation: yaw=%.2f pitch=%.2f", yaw, pitch))
            
            -- Update camera position (implement your rotation logic here)
            -- Example: Convert spherical coordinates to Cartesian
            local radYaw = math.rad(yaw)
            local radPitch = math.rad(pitch)
            
            local x = target.x + distance * math.cos(radPitch) * math.sin(radYaw)
            local y = target.y + distance * math.sin(radPitch)
            local z = target.z + distance * math.cos(radPitch) * math.cos(radYaw)
            
            camera:SetPosition(x, y, z)
            camera:LookAt(target.x, target.y, target.z)
        end
        
        lastX, lastY = mx, my
    end
    
    -- Handle release
    if dragging and not Input.GetMouseButton(0) then
        dragging = false
        print("[Lua] Stopped dragging")
    end
end

function on_destroy()
    print("[camera_touch_control.lua] Destroyed")
end
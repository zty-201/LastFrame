-- ============================================================
-- flashlight_controller.lua
-- Author: Benjamin Lau Rey Jun
-- Date: October 2025
-- 
-- Holds flashlight cone info in SHARED.FLASHLIGHT
-- and provides Flashlight_Toggle() for UI.
-- Attach this to the same entity as player_pov_camera.
-- ============================================================

-- ============================================================
-- flashlight_controller.lua
-- Attach this directly to your separate Flashlight Entity!
-- ============================================================

local camera = nil
local isOn = false
local baseIntensity = 10000.0

-- Helper to safely get the SHARED table (used to grab player Yaw/Pitch)
local function getGlobalShared()
    return rawget(_G, "SHARED")
end

function on_create(self)
    camera = GetMainCamera()

    if EntityLight then
        -- Store the intensity you set in the editor Inspector
        local current = EntityLight:GetIntensity()
        if current > 0.0 then
            baseIntensity = current
        end

        -- Start the flashlight OFF
        EntityLight:SetIntensity(0.0)
        isOn = false
    end
end

function Flashlight_Toggle()
    isOn = not isOn

    -- Toggle the Light Component
    if EntityLight then
        if isOn then
            EntityLight:SetIntensity(baseIntensity)
        else
            EntityLight:SetIntensity(0.0)
        end
    end

    -- Play the toggle sound using the new Audio API
    if type(Audio_PlaySlot) == "function" then
        Audio_StopSlot(EntityID, "Flashlight")
        Audio_PlaySlot(EntityID, "Flashlight")
    end
end

function on_update(self, dt)
    if not camera or not EntityTransform then return end

    -- 1. Get Base Camera Position
    local cx, cy, cz = camera:GetPosition()

    -- 2. Grab Yaw & Pitch from your Player POV script
    local SHARED = getGlobalShared()
    local yaw, pitch = 0.0, 0.0
    if SHARED and SHARED.POV then
        yaw   = SHARED.POV.yaw   or 0.0
        pitch = SHARED.POV.pitch or 0.0
    end

    -- 3. Set Rotation (Match the player's looking angle)
    EntityTransform:SetRotation(-pitch, yaw, 0.0)

    -- 4. Calculate Forward & Right vectors for offset
    local radYaw   = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local cosPitch = math.cos(radPitch)

    local fx = cosPitch * math.sin(radYaw)
    local fy = -math.sin(radPitch) 
    local fz = cosPitch * math.cos(radYaw)

    local rx = math.cos(radYaw)
    local rz = -math.sin(radYaw)

    -- 5. Apply Realistic Positional Offsets
    local forwardOffset = 15.0   -- 0.5 units in front of the camera
    local rightOffset   = 0.0   -- 0.3 units to the right (held in right hand)
    local upOffset      = 0.0  -- Slightly below eye-level

    local lx = cx + (fx * forwardOffset) + (rx * rightOffset)
    local ly = cy + (fy * forwardOffset) + upOffset
    local lz = cz + (fz * forwardOffset) + (rz * rightOffset)

    EntityTransform:SetPosition(lx, ly, lz)

    -- 6. Keyboard Toggle (Press F to toggle)
    if PLATFORM == "windows" and Input.GetKeyDown(GLFW_KEY_F) then
        Flashlight_Toggle()
    end
    
    -- 7. UI Toggle hook
    if SHARED and SHARED.FLASHLIGHT and SHARED.FLASHLIGHT.requestToggle then
        SHARED.FLASHLIGHT.requestToggle = false
        Flashlight_Toggle()
    end
end

function on_destroy()
    print("[flashlight_controller] destroyed")
end

-- ============================================================
-- flashlight_controller.lua
-- Author: Benjamin Lau Rey Jun
-- Date: October 2025
-- 
-- Holds flashlight cone info in SHARED.FLASHLIGHT
-- and provides Flashlight_Toggle() for UI.
-- Attach this to the same entity as player_pov_camera.
-- ============================================================

local camera = nil
local range   = 8.0
local fovDeg  = 40.0
local isOn    = false
local transform = nil
local light     = nil
local baseIntensity = 10000.0

-- Remember previous F state so we can detect a press
local prevFDown = false

local function getGlobalShared()
    return rawget(_G, "SHARED")
end

local function ensureSharedFlashlight()
    local SHARED = getGlobalShared()
    if not SHARED then return nil end
    SHARED.FLASHLIGHT = SHARED.FLASHLIGHT or {}
    return SHARED.FLASHLIGHT
end

function on_create(self)
    camera = GetMainCamera()
    transform = EntityTransform
    light     = EntityLight

    print("[flashlight_controller] on_create, transform=", transform, " light=", light)

    if not camera then
        print("[flashlight_controller] ERROR: no camera")
        return
    end

    local FL = ensureSharedFlashlight()
    if FL then
        FL.isOn       = isOn
        FL.range      = range
        FL.cosHalfFov = math.cos(math.rad(fovDeg * 0.5))
        FL.posX, FL.posY, FL.posZ = camera:GetPosition()
        FL.fx, FL.fy, FL.fz       = 0.0, 0.0, 1.0
    end

    -- sync Lua state with actual light state
    if light then
        -- remember whatever you set in the inspector as the "ON" value
        if light.GetIntensity then
            local current = light:GetIntensity()
            if current and current > 0.0 then
                baseIntensity = current
            end
        end

        -- start flashlight OFF
        light:SetIntensity(10000.0)
        isOn = true
        if FL then FL.isOn = true end
    end
end

-- This is what we’ll call from C++ / UI
function Flashlight_Toggle()
    local FL = ensureSharedFlashlight()
    if not FL then return end

    isOn = not isOn
    FL.isOn = isOn

    print("[flashlight_controller] Toggled flashlight, isOn=", isOn)

    if light then
        if isOn then
            light:SetIntensity(baseIntensity)
        else
            light:SetIntensity(0.0)
        end
    end

    -------------------------------------------------
    -- PLAY FLASHLIGHT TOGGLE SOUND
    -------------------------------------------------
    if type(Audio_PlaySlot) == "function" then
        
        -- Force retrigger in case player spams toggle
        Audio_StopSlot(EntityID, "Flashlight")
        Audio_PlaySlot(EntityID, "Flashlight")

        print("[FLASHLIGHT AUDIO] Played slot 'Flashlight'")
    else
        print("[FLASHLIGHT AUDIO ERROR] Audio_PlaySlot missing!")
    end
end



function on_update(self, dt)
    if not camera then return end
    local FL = ensureSharedFlashlight()
    if not FL then return end

    local cx, cy, cz = camera:GetPosition()
    FL.posX, FL.posY, FL.posZ = cx, cy, cz

    -- Read yaw/pitch from your POV script if you store them there
    local SHARED = getGlobalShared()
    local yaw, pitch = 0.0, 0.0
    if SHARED and SHARED.POV then
        yaw   = SHARED.POV.yaw   or 0.0
        pitch = SHARED.POV.pitch or 0.0
    end

    local radYaw   = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local cosPitch = math.cos(radPitch)

    -- Camera forward vector
    local fx = cosPitch * math.sin(radYaw)
    local fy = math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    -- Use the SAME forward for flashlight
    local dirX = fx
    local dirY = fy
    local dirZ = fz

    -- Store this as the cone's forward vector too (icons will use this)
    FL.fx, FL.fy, FL.fz = dirX, dirY, dirZ

    if transform and light then
        -- 1) move light to player
        local px, py, pz = transform:GetPosition()

        -- Give it a slight offset in front + up (tweak these)
        local forwardOffset = 0.5
        local upOffset      = 0

        local lx = px + dirX * forwardOffset
        local ly = py + upOffset
        local lz = pz + dirZ * forwardOffset

        light:SetPosition(lx, ly, lz)

        -- 2) aim the light forward into the world
        light:SetDirection(dirX, dirY, dirZ)
    end

    -- =========================================
    -- Keyboard toggle + AUDIO PLAY
    -- =========================================
    if PLATFORM == "windows" then
        if Input.GetKeyDown(GLFW_KEY_Q) then
            print("[FLASH] Q pressed")

            -- 1) toggle flashlight
            Flashlight_Toggle()

            -- 2) play audio ONCE (NO LOOP)
            if EntityAudio then
                EntityAudio:SetLoop(false)
                EntityAudio:RequestPlay()
                print("[FLASH] Played toggle sound")
            else
                print("[FLASH] NO AUDIO COMPONENT FOUND!")
            end
        end
    end


    -- =========================================
    -- UI toggle request (from ui_flashlight.lua)
    -- =========================================
    if FL.requestToggle then
        FL.requestToggle = false
        Flashlight_Toggle()
    end
end

function on_destroy()
    print("[flashlight_controller] destroyed")
end

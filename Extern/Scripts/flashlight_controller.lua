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

-- -----------------------------------------------
-- Battery / flicker config
-- -----------------------------------------------
local STABLE_TIME  = 15.0   -- seconds of stable light before flickering
local FLICKER_TIME = 10.0   -- seconds of flickering before auto-off

-- -----------------------------------------------
-- Runtime state
-- -----------------------------------------------
local onTimer      = 0.0    -- time since flashlight was turned on
local flickerTimer = 0.0    -- countdown to next flicker toggle
local flickerIsOn  = true   -- current flicker sub-state
local flickering   = false

local deathCount   = 0      -- how many times it has auto-died
local autoDied     = false  -- true when light is off due to battery death
local pressCount   = 0      -- F presses accumulated while trying to revive

local function required_presses()
    -- First death: 1 press. Every death after: 5 presses.
    return deathCount <= 1 and 1 or 5
end

local function get_flicker_interval()
    -- Flicker gets more frantic as the 10s window runs out
    local flickerProgress = math.min((onTimer - STABLE_TIME) / FLICKER_TIME, 1.0)
    local maxInt = 0.4 * (1.0 - flickerProgress * 0.85)   -- shrinks from 0.4 -> 0.06
    local minInt = 0.04
    maxInt = math.max(maxInt, minInt)
    return minInt + math.random() * (maxInt - minInt)
end

local function getGlobalShared()
    return rawget(_G, "SHARED")
end

-- -----------------------------------------------
-- Auto-die (called when battery runs out)
-- -----------------------------------------------
local function auto_die()
    isOn        = false
    autoDied    = true
    deathCount  = deathCount + 1
    pressCount  = 0
    onTimer     = 0.0
    flickering  = false
    flickerIsOn = true
    flickerTimer = 0.0

    if EntityLight then EntityLight:SetIntensity(0.0) end

    if type(Audio_PlaySlot) == "function" then
        Audio_StopSlot(EntityID, "Flashlight")
        Audio_PlaySlot(EntityID, "Flashlight")
    end
end

-- -----------------------------------------------
-- Revive attempt (called on F press / UI tap when off)
-- -----------------------------------------------
local function try_revive()
    pressCount = pressCount + 1
    if pressCount >= required_presses() then
        autoDied    = false
        pressCount  = 0
        isOn        = true
        onTimer     = 0.0
        flickering  = false
        flickerIsOn = true
        flickerTimer = 0.0

        if EntityLight then EntityLight:SetIntensity(baseIntensity) end

        if type(Audio_PlaySlot) == "function" then
            Audio_StopSlot(EntityID, "Flashlight")
            Audio_PlaySlot(EntityID, "Flashlight")
        end
    end
end

function on_create(self)
    camera = GetMainCamera()

    if EntityLight then
        local current = EntityLight:GetIntensity()
        if current > 0.0 then
            baseIntensity = current
        end
        EntityLight:SetIntensity(0.0)
        isOn = false
    end
end

function Flashlight_Toggle()
    if autoDied then
        try_revive()
        return
    end

    isOn = not isOn

    if EntityLight then
        EntityLight:SetIntensity(isOn and baseIntensity or 0.0)
    end

    -- Reset flicker when manually toggled
    onTimer      = 0.0
    flickering   = false
    flickerIsOn  = true
    flickerTimer = 0.0

    if type(Audio_PlaySlot) == "function" then
        Audio_StopSlot(EntityID, "Flashlight")
        Audio_PlaySlot(EntityID, "Flashlight")
    end
end

function on_update(self, dt)
    local state_name = StateMachine_GetCurrentStateName()
    if state_name ~= "Gameplay" then return end

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
    local forwardOffset = 15.0
    local rightOffset   = 0.0
    local upOffset      = 0.0

    local lx = cx + (fx * forwardOffset) + (rx * rightOffset)
    local ly = cy + (fy * forwardOffset) + upOffset
    local lz = cz + (fz * forwardOffset) + (rz * rightOffset)

    EntityTransform:SetPosition(lx, ly, lz)

    -- 6. Keyboard Toggle (Press F)
    if PLATFORM == "windows" and Input.GetKeyDown(GLFW_KEY_F) then
        if autoDied then
            try_revive()
        elseif isOn then
            Flashlight_Toggle()   -- manual off while on
        else
            Flashlight_Toggle()   -- manual on while off (not dead)
        end
    end

    -- 7. UI Toggle hook
    if SHARED and SHARED.FLASHLIGHT and SHARED.FLASHLIGHT.requestToggle then
        SHARED.FLASHLIGHT.requestToggle = false
        if autoDied then
            try_revive()
        else
            Flashlight_Toggle()
        end
    end

    -- 8. Battery lifecycle (only while on)
    if not isOn then return end

    onTimer = onTimer + dt

    -- Phase 1: stable (0 -> STABLE_TIME) — nothing to do

    -- Phase 2: flicker (STABLE_TIME -> STABLE_TIME + FLICKER_TIME)
    if onTimer >= STABLE_TIME then
        if not flickering then
            flickering   = true
            flickerTimer = get_flicker_interval()
        end

        flickerTimer = flickerTimer - dt
        if flickerTimer <= 0.0 then
            flickerIsOn  = not flickerIsOn
            flickerTimer = get_flicker_interval()
            if EntityLight then
                EntityLight:SetIntensity(flickerIsOn and baseIntensity or 0.0)
            end
        end
    end

    -- Phase 3: auto-off after full duration
    if onTimer >= STABLE_TIME + FLICKER_TIME then
        auto_die()
    end
end

function on_destroy()
    print("[flashlight_controller] destroyed")
end

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
end

-- This is what we’ll call from C++ / UI
function Flashlight_Toggle()
    local FL = ensureSharedFlashlight()
    if not FL then return end

    isOn = not isOn
    FL.isOn = isOn

    print("[flashlight_controller] Toggled flashlight, isOn=", isOn)

    -- TODO: if you have a Light component, enable/disable it here.
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

    local fx = cosPitch * math.sin(radYaw)
    local fy = math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    FL.fx, FL.fy, FL.fz = fx, fy, fz
end

function on_destroy()
    print("[flashlight_controller] destroyed")
end

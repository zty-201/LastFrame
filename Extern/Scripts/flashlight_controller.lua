-- ============================================================
-- flashlight_controller.lua
-- Author: Benjamin Lau Rey Jun
-- Date: October 2025
--
-- Depletable battery across 4 levels: FULL / 2-bar / 1-bar / EMPTY
-- Battery drains only while ON. Persists across toggles.
-- Empty = permanently off for the rest of the level.
-- ============================================================

local camera      = nil
local isOn        = false
local baseIntensity = 10000.0

-- -----------------------------------------------
-- Battery config
-- -----------------------------------------------
local STABLE_TIME  = 30.0   -- seconds at FULL before flickering starts
local FLICKER_TIME = 10.0   -- seconds of flicker before empty

-- Battery level thresholds (based on cumulative onTimer):
-- Each tier lasts STABLE_TIME seconds (equal duration).
--   FULL  (3): 0              .. STABLE_TIME
--   2-bar (2): STABLE_TIME    .. STABLE_TIME * 2
--   1-bar (1): STABLE_TIME*2  .. STABLE_TIME * 3
--   EMPTY (0): >= STABLE_TIME * 3

-- Battery icon entity IDs (in Android_Inventory_Buttons overlay)
local ICON_FULL  = 474   -- 3 bars
local ICON_2BAR  = 473   -- 2 bars
local ICON_1BAR  = 469   -- 1 bar
local ICON_EMPTY = 477   -- 0 bars / empty

-- -----------------------------------------------
-- Runtime state
-- -----------------------------------------------
local onTimer      = 0.0   -- cumulative ON-time; does NOT reset on toggle
local flickerTimer = 0.0
local flickerIsOn  = true
local flickering   = false

-- Burst / pause pattern
-- inBurst=true  → currently doing rapid flicker
-- inBurst=false → light stable, waiting for next burst
local inBurst      = false
local burstTimer   = 0.0   -- countdown for current phase
local batteryEmpty = false
local batteryLevel = 3     -- 3=full, 2=2bar, 1=1bar, 0=empty

-- -----------------------------------------------
-- Helpers
-- -----------------------------------------------
local function getGlobalShared()
    return rawget(_G, "SHARED")
end

local function get_battery_level()
    if onTimer >= STABLE_TIME * 3 then return 0 end
    if onTimer >= STABLE_TIME * 2 then return 1 end
    if onTimer >= STABLE_TIME     then return 2 end
    return 3
end

local function get_flicker_interval()
    -- Flicker spans STABLE_TIME * 2 seconds (2-bar + 1-bar zones combined).
    -- Early in that window → long intervals (subtle, 2-bar feel)
    -- Late in that window  → short intervals (rapid, 1-bar feel)
    local flickerProgress = math.min((onTimer - STABLE_TIME) / (STABLE_TIME * 2), 1.0)
    local maxInt = 0.4 * (1.0 - flickerProgress * 0.85)  -- 0.4 -> 0.06
    local minInt = 0.04
    maxInt = math.max(maxInt, minInt)
    return minInt + math.random() * (maxInt - minInt)
end

local function update_battery_icons(level)
    local mapping = {
        [3] = ICON_FULL,
        [2] = ICON_2BAR,
        [1] = ICON_1BAR,
        [0] = ICON_EMPTY,
    }
    for lvl, id in pairs(mapping) do
        local ui = GetEntityUI(id)
        if ui then ui:SetVisible(lvl == level) end
    end
end

-- -----------------------------------------------
-- Battery empty (permanent)
-- -----------------------------------------------
local function go_empty()
    isOn         = false
    batteryEmpty = true
    batteryLevel = 0
    flickering   = false
    inBurst      = false
    burstTimer   = 0.0
    flickerIsOn  = true
    flickerTimer = 0.0

    if EntityLight then EntityLight:SetIntensity(0.0) end

    if type(Audio_PlaySlot) == "function" then
        Audio_StopSlot(EntityID, "Flashlight")
        Audio_PlaySlot(EntityID, "Flashlight")
    end

    update_battery_icons(0)
    print("[flashlight_controller] Battery empty — flashlight disabled for this level")
end

-- -----------------------------------------------
-- Lifecycle
-- -----------------------------------------------
function on_create(self)
    camera = GetMainCamera()

    onTimer      = 0.0
    flickerTimer = 0.0
    flickerIsOn  = true
    flickering   = false
    batteryEmpty = false
    batteryLevel = 3

    if EntityLight then
        local current = EntityLight:GetIntensity()
        if current > 0.0 then baseIntensity = current end
        EntityLight:SetIntensity(0.0)
        isOn = false
    end

    update_battery_icons(3)
end

-- -----------------------------------------------
-- Toggle (called by key / UI button)
-- -----------------------------------------------
function Flashlight_Toggle()
    if batteryEmpty then return end   -- permanently dead, ignore

    isOn = not isOn

    if EntityLight then
        EntityLight:SetIntensity(isOn and baseIntensity or 0.0)
    end

    if isOn then
        -- Resume flickering if battery is already in the flicker zone
        flickering   = (onTimer >= STABLE_TIME)
        inBurst      = false
        burstTimer   = 1.0 + math.random() * 2.0   -- short pause before first burst on resume
        flickerIsOn  = true
        flickerTimer = 0.0
    else
        -- Turning off: freeze flicker state, light off
        flickering   = false
        flickerIsOn  = true
    end

    if type(Audio_PlaySlot) == "function" then
        Audio_StopSlot(EntityID, "Flashlight")
        Audio_PlaySlot(EntityID, "Flashlight")
    end
end

-- -----------------------------------------------
-- Update
-- -----------------------------------------------
function on_update(self, dt)
    local state_name = StateMachine_GetCurrentStateName()
    if state_name ~= "Gameplay" then return end

    if not camera or not EntityTransform then return end

    -- ── Position / rotation tracking ──────────────────────────
    local cx, cy, cz = camera:GetPosition()

    local SHARED = getGlobalShared()
    local yaw, pitch = 0.0, 0.0
    if SHARED and SHARED.POV then
        yaw   = SHARED.POV.yaw   or 0.0
        pitch = SHARED.POV.pitch or 0.0
    end

    EntityTransform:SetRotation(-pitch, yaw, 0.0)

    local radYaw   = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local cosPitch = math.cos(radPitch)

    local fx = cosPitch * math.sin(radYaw)
    local fy = -math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    local rx = math.cos(radYaw)
    local rz = -math.sin(radYaw)

    local forwardOffset = 15.0
    local rightOffset   = 0.0
    local upOffset      = 0.0

    local lx = cx + (fx * forwardOffset) + (rx * rightOffset)
    local ly = cy + (fy * forwardOffset) + upOffset
    local lz = cz + (fz * forwardOffset) + (rz * rightOffset)

    EntityTransform:SetPosition(lx, ly, lz)

    -- ── Input ─────────────────────────────────────────────────
    if PLATFORM == "windows" and Input.GetKeyDown(GLFW_KEY_F) then
        Flashlight_Toggle()
    end

    if SHARED and SHARED.FLASHLIGHT and SHARED.FLASHLIGHT.requestToggle then
        SHARED.FLASHLIGHT.requestToggle = false
        Flashlight_Toggle()
    end

    -- ── Battery drain (only while ON) ─────────────────────────
    if not isOn then return end

    onTimer = onTimer + dt

    -- Update battery level and refresh icons every frame.
    -- Refreshing every frame (not just on change) ensures icons show correctly
    -- even if the overlay wasn't open yet when on_create ran.
    local newLevel = get_battery_level()
    if newLevel ~= batteryLevel then
        batteryLevel = newLevel
        print("[flashlight_controller] Battery level ->", batteryLevel)
    end
    update_battery_icons(batteryLevel)

    -- Hit empty
    if batteryLevel == 0 then
        go_empty()
        return
    end

    -- ── Flicker (2-bar and 1-bar zones) ───────────────────────
    if onTimer >= STABLE_TIME then
        if not flickering then
            flickering  = true
            inBurst     = false
            -- Start with a pause so the first burst feels earned
            burstTimer  = 2.0 + math.random() * 3.0
            flickerIsOn = true
            if EntityLight then EntityLight:SetIntensity(baseIntensity) end
        end

        burstTimer = burstTimer - dt

        if inBurst then
            -- Rapid flicker within the burst
            flickerTimer = flickerTimer - dt
            if flickerTimer <= 0.0 then
                flickerIsOn  = not flickerIsOn
                flickerTimer = get_flicker_interval()
                if EntityLight then
                    EntityLight:SetIntensity(flickerIsOn and baseIntensity or 0.0)
                end
            end

            -- Burst over → go to pause, light back on
            if burstTimer <= 0.0 then
                inBurst     = false
                flickerIsOn = true
                if EntityLight then EntityLight:SetIntensity(baseIntensity) end

                -- Pause duration: shorter at 1-bar than 2-bar
                if batteryLevel == 2 then
                    burstTimer = 3.0 + math.random() * 3.0   -- 3-6s pause at 2-bar
                else
                    burstTimer = 1.0 + math.random() * 2.0   -- 1-3s pause at 1-bar
                end
            end
        else
            -- Pause over → start a burst
            if burstTimer <= 0.0 then
                inBurst      = true
                flickerTimer = get_flicker_interval()

                -- Burst duration: shorter at 2-bar, longer at 1-bar
                if batteryLevel == 2 then
                    burstTimer = 0.3 + math.random() * 0.5   -- 0.3-0.8s burst at 2-bar
                else
                    burstTimer = 0.5 + math.random() * 1.0   -- 0.5-1.5s burst at 1-bar
                end
            end
        end
    end
end

function on_destroy()
    print("[flashlight_controller] destroyed")
end

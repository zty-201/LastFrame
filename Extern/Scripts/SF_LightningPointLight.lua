----------------------------------------------------------------
-- SF_LightningPointLight.lua
-- Listens to rooftop lightning cue from SF_LightningSpotlight
-- and makes a point light flash bright, then fade away.
----------------------------------------------------------------

local DEBUG = false

-- =========================
-- CONFIG
-- =========================

local LIGHTNING_TABLE = "LIGHTNING_CMD"
local ROOFTOP_KEY = "ROOFTOP_LIGHTNING"

local OFF_INTENSITY   = 0.0
local FLASH_INTENSITY = 300000.0

local FLASH_HOLD_TIME = 0.3
local FLASH_FADE_TIME = 0.6

-- =========================
-- INTERNAL STATE
-- =========================

local STATE_IDLE = 0
local STATE_HOLD = 1
local STATE_FADE = 2

local state = STATE_IDLE
local timer = 0.0
local duration = 0.0
local startIntensity = FLASH_INTENSITY

local lastProcessedEventId = -1

------------------------------------------------

local function set_intensity(v)
    if EntityLight ~= nil then
        EntityLight:SetIntensity(v)
    end
end

local function begin_idle()
    state = STATE_IDLE
    timer = 0.0
    duration = 0.0
    set_intensity(OFF_INTENSITY)
end

local function begin_hold()
    state = STATE_HOLD
    timer = 0.0
    duration = FLASH_HOLD_TIME
    startIntensity = FLASH_INTENSITY
    set_intensity(FLASH_INTENSITY)
end

local function begin_fade()
    state = STATE_FADE
    timer = 0.0
    duration = FLASH_FADE_TIME
end

------------------------------------------------

function on_create(self)
    if EntityLight == nil then
        print("[SF_LightningPointLight] ERROR: EntityLight missing")
        return
    end

    SHARED = SHARED or {}
    SHARED[LIGHTNING_TABLE] = SHARED[LIGHTNING_TABLE] or {}

    lastProcessedEventId = -1
    begin_idle()

    if DEBUG then
        print("[SF_LightningPointLight] Ready EntityID=" .. tostring(EntityID))
    end
end

------------------------------------------------

function on_update(self, dt)
    if EntityLight == nil then return end
    dt = dt or 0.016

    SHARED = SHARED or {}
    SHARED[LIGHTNING_TABLE] = SHARED[LIGHTNING_TABLE] or {}

    local lightningTable = SHARED[LIGHTNING_TABLE]
    local lightningData = lightningTable[ROOFTOP_KEY]
    local isPlayerOnRooftop = ((_G.ROOFTOP_COUNT or 0) > 0)

    ------------------------------------------------
    -- If player is NOT on rooftop:
    -- consume latest event silently and keep light off
    ------------------------------------------------
    if not isPlayerOnRooftop then
        if lightningData ~= nil then
            lastProcessedEventId = lightningData.eventId
        end

        begin_idle()
        return
    end

    ------------------------------------------------
    -- LISTEN FOR NEXT NEW ROOFTOP LIGHTNING CUE
    ------------------------------------------------
    if lightningData ~= nil and lightningData.eventId ~= lastProcessedEventId then
        lastProcessedEventId = lightningData.eventId
        begin_hold()

        if DEBUG then
            print("[SF_LightningPointLight] Triggered by rooftop lightning eventId = " .. tostring(lightningData.eventId))
        end
    end

    timer = timer + dt

    ------------------------------------------------
    -- HOLD
    ------------------------------------------------
    if state == STATE_HOLD then
        if timer >= duration then
            begin_fade()
        end

    ------------------------------------------------
    -- FADE
    ------------------------------------------------
    elseif state == STATE_FADE then
        local t = timer / duration
        if t > 1.0 then t = 1.0 end

        local intensity = startIntensity * (1.0 - t)
        set_intensity(intensity)

        if timer >= duration then
            begin_idle()
        end
    end
end

------------------------------------------------

function on_destroy(self)
    set_intensity(OFF_INTENSITY)
end
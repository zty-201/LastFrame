----------------------------------------------------------------
-- Bedroom_LightningSpotlight.lua
-- Lightning-style spotlight flashes.
-- Normal lightning = 50000 intensity
-- Ghost scare lightning = 100000 intensity
----------------------------------------------------------------

local DEBUG = false

-- =========================
-- INTENSITY CONFIG
-- =========================

local NORMAL_INTENSITY = 50000.0
local SCARE_INTENSITY  = 100000.0
local OFF_INTENSITY = 0.0

-- =========================
-- NORMAL LIGHTNING CONFIG
-- =========================

local MIN_WAIT = 2.0
local MAX_WAIT = 4.0

local MIN_HOLD_TIME = 0.12
local MAX_HOLD_TIME = 0.25

local MIN_FADE_TIME = 0.25
local MAX_FADE_TIME = 0.50

local MIN_FLASHES = 1
local MAX_FLASHES = 3

local lastProcessedEventId = -1

-- =========================
-- SCARE LIGHTNING CONFIG
-- =========================

local SCARE_HOLD_TIME = 0.45
local SCARE_FADE_TIME = 1.20

-- =========================
-- COMMANDS
-- =========================

local LIGHTNING_TABLE = "LIGHTNING_CMD"
local SCARE_KEY = "GHOST_GIRL_SCARE"

local THUNDER_SFX_ID = "Thunder"

-- =========================
-- INTERNAL STATE
-- =========================

local STATE_WAIT = 0
local STATE_HOLD = 1
local STATE_FADE = 2

local state = STATE_WAIT

local timer = 0.0
local duration = 0.0

local flashesRemaining = 0
local startIntensity = NORMAL_INTENSITY
local isScareFlash = false

------------------------------------------------

local function random_range(a, b)
    return a + (b - a) * math.random()
end

local function random_int(a, b)
    return math.floor(random_range(a, b + 1))
end

local function set_intensity(v)
    if EntityLight ~= nil then
        EntityLight:SetIntensity(v)
    end
end

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end
    return false
end

------------------------------------------------

local function begin_wait()
    state = STATE_WAIT
    timer = 0.0
    duration = random_range(MIN_WAIT, MAX_WAIT)
    isScareFlash = false
    set_intensity(OFF_INTENSITY)
end

local function begin_hold()
    state = STATE_HOLD
    timer = 0.0
    duration = random_range(MIN_HOLD_TIME, MAX_HOLD_TIME)

    startIntensity = NORMAL_INTENSITY
    isScareFlash = false

    set_intensity(NORMAL_INTENSITY)
end

local function begin_scare_hold()
    state = STATE_HOLD
    timer = 0.0
    duration = SCARE_HOLD_TIME

    startIntensity = SCARE_INTENSITY
    isScareFlash = true

    set_intensity(SCARE_INTENSITY)
end

local function begin_fade()
    state = STATE_FADE
    timer = 0.0

    if isScareFlash then
        duration = SCARE_FADE_TIME
    else
        duration = random_range(MIN_FADE_TIME, MAX_FADE_TIME)
    end
end

------------------------------------------------

function on_create(self)

    if EntityLight == nil then
        print("[SF_LightningSpotlight] ERROR: EntityLight missing")
        return
    end

    SHARED = SHARED or {}
    SHARED[LIGHTNING_TABLE] = SHARED[LIGHTNING_TABLE] or {}

    math.randomseed(os.time() + EntityID)

    begin_wait()

    if DEBUG then
        print("[SF_LightningSpotlight] Ready")
    end
end

------------------------------------------------

function on_update(self, dt)

    if EntityLight == nil then return end
    dt = dt or 0.016

    SHARED = SHARED or {}
    SHARED[LIGHTNING_TABLE] = SHARED[LIGHTNING_TABLE] or {}

    ------------------------------------------------
    -- FORCED FLASH FROM GHOST GIRL SCARE
    ------------------------------------------------
    local lightningTable = SHARED[LIGHTNING_TABLE]

    local scareData = lightningTable[SCARE_KEY]

    if scareData ~= nil and scareData.eventId ~= lastProcessedEventId then
        lastProcessedEventId = scareData.eventId

        flashesRemaining = 1
        begin_scare_hold()

        play_3d_sfx(THUNDER_SFX_ID)

        if DEBUG then
            print("[SF_LightningSpotlight] Forced scare lightning, eventId = " .. tostring(scareData.eventId))
        end
    end

    timer = timer + dt

    ------------------------------------------------
    -- WAIT
    ------------------------------------------------
    if state == STATE_WAIT then

        if timer >= duration then
            flashesRemaining = random_int(MIN_FLASHES, MAX_FLASHES)
            begin_hold()
        end

    ------------------------------------------------
    -- HOLD
    ------------------------------------------------
    elseif state == STATE_HOLD then

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

            flashesRemaining = flashesRemaining - 1

            if flashesRemaining > 0 then
                begin_hold()
            else
                begin_wait()
            end
        end
    end
end

------------------------------------------------

function on_destroy(self)
    set_intensity(OFF_INTENSITY)
end
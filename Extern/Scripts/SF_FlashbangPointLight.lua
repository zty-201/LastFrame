----------------------------------------------------------------
-- SF_FlashbangPointLight.lua
-- Hidden point light that flashbangs the whole room when
-- the ghost girl scare is triggered.
----------------------------------------------------------------

local DEBUG = false

-- =========================
-- CONFIG
-- =========================

local LIGHTNING_TABLE = "LIGHTNING_CMD"
local SCARE_KEY = "GHOST_GIRL_SCARE"

local OFF_INTENSITY = 0.0
local FLASH_INTENSITY = 150000.0   -- big room-filling burst

-- Flash timing
local FLASH_HOLD_TIME = 0.2       -- very brief peak
local FLASH_FADE_TIME = 0.6       -- quick fade out

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
local hasTriggered = false

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

local function begin_flash_hold()
    state = STATE_HOLD
    timer = 0.0
    duration = FLASH_HOLD_TIME
    startIntensity = FLASH_INTENSITY
    set_intensity(FLASH_INTENSITY)
end

local function begin_flash_fade()
    state = STATE_FADE
    timer = 0.0
    duration = FLASH_FADE_TIME
end

------------------------------------------------

function on_create(self)
    if EntityLight == nil then
        print("[SF_FlashbangPointLight] ERROR: EntityLight missing")
        return
    end

    SHARED = SHARED or {}
    SHARED[LIGHTNING_TABLE] = SHARED[LIGHTNING_TABLE] or {}

    hasTriggered = false
    begin_idle()

    if DEBUG then
        print("[SF_FlashbangPointLight] Ready")
    end
end

------------------------------------------------

function on_update(self, dt)
    if EntityLight == nil then return end
    dt = dt or 0.016

    SHARED = SHARED or {}
    SHARED[LIGHTNING_TABLE] = SHARED[LIGHTNING_TABLE] or {}

    local lightningTable = SHARED[LIGHTNING_TABLE]

    ------------------------------------------------
    -- TRIGGER FROM GHOST GIRL SCARE
    ------------------------------------------------
    if not hasTriggered and lightningTable[SCARE_KEY] ~= nil then
        hasTriggered = true
        begin_flash_hold()

        if DEBUG then
            print("[SF_FlashbangPointLight] Ghost scare flash triggered")
        end
    end

    timer = timer + dt

    ------------------------------------------------
    -- HOLD
    ------------------------------------------------
    if state == STATE_HOLD then
        if timer >= duration then
            begin_flash_fade()
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
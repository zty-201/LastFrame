-- CanteenLightEmitterOffOn.lua
-- Syncs emitter with canteen lights, but starts slightly earlier for better visuals

local DEBUG = false

local LIGHTS_TABLE = "LIGHTS_CMD"
local EVENT_OFF_KEY = "CANTEEN_LIGHTS_OFF"
local EVENT_ON_KEY = "CANTEEN_LIGHTS_ON"
local DISABLE_KEY = "CANTEEN_LIGHTS_DISABLED"

local SHUTDOWN_DELAY = 0.5
local FADE_ON_DURATION = 1.0

local EMITTER_START_RATE = 0.0   -- instant visible flame
local EMITTER_TARGET_RATE = 50.0  -- full flame

local shutdownTimer = 0.0
local waitingForShutdown = false

local fadeOnTimer = 0.0
local isFadingOn = false

local isCurrentlyOff = false

local function clamp(v, lo, hi)
    if v < lo then return lo end
    if v > hi then return hi end
    return v
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[LIGHTS_TABLE] = SHARED[LIGHTS_TABLE] or {}

    shutdownTimer = 0.0
    waitingForShutdown = false

    fadeOnTimer = 0.0
    isFadingOn = false

    isCurrentlyOff = false

    if SHARED[LIGHTS_TABLE][EVENT_OFF_KEY] == nil then
        SHARED[LIGHTS_TABLE][EVENT_OFF_KEY] = false
    end
    if SHARED[LIGHTS_TABLE][EVENT_ON_KEY] == nil then
        SHARED[LIGHTS_TABLE][EVENT_ON_KEY] = false
    end
    if SHARED[LIGHTS_TABLE][DISABLE_KEY] == nil then
        SHARED[LIGHTS_TABLE][DISABLE_KEY] = false
    end

    local emitter = Query.GetParticleEmitter(EntityID)
    if emitter == nil then
        print("[Emitter] ERROR: Missing emitter on " .. tostring(EntityID))
        return
    end

    emitter.spawnRate = EMITTER_TARGET_RATE

    if DEBUG then
        print("[Emitter] Ready Entity=" .. tostring(EntityID))
    end
end

function on_update(self, dt)
    dt = dt or 0.016

    local emitter = Query.GetParticleEmitter(EntityID)
    if emitter == nil then return end

    local t = SHARED and SHARED[LIGHTS_TABLE]
    if t == nil then return end

    ----------------------------------------------------------------
    -- ON (start EARLY + visible immediately)
    ----------------------------------------------------------------
    if t[EVENT_ON_KEY] == true then
        if isCurrentlyOff and not isFadingOn then
            isFadingOn = true
            fadeOnTimer = 0.0
            waitingForShutdown = false
            shutdownTimer = 0.0
            t[DISABLE_KEY] = false

            -- start with visible flame immediately
            emitter.spawnRate = EMITTER_START_RATE
            emitter.timeSinceLastSpawn = 0.0

            if DEBUG then
                print("[Emitter] Ignition started")
            end
        end
    end

    if isFadingOn then
        fadeOnTimer = fadeOnTimer + dt
        local alpha = clamp(fadeOnTimer / FADE_ON_DURATION, 0.0, 1.0)

        local rate = EMITTER_START_RATE + (EMITTER_TARGET_RATE - EMITTER_START_RATE) * alpha
        emitter.spawnRate = rate
        emitter.timeSinceLastSpawn = 0.0

        if alpha >= 1.0 then
            isFadingOn = false
            isCurrentlyOff = false
            emitter.spawnRate = EMITTER_TARGET_RATE

            if DEBUG then
                print("[Emitter] Fully ON")
            end
        end

        return
    end

    ----------------------------------------------------------------
    -- OFF (same timing as lights)
    ----------------------------------------------------------------
    if t[EVENT_OFF_KEY] == true then
        if not waitingForShutdown and not isCurrentlyOff then
            waitingForShutdown = true
            shutdownTimer = 0.0
        end
    end

    if waitingForShutdown then
        shutdownTimer = shutdownTimer + dt

        if shutdownTimer >= SHUTDOWN_DELAY then
            emitter.spawnRate = 0.0
            emitter.timeSinceLastSpawn = 0.0

            isCurrentlyOff = true
            isFadingOn = false
            fadeOnTimer = 0.0

            waitingForShutdown = false
            shutdownTimer = 0.0
            t[DISABLE_KEY] = true

            if DEBUG then
                print("[Emitter] OFF")
            end
        end
    end
end
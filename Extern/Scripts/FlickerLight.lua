----------------------------------------------------------------
-- FlickerLight.lua
-- Simple standalone flickering light behaviour
-- Attach to a light entity
----------------------------------------------------------------

local DEBUG = false

local baseIntensity = 1.0
local currentIntensity = 1.0

local flickerTimer = 0.0
local nextFlickerTime = 0.0

local flickerElapsed = 0.0
local flickerDuration = 0.0
local isFlickering = false

local LIGHTS_TABLE = "LIGHTS_CMD"
local DISABLE_KEY = "CANTEEN_LIGHTS_DISABLED"
local EVENT_ON_KEY = "CANTEEN_LIGHTS_ON"
local permanentlyOff = false

local function random_range(minVal, maxVal)
    return minVal + (maxVal - minVal) * math.random()
end

local function schedule_next_flicker()
    nextFlickerTime = random_range(0.05, 0.25)
    flickerTimer = 0.0
end

local function start_flicker()
    isFlickering = true
    flickerElapsed = 0.0
    flickerDuration = random_range(0.2, 0.4)
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED[LIGHTS_TABLE] = SHARED[LIGHTS_TABLE] or {}

    permanentlyOff = false

    if SHARED[LIGHTS_TABLE][DISABLE_KEY] == nil then
        SHARED[LIGHTS_TABLE][DISABLE_KEY] = false
    end
    if SHARED[LIGHTS_TABLE][EVENT_ON_KEY] == nil then
        SHARED[LIGHTS_TABLE][EVENT_ON_KEY] = false
    end

    if EntityLight == nil then
        print("[FlickerLight] ERROR: EntityLight missing on entity " .. tostring(EntityID))
        return
    end

    baseIntensity = 500.0
    currentIntensity = 500.0
    EntityLight:SetIntensity(500.0)

    math.randomseed(os.time() + EntityID)
    schedule_next_flicker()

    if DEBUG then
        print("[FlickerLight] Ready. Base intensity:", baseIntensity)
    end
end

function on_update(self, dt)
    if EntityLight == nil then return end
    dt = dt or 0.016

    if SHARED and SHARED[LIGHTS_TABLE] and SHARED[LIGHTS_TABLE][EVENT_ON_KEY] == true then
        permanentlyOff = false
    end

    if SHARED and SHARED[LIGHTS_TABLE] and SHARED[LIGHTS_TABLE][DISABLE_KEY] == true then
        permanentlyOff = true
    end

    if permanentlyOff then
        EntityLight:SetIntensity(0.0)
        return
    end

    if isFlickering then
        flickerElapsed = flickerElapsed + dt

        if flickerElapsed >= flickerDuration then
            isFlickering = false
            currentIntensity = baseIntensity
            schedule_next_flicker()
        else
            currentIntensity = baseIntensity * random_range(0.85, 1.05)
        end
    else
        flickerTimer = flickerTimer + dt

        if flickerTimer >= nextFlickerTime then
            start_flicker()
        end
    end

    EntityLight:SetIntensity(currentIntensity)
end
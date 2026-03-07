----------------------------------------------------------------
-- FlickerLight.lua
-- Simple standalone flickering light behaviour
-- Attach to a light entity
----------------------------------------------------------------

local DEBUG = false

local baseIntensity = 1.0
local currentIntensity = 1.0

-- how often flicker bursts happen
local flickerTimer = 0.0
local nextFlickerTime = 0.0

-- flicker state
local flickerElapsed = 0.0
local flickerDuration = 0.0
local isFlickering = false

------------------------------------------------

local function random_range(minVal, maxVal)
    return minVal + (maxVal - minVal) * math.random()
end

local function schedule_next_flicker()
    -- frequent like flame
    nextFlickerTime = random_range(0.05, 0.25)
    flickerTimer = 0.0
end

local function start_flicker()
    isFlickering = true
    flickerElapsed = 0.0

    -- short flicker burst
    flickerDuration = random_range(0.2, 0.4)
end

------------------------------------------------

function on_create(self)

    if EntityLight == nil then
        print("[FlickerLight] ERROR: EntityLight missing on entity " .. tostring(EntityID))
        return
    end

    baseIntensity = EntityLight:GetIntensity()
    currentIntensity = baseIntensity

    math.randomseed(os.time() + EntityID)

    schedule_next_flicker()

    if DEBUG then
        print("[FlickerLight] Ready. Base intensity:", baseIntensity)
    end
end

------------------------------------------------

function on_update(self, dt)

    if EntityLight == nil then return end

    dt = dt or 0.016

    if isFlickering then

        flickerElapsed = flickerElapsed + dt

        if flickerElapsed >= flickerDuration then
            isFlickering = false
            currentIntensity = baseIntensity
            schedule_next_flicker()
        else
            -- random variation around base intensity
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
----------------------------------------------------------------
-- RainDrop_Fall.lua
----------------------------------------------------------------

local DEBUG = false

local SPEED_MIN = 1800.0
local SPEED_MAX = 2000.0
local FALL_DISTANCE = 500.0
local RESPAWN_RADIUS = 15.0

local WIND_DRIFT = 40.0

local START_Y_MIN = 1000.0
local START_Y_MAX = 1055.0

local RANDOMIZE_START_PROGRESS = true

local DETECTOR_COUNT_KEY = "ROOFTOP_COUNT"

local hasInitialized = false
local wasActiveLastFrame = false

local startX = 0.0
local startZ = 0.0
local startY = 0.0

local currentX = 0.0
local currentY = 0.0
local currentZ = 0.0

local currentFall = 0.0
local currentSpeed = 0.0

------------------------------------------------

local function random_range(a, b)
    return a + (b - a) * math.random()
end

local function random_point_in_circle(radius)
    local angle = random_range(0.0, math.pi * 2.0)
    local dist = math.sqrt(math.random()) * radius

    local x = math.cos(angle) * dist
    local z = math.sin(angle) * dist

    return x, z
end

local function is_rain_active()
    return ((_G[DETECTOR_COUNT_KEY] or 0) > 0)
end

local function hide_drop(tr)
    if tr == nil then return end

    -- Push far below the map while inactive
    tr:SetPosition(currentX, -99999.0, currentZ)
end

local function respawn_drop(tr)
    if tr == nil then return end

    local offsetX, offsetZ = random_point_in_circle(RESPAWN_RADIUS)

    startY = random_range(START_Y_MIN, START_Y_MAX)

    currentFall = 0.0
    currentX = startX + offsetX
    currentY = startY
    currentZ = startZ + offsetZ

    currentSpeed = random_range(SPEED_MIN, SPEED_MAX)

    tr:SetPosition(currentX, currentY, currentZ)

    if DEBUG then
        print(string.format(
            "[RainDrop] Respawn -> (%.2f, %.2f, %.2f) speed=%.2f",
            currentX, currentY, currentZ, currentSpeed
        ))
    end
end

local function init_if_needed(tr)
    if hasInitialized or tr == nil then
        return
    end

    local editorX, _, editorZ = tr:GetPosition()

    startX = editorX
    startZ = editorZ
    startY = random_range(START_Y_MIN, START_Y_MAX)

    currentX = startX
    currentY = startY
    currentZ = startZ
    currentFall = 0.0
    currentSpeed = random_range(SPEED_MIN, SPEED_MAX)

    if RANDOMIZE_START_PROGRESS then
        currentFall = random_range(0.0, FALL_DISTANCE)
        currentY = startY - currentFall
    end

    tr:SetPosition(currentX, currentY, currentZ)

    hasInitialized = true

    if DEBUG then
        print(string.format(
            "[RainDrop] Initialized startX=%.2f startZ=%.2f startY=%.2f speed=%.2f",
            startX, startZ, startY, currentSpeed
        ))
    end
end

local function reactivate_drop(tr)
    if tr == nil then return end

    -- Fresh randomized rain cycle whenever player re-enters detector
    startY = random_range(START_Y_MIN, START_Y_MAX)
    currentSpeed = random_range(SPEED_MIN, SPEED_MAX)

    currentX = startX
    currentZ = startZ
    currentFall = 0.0
    currentY = startY

    if RANDOMIZE_START_PROGRESS then
        currentFall = random_range(0.0, FALL_DISTANCE)
        currentY = startY - currentFall
    end

    tr:SetPosition(currentX, currentY, currentZ)

    if DEBUG then
        print("[RainDrop] Reactivated")
    end
end

------------------------------------------------

function on_create(self)
    if _G.__RAIN_RANDOMSEEDED ~= true then
        math.randomseed(os.time())
        _G.__RAIN_RANDOMSEEDED = true
    end

    hasInitialized = false
    wasActiveLastFrame = false
    currentFall = 0.0
end

------------------------------------------------

function on_update(self, dt)
    dt = dt or 0.016

    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end

    local active = is_rain_active()

    -- Outside detector: keep hidden and do no rain simulation
    if not active then
        if hasInitialized then
            hide_drop(tr)
        end
        wasActiveLastFrame = false
        return
    end

    -- First time entering detector: capture editor X/Z correctly here
    init_if_needed(tr)
    if not hasInitialized then return end

    -- Re-entering detector after being inactive
    if not wasActiveLastFrame then
        reactivate_drop(tr)
    end

    wasActiveLastFrame = true

    currentFall = currentFall + (currentSpeed * dt)
    currentY = startY - currentFall
    currentX = currentX + random_range(-WIND_DRIFT, WIND_DRIFT) * dt

    tr:SetPosition(currentX, currentY, currentZ)

    if currentFall >= FALL_DISTANCE then
        respawn_drop(tr)
    end
end

------------------------------------------------

function on_destroy(self)
end
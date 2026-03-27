-- HallLightController2.0.lua
-- Attach to Light entity
-- Controlled via SHARED.LIGHT_CMD[EntityID]
-- Commands:
--   { cmd="off", by=<id> }
--   { cmd="on",  by=<id> }
--   { cmd="set", by=<id>, value=<number> }

local defaultIntensity = 1.0
local currentIntensity = 1.0

local LIGHT_ON_SFX_ID = "LightOn"

local isLightOn = true

-- Flicker settings
local flickerTimer = 0.0
local nextFlickerTime = 0.0
local flickerDuration = 0.0
local flickerElapsed = 0.0
local isFlickering = false
local flickerIntensity = 0.0

local function ensure_shared()
    SHARED = SHARED or {}
    SHARED.LIGHT_CMD = SHARED.LIGHT_CMD or {}
end

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end
    return false
end

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

    -- very short bursts
    flickerDuration = random_range(0.02, 0.08)

    -- flame-like brightness variation
    flickerIntensity = defaultIntensity * random_range(0.3, 1.05)
end

function on_create(self)
    ensure_shared()

    if EntityLight == nil then
        print("[Light] ERROR: No LightComponent on EntityID=" .. tostring(EntityID))
        return
    end

    currentIntensity = EntityLight:GetIntensity()
    defaultIntensity = currentIntensity
    isLightOn = currentIntensity > 0.0

    local seedBase = tonumber(EntityID) or 0
    if os ~= nil and type(os.time) == "function" then
        math.randomseed(os.time() + seedBase)
    else
        math.randomseed(54321 + seedBase)
    end

    schedule_next_flicker()

    print(string.format(
        "[Light] ready EntityID=%d defaultIntensity=%.2f",
        EntityID,
        defaultIntensity
    ))
end

function on_update(self, dt)
    if EntityLight == nil then return end
    dt = dt or 0.016
    ensure_shared()

    local cmdObj = SHARED.LIGHT_CMD[EntityID]

    if cmdObj then
        SHARED.LIGHT_CMD[EntityID] = nil
        local cmd = cmdObj.cmd

        if cmd == "off" then
            currentIntensity = 0.0
            isLightOn = false
            isFlickering = false

        elseif cmd == "on" then
            currentIntensity = defaultIntensity
            isLightOn = true
            play_3d_sfx(LIGHT_ON_SFX_ID)
            schedule_next_flicker()

        elseif cmd == "set" then
            local v = cmdObj.value
            if v ~= nil then
                local prevIntensity = currentIntensity
                local newIntensity = tonumber(v) or currentIntensity

                currentIntensity = newIntensity
                isLightOn = currentIntensity > 0.0

                if currentIntensity > 0.0 then
                    defaultIntensity = currentIntensity

                    if prevIntensity <= 0.0 then
                        play_3d_sfx(LIGHT_ON_SFX_ID)
                    end

                    schedule_next_flicker()
                else
                    isFlickering = false
                end
            end
        end

        print(string.format(
            "[Light] CMD=%s by=%s -> intensity=%.2f",
            tostring(cmd),
            tostring(cmdObj.by),
            currentIntensity
        ))
    end

    if isLightOn then
        if isFlickering then
            flickerElapsed = flickerElapsed + dt

            if flickerElapsed >= flickerDuration then
                isFlickering = false
                currentIntensity = defaultIntensity
                schedule_next_flicker()
            else
                currentIntensity = defaultIntensity * random_range(0.55, 1.05)
            end
        else
            flickerTimer = flickerTimer + dt
            if flickerTimer >= nextFlickerTime then
                start_flicker()
            end
        end
    end

    EntityLight:SetIntensity(currentIntensity)
end
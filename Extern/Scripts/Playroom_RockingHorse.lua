----------------------------------------------------------------
-- RockingHorse.lua
-- Rocks during CP1
-- Plays one non-looping rocking horse audio track during CP1
-- Force stops the track when CP2 begins
----------------------------------------------------------------

local DEBUG = false

local STATE_IDLE = 0
local STATE_CP1  = 1

local ROCK_ANGLE = 10.0
local ROCK_SPEED = 3.42

local ROCK_SOUND_SLOT = "HorseRock"

local baseRotX = 0.0
local baseRotY = 0.0
local baseRotZ = 0.0

local timer = 0.0
local rockingActive = false
local audioStarted = false

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
    end
end

local function stop_3d_sfx(slotId)
    if type(Audio_StopSlot) == "function" and slotId ~= nil then
        Audio_StopSlot(EntityID, slotId)
    end
end

function on_create(self)
    local tr = Query.GetTransform(EntityID)
    if not tr then
        print("[RockingHorse] ERROR: no transform")
        return
    end

    baseRotX, baseRotY, baseRotZ = tr:GetRotation()

    timer = 0.0
    rockingActive = false
    audioStarted = false

    if DEBUG then
        print("[RockingHorse] Ready")
    end
end

function on_update(self, dt)
    dt = dt or 0.016

    local controller = SHARED and SHARED.StateController and SHARED.StateController._self
    if not controller then return end

    local tr = Query.GetTransform(EntityID)
    if not tr then return end

    if controller.state == STATE_CP1 then
        if not rockingActive then
            rockingActive = true
            timer = 0.0

            if not audioStarted then
                play_3d_sfx(ROCK_SOUND_SLOT)
                audioStarted = true
            end

            if DEBUG then
                print("[RockingHorse] CP1 started -> rocking + audio start")
            end
        end

        timer = timer + dt

        local wave = math.sin(timer * ROCK_SPEED)
        local offset = wave * ROCK_ANGLE

        tr:SetRotation(baseRotX + offset, baseRotY, baseRotZ)

    else
        if rockingActive then
            stop_3d_sfx(ROCK_SOUND_SLOT)

            if DEBUG then
                print("[RockingHorse] CP1 ended -> rocking stop + audio stop")
            end
        end

        rockingActive = false
        audioStarted = false
        timer = 0.0

        tr:SetRotation(baseRotX, baseRotY, baseRotZ)
    end
end
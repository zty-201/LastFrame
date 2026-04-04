----------------------------------------------------------------
-- SF_Graves_Detector.lua
-- When player steps into this box play an audio cue
----------------------------------------------------------------
print("[GravesRevealTrigger] script loaded")

local DEBUG = true
local PLAYER_ID = 381

local VOICEOVER_SFX_ID = "VoiceOver"
local CREEPY_SFX_ID = "Creepy"
local VO_DELAY = 1.2

local fired = false
local playerInside = false

local pendingVO = false
local voTimer = 0.0

local function trigger_reaction()
    if fired then return end

    ------------------------------------------------
    -- Play creepy stinger immediately on first entry
    ------------------------------------------------
    if type(Audio_PlaySlot) == "function" then
        Audio_PlaySlot(EntityID, CREEPY_SFX_ID)
    end

    pendingVO = true
    voTimer = 0.0
    fired = true

    if DEBUG then
        print("[GravesRevealTrigger] First entry -> Creepy played immediately, delayed voiceover scheduled")
    end
end

function on_create(self)
    fired = false
    playerInside = false
    pendingVO = false
    voTimer = 0.0

    if DEBUG then
        print("[GravesRevealTrigger] Ready EntityID =", EntityID)
    end
end

function on_trigger_enter(self, otherId)
    if otherId ~= PLAYER_ID then return end

    if playerInside then
        return
    end

    playerInside = true
    trigger_reaction()

    if DEBUG then
        print("[GravesRevealTrigger] ENTER")
    end
end

function on_trigger_exit(self, otherId)
    if otherId ~= PLAYER_ID then return end

    playerInside = false

    if DEBUG then
        print("[GravesRevealTrigger] EXIT")
    end
end

function on_update(self, dt)
    dt = dt or 0.016

    if pendingVO then
        voTimer = voTimer + dt
        if voTimer >= VO_DELAY then
            pendingVO = false
            voTimer = 0.0

            if type(Audio_PlaySlot) == "function" then
                Audio_PlaySlot(EntityID, VOICEOVER_SFX_ID)
            end

            SHARED = SHARED or {}
            SHARED.SUBTITLE_CMD = SHARED.SUBTITLE_CMD or {}
            SHARED.SUBTITLE_CMD["GRAVES_REVEAL_REACTION"] = {
                trigger = true,
                by = EntityID
            }

            if DEBUG then
                print("[GravesRevealTrigger] VoiceOver + subtitle played")
            end
        end
    end
end

function on_destroy(self)
    playerInside = false
end
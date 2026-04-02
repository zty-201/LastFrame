-- Bedroom_EnterTrigger.lua
-- 1. Player enters -> activate girl + start crying
-- 2. If checkpoint has been reached and player leaves -> deactivate girl permanently
-- 3. After that, re-entering the bedroom will NOT reactivate her

local DEBUG = true
local PLAYER_ENTITY_ID = 381

local GIRL_ENTITY_ID = 251
local TENSION_SLOT   = "Tension"

local VOICEOVER_01_SFX_ID = "VoiceOver_01"
local VO_DELAY = 3.0
local VOICEOVER_02_SFX_ID = "VoiceOver_02"
local EXIT_VO_DELAY = 3.0

local hasPlayedVO = false
local pendingVO = false
local voTimer = 0.0

local hasPlayedExitVO = false
local pendingExitVO = false
local exitVOTimer = 0.0

local playerInside = false

local function activate_girl()
    SHARED = SHARED or {}

    if SHARED.BEDROOM_GIRL_DONE == true then
        if DEBUG then
            print("[Bedroom_EnterTrigger] activate_girl skipped, girl already permanently gone")
        end
        return
    end

    SHARED.BEDROOM_GIRL_ACTIVE = true

    -- 🔥 NEW: tell LookAt script to start crying cycle
    SHARED.BEDROOM_START_CRY = true

    if DEBUG then
        print("[Bedroom_EnterTrigger] Activated bedroom girl + start cry cycle")
    end
end

local function deactivate_girl_permanently()
    SHARED = SHARED or {}
    SHARED.BEDROOM_GIRL_ACTIVE = false
    SHARED.BEDROOM_GIRL_DONE   = true

    if type(Audio_StopSlot) == "function" then
        Audio_StopSlot(GIRL_ENTITY_ID, TENSION_SLOT)
    end

    if DEBUG then
        print("[Bedroom_EnterTrigger] Bedroom girl permanently deactivated")
    end
end

function on_create(self)
    SHARED = SHARED or {}
    SHARED.BEDROOM_GIRL_ACTIVE = true
    SHARED.BEDROOM_CP_REACHED  = false
    SHARED.BEDROOM_GIRL_DONE   = false
    
    playerInside = false

    if DEBUG then
        print("[Bedroom_EnterTrigger] Ready on entity " .. tostring(EntityID))
    end
end

function on_trigger_enter(self, otherID)
    if otherID ~= PLAYER_ENTITY_ID then
        return
    end

    if playerInside then
        return
    end

    playerInside = true

    -- If girl is already permanently gone, do not reactivate her.
    -- Instead, play VoiceOver_02 once after a delay.
    if SHARED and SHARED.BEDROOM_GIRL_DONE == true then
        if DEBUG then
            print("[Bedroom_EnterTrigger] Entered room after girl permanently disappeared")
        end

        if not hasPlayedExitVO and not pendingExitVO then
            pendingExitVO = true
            exitVOTimer = 0.0

            if DEBUG then
                print("[Bedroom_EnterTrigger] VoiceOver_02 scheduled")
            end
        end
        return
    end

    activate_girl()

    -- First-time delayed VO
    if not hasPlayedVO and not pendingVO then
        pendingVO = true
        voTimer = 0.0

        if DEBUG then
            print("[Bedroom_EnterTrigger] First entry -> VO scheduled")
        end
    end
end

function on_trigger_exit(self, otherID)
    if otherID ~= PLAYER_ENTITY_ID then
        return
    end

    playerInside = false

    SHARED = SHARED or {}

    -- ALWAYS deactivate girl when leaving
    SHARED.BEDROOM_GIRL_ACTIVE = false

    if type(Audio_StopSlot) == "function" then
        Audio_StopSlot(GIRL_ENTITY_ID, TENSION_SLOT)
    end

    if DEBUG then
        print("[Bedroom_EnterTrigger] Player exited -> girl deactivated")
    end

    ----------------------------------------------------------------
    -- ONLY mark permanently gone if checkpoint was reached
    ----------------------------------------------------------------
    if SHARED.BEDROOM_CP_REACHED == true and SHARED.BEDROOM_GIRL_DONE ~= true then
        SHARED.BEDROOM_GIRL_DONE = true

        if DEBUG then
            print("[Bedroom_EnterTrigger] Girl permanently removed after checkpoint")
        end
    end
end

function on_update(self, dt)
    dt = dt or 0.016

    ------------------------------------------------
    -- VO 1 (first entry)
    ------------------------------------------------
    if pendingVO then
        voTimer = voTimer + dt
        if voTimer >= VO_DELAY then
            pendingVO = false
            voTimer = 0.0
            hasPlayedVO = true

            if type(Audio_PlaySlot) == "function" then
                Audio_PlaySlot(EntityID, VOICEOVER_01_SFX_ID)
            end

            if DEBUG then
                print("[Bedroom_EnterTrigger] VoiceOver_01 played")
            end
        end
    end

    ------------------------------------------------
    -- VO 2 (re-enter after girl permanently gone)
    ------------------------------------------------
    if pendingExitVO then
        exitVOTimer = exitVOTimer + dt
        if exitVOTimer >= EXIT_VO_DELAY then
            pendingExitVO = false
            exitVOTimer = 0.0
            hasPlayedExitVO = true

            if type(Audio_PlaySlot) == "function" then
                Audio_PlaySlot(EntityID, VOICEOVER_02_SFX_ID)
            end

            if DEBUG then
                print("[Bedroom_EnterTrigger] VoiceOver_02 played")
            end
        end
    end
end

function on_destroy(self)
    playerInside = false
end
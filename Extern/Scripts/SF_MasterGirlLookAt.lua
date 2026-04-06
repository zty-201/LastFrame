----------------------------------------------------------------
-- SF_MasterGirlLookAt.lua
-- Triggers after all evidence pages are viewed.
-- Sequence: force camera to girl -> laugh -> VO16 ->
--           girl turns + DontLookAtMe -> zoom+wiggle ->
--           scream+vigorous shake -> credits
----------------------------------------------------------------

local DEBUG              = true
local LOG_EVERY_N_FRAMES = 60

local ACTIVATION_FLAG = "EVIDENCE_FOLDER_FINISHED"

local VISIBLE_SCALE_X = 1.0
local VISIBLE_SCALE_Y = 1.0
local VISIBLE_SCALE_Z = 1.0

local LAUGH_SLOT    = "GirlLaughing"
local LAUGH_VOL_MIN = 0.3
local LAUGH_VOL_MAX = 1.0

-- Sequence timings (from activation)
local LAUGH_BEFORE_TURN    = 2.0   -- laugh plays for this long before camera snaps to girl
local TURN_VO_DELAY        = 1.3   -- delay after camera turn before VoiceOver_16 plays
local VO16_DURATION        = 1.6   -- adjust to match actual clip length
local DONT_LOOK_DELAY      = 0.1   -- gap between VO end and DontLookAtMe
local DONT_LOOK_DURATION   = 1.5   -- adjust to match actual clip length
local POST_DONT_LOOK_DELAY = 0.5   -- pause after DontLookAtMe before pull
local PULL_DURATION        = 0.15  -- how long the pull toward girl lasts
local SHIFT_UP_DURATION    = 0.4   -- camera shift upward after pull
local PULL_SETTLE          = 0.3   -- brief pause after shift before scream
local SCREAM_LEAD          = 1.2   -- how early to fire scream audio before shake re-assert
local SCREAM_DURATION      = 0.2   -- adjust to match actual clip length

-- Pull toward girl
local PULL_AMOUNT    = 395.0
local PULL_AMPLITUDE = 8.0

-- Upward nudge after pull
local CAM_SHIFT_UP      = 5.0
local CAM_SHIFT_FORWARD = 2.0

-- Rain fade out
local RAIN_EMITTER_1     = 900
local RAIN_EMITTER_2     = 905
local RAIN_SLOT          = "RainLoop"
local RAIN_VOL_1         = 1.133  -- original volumes from scene
local RAIN_VOL_2         = 1.235
local RAIN_FADE_DURATION = 0.5    -- seconds to fade out

-- Vigorous shake when scream hits
local VIG_SHAKE_AMPLITUDE = 200.0
local VIG_SHAKE_SPEED     = 0.05

-- Derived timestamps
local T_TURN      = LAUGH_BEFORE_TURN
local T_VO        = T_TURN + TURN_VO_DELAY
local T_DONT_LOOK = T_VO + VO16_DURATION + DONT_LOOK_DELAY
local T_MUTE      = T_DONT_LOOK + DONT_LOOK_DURATION
local T_ZOOM      = T_MUTE + POST_DONT_LOOK_DELAY
local T_SHIFT     = T_ZOOM + PULL_DURATION
local T_SCREAM       = T_SHIFT + SHIFT_UP_DURATION + PULL_SETTLE
local T_SCREAM_AUDIO = T_SCREAM - SCREAM_LEAD
local T_CREDITS      = T_SCREAM + SCREAM_DURATION

-- State
local isActivated       = false
local activationHandled = false
local isGirlVisible     = false
local sequenceTimer     = 0.0
local frame             = 0

local turnPlayed     = false
local voPlayed       = false
local dontLookPlayed = false
local mutePlayed     = false
local rainFading     = false
local rainFadeTimer  = 0.0
local rainStopped    = false
local zoomTriggered  = false
local shiftTriggered = false
local screamPlayed   = false
local shakeStopped   = false
local creditsFired   = false

------------------------------------------------
local function show_girl()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(VISIBLE_SCALE_X, VISIBLE_SCALE_Y, VISIBLE_SCALE_Z)
        isGirlVisible = true
        if DEBUG then print("[MasterGirlLookAt] Girl shown") end
    end
end

local function hide_girl()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(0.0, 0.0, 0.0)
        isGirlVisible = false
        if DEBUG then print("[MasterGirlLookAt] Girl hidden") end
    end
end

local function force_player_look_at_girl()
    SHARED = SHARED or {}
    SHARED.POV = SHARED.POV or {}
    SHARED.POV.locked           = true
    SHARED.POV.lockTargetEntity = EntityID
    if DEBUG then print("[MasterGirlLookAt] Camera locked to girl") end
end

local function activate_girl()
    if isActivated then return end
    isActivated = true
    show_girl()
    Audio_SetSlotVolume(EntityID, LAUGH_SLOT, LAUGH_VOL_MIN)
    Audio_PlaySlot(EntityID, LAUGH_SLOT)
    if DEBUG then print("[MasterGirlLookAt] Activated — laugh playing, camera free") end
end

------------------------------------------------

function on_create(self)
    SHARED = SHARED or {}
    SHARED[ACTIVATION_FLAG] = SHARED[ACTIVATION_FLAG] or false

    isActivated       = false
    activationHandled = false
    isGirlVisible     = false
    sequenceTimer     = 0.0
    frame             = 0

    turnPlayed     = false
    voPlayed       = false
    dontLookPlayed = false
    mutePlayed     = false
    rainFading     = false
    rainFadeTimer  = 0.0
    rainStopped    = false
    zoomTriggered  = false
    shiftTriggered = false
    screamPlayed   = false
    shakeStopped   = false
    creditsFired   = false

    hide_girl()
    if DEBUG then print("[MasterGirlLookAt] Ready on entity " .. tostring(EntityID)) end
end

function on_update(self, dt)
    dt = dt or 0.016

    if not activationHandled and SHARED and SHARED[ACTIVATION_FLAG] == true then
        activationHandled = true
        activate_girl()
    end

    if not isActivated then return end

    sequenceTimer = sequenceTimer + dt

    -- Camera snaps to face girl after laugh plays
    if not turnPlayed and sequenceTimer >= T_TURN then
        turnPlayed = true
        force_player_look_at_girl()
        if DEBUG then print("[MasterGirlLookAt] Camera turned to girl") end
    end

    -- VoiceOver_16 plays after camera snaps to girl
    if not voPlayed and sequenceTimer >= T_VO then
        voPlayed = true
        Audio_PlaySlot(EntityID, "VoiceOver_16")
        if DEBUG then print("[MasterGirlLookAt] VoiceOver_16 playing") end
    end

    -- Girl turns to face player + DontLookAtMe plays + laugh ramps up + rain starts fading
    -- Also unlock camera here so it's free to be pulled at T_ZOOM
    if not dontLookPlayed and sequenceTimer >= T_DONT_LOOK then
        dontLookPlayed = true
        Transform_SetRotation(EntityID, 0.0, 270.0, 0.0)
        Audio_SetSlotVolume(EntityID, LAUGH_SLOT, LAUGH_VOL_MAX)
        Audio_PlaySlot(EntityID, "DontLookAtMe")
        SHARED = SHARED or {}
        SHARED.POV = SHARED.POV or {}
        SHARED.POV.locked           = false
        SHARED.POV.lockTargetEntity = nil
        mutePlayed    = true
        rainFading    = true
        rainFadeTimer = 0.0
        if DEBUG then print("[MasterGirlLookAt] Girl turned + DontLookAtMe playing, camera unlocked, rain fading") end
    end

    if rainFading and not rainStopped then
        rainFadeTimer = rainFadeTimer + dt
        local t = math.min(rainFadeTimer / RAIN_FADE_DURATION, 1.0)
        Audio_SetSlotVolume(RAIN_EMITTER_1, RAIN_SLOT, RAIN_VOL_1 * (1.0 - t))
        Audio_SetSlotVolume(RAIN_EMITTER_2, RAIN_SLOT, RAIN_VOL_2 * (1.0 - t))
        if t >= 1.0 then
            rainStopped = true
            rainFading  = false
            Audio_StopSlot(RAIN_EMITTER_1, RAIN_SLOT)
            Audio_StopSlot(RAIN_EMITTER_2, RAIN_SLOT)
            if DEBUG then print("[MasterGirlLookAt] Rain fully faded and stopped") end
        end
    end

    -- Pull camera toward girl (this is the zoom)
    if not zoomTriggered and sequenceTimer >= T_ZOOM then
        zoomTriggered = true
        SHARED = SHARED or {}
        SHARED.POV = SHARED.POV or {}
        SHARED.POV.locked           = true
        SHARED.POV.lockTargetEntity = EntityID
        SHARED.POV.shake            = true
        SHARED.POV.shakeDuration    = PULL_DURATION
        SHARED.POV.shakeAmplitude   = PULL_AMPLITUDE
        SHARED.POV.shakeSpeed       = 0.01
        SHARED.POV.shakePullAmount    = PULL_AMOUNT
        SHARED.POV.freeShakeSustain   = true
        SHARED.POV.freeShakeAmplitude = VIG_SHAKE_AMPLITUDE
        SHARED.POV.freeShakeSpeed     = VIG_SHAKE_SPEED
        if DEBUG then print("[MasterGirlLookAt] Pull zoom + vigorous shake triggered") end
    end

    -- After pull: nudge camera upward toward face
    if not shiftTriggered and sequenceTimer >= T_SHIFT then
        shiftTriggered = true
        SHARED = SHARED or {}
        SHARED.POV = SHARED.POV or {}
        SHARED.POV.camShift         = true
        SHARED.POV.camShiftUp       = CAM_SHIFT_UP
        SHARED.POV.camShiftForward  = CAM_SHIFT_FORWARD
        SHARED.POV.camShiftDuration = SHIFT_UP_DURATION
        if DEBUG then print("[MasterGirlLookAt] Upward shift triggered") end
    end

    -- Fire scream audio slightly early to account for audio delay
    if not screamPlayed and sequenceTimer >= T_SCREAM_AUDIO then
        screamPlayed = true
        Audio_PlaySlot(EntityID, "Scream")
        if DEBUG then print("[MasterGirlLookAt] Scream audio triggered (early)") end
    end

    -- Re-assert shake after camShift settles
    if screamPlayed and not shakeStopped and sequenceTimer >= T_SCREAM then
        SHARED = SHARED or {}
        SHARED.POV = SHARED.POV or {}
        SHARED.POV.freeShakeSustain   = true
        SHARED.POV.freeShakeAmplitude = VIG_SHAKE_AMPLITUDE
        SHARED.POV.freeShakeSpeed     = VIG_SHAKE_SPEED
        if DEBUG then print("[MasterGirlLookAt] Shake re-asserted") end
    end

    -- Scream done: stop shake, fire credits
    if screamPlayed and not shakeStopped and sequenceTimer >= T_CREDITS then
        shakeStopped = true
        SHARED = SHARED or {}
        SHARED.POV = SHARED.POV or {}
        SHARED.POV.freeShakeSustain = false
        if DEBUG then print("[MasterGirlLookAt] Shake stopped") end
    end

    if shakeStopped and not creditsFired then
        creditsFired = true
        if DEBUG then print("[MasterGirlLookAt] Triggering ToEndCredit") end
        if StateMachine_SetTrigger then
            StateMachine_SetTrigger("ToEndCredit")
        end
    end

    if DEBUG then
        frame = frame + 1
        if (frame % LOG_EVERY_N_FRAMES) == 0 then
            print(string.format("[MasterGirlLookAt] t=%.2f", sequenceTimer))
        end
    end
end

function on_destroy(self)
    isActivated       = false
    activationHandled = false
    isGirlVisible     = false
    sequenceTimer     = 0.0

    turnPlayed     = false
    voPlayed       = false
    dontLookPlayed = false
    mutePlayed     = false
    rainFading     = false
    rainFadeTimer  = 0.0
    rainStopped    = false
    zoomTriggered  = false
    shiftTriggered = false
    screamPlayed   = false
    shakeStopped   = false
    creditsFired   = false

    if type(Audio_StopSlot) == "function" then
        Audio_StopSlot(EntityID, LAUGH_SLOT)
    end
end

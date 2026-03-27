----------------------------------------------------------------
-- EvidenceGirlLookAt.lua
-- Another girl object:
-- Appears only after all evidence pages are viewed.
-- Immediately forces player camera to look at her.
-- Reuses the same look-at danger sequence afterwards.
----------------------------------------------------------------

local DEBUG               = true
local LOG_EVERY_N_FRAMES  = 15

local ACTIVATION_FLAG = "EVIDENCE_FOLDER_FINISHED"

local MAX_DISTANCE            = 1000.0
local LOOK_DOT_THRESHOLD      = 0.95
local REQUIRED_LOOK_TIME      = 0.10
local STOP_LOOKING_DELAY      = 6.0
local SILENCE_BEFORE_JINGLE   = 0.0
local EPIC_SOUND_OFFSET       = 0.0
local GIRL_LOOK_HEIGHT_OFFSET = 0.0

local LAUGH_SLOT       = "GirlLaughing"
local TENSION_SLOT     = "Tension"
local LAUGH_VOL_MIN    = 0.3
local LAUGH_VOL_MAX    = 1.0

local VISIBLE_SCALE_X = 1.0
local VISIBLE_SCALE_Y = 1.0
local VISIBLE_SCALE_Z = 1.0

local isActivated       = false
local activationHandled = false
local isGirlVisible     = false

local tensionPlaying    = false
local isLooking         = false
local thresholdMet      = false
local stopLookingPlayed = false
local epicSoundPlayed   = false
local jinglePlayed      = false
local lookTimer         = 0.0
local silenceTimer      = 0.0
local frame             = 0

local rotateAfterPull         = false
local rotateTimer             = 0.0
local ROTATE_AFTER_PULL_DELAY = 1.2

local creditsCountdown        = false
local creditsTimer            = 0.0
local CREDITS_DELAY           = 0.5

------------------------------------------------
local function get_camera()
    local SHARED = rawget(_G, "SHARED")
    if SHARED and SHARED.POV and SHARED.POV.cam then
        return SHARED.POV.cam
    end
    return nil
end

local function get_pov_forward()
    local SHARED = rawget(_G, "SHARED")
    if not SHARED or not SHARED.POV then
        return 0.0, 0.0, 1.0
    end

    local yaw   = SHARED.POV.yaw   or 0.0
    local pitch = SHARED.POV.pitch or 0.0

    local radYaw   = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local cosPitch = math.cos(radPitch)

    local fx = cosPitch * math.sin(radYaw)
    local fy = math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    return fx, fy, fz
end

local function normalize(x, y, z)
    local len = math.sqrt(x * x + y * y + z * z)
    if len <= 0.0001 then return 0.0, 0.0, 0.0, 0.0 end
    return x / len, y / len, z / len, len
end

local function dot(ax, ay, az, bx, by, bz)
    return ax * bx + ay * by + az * bz
end

------------------------------------------------

local function show_girl()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(VISIBLE_SCALE_X, VISIBLE_SCALE_Y, VISIBLE_SCALE_Z)
        isGirlVisible = true
        if DEBUG then
            print("[EvidenceGirlLookAt] Girl shown")
        end
    end
end

local function hide_girl()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(0.0, 0.0, 0.0)
        isGirlVisible = false
        if DEBUG then
            print("[EvidenceGirlLookAt] Girl hidden")
        end
    end
end

local function start_tension()
    if not tensionPlaying then
        Audio_PlaySlot(EntityID, TENSION_SLOT)
        tensionPlaying = true
        if DEBUG then
            print("[EvidenceGirlLookAt] Tension sound started")
        end
    end
end

local function stop_tension()
    if tensionPlaying then
        Audio_StopSlot(EntityID, TENSION_SLOT)
        tensionPlaying = false
        if DEBUG then
            print("[EvidenceGirlLookAt] Tension sound stopped")
        end
    end
end

local function reset_look_state()
    isLooking         = false
    thresholdMet      = false
    stopLookingPlayed = false
    epicSoundPlayed   = false
    silenceTimer      = 0.0
    lookTimer         = 0.0

    stop_tension()

    Audio_SetSlotVolume(EntityID, LAUGH_SLOT, LAUGH_VOL_MIN)
    Audio_PlaySlot(EntityID, LAUGH_SLOT)
end

local function deactivate_look_state()
    isLooking         = false
    thresholdMet      = false
    stopLookingPlayed = false
    epicSoundPlayed   = false
    lookTimer         = 0.0
    silenceTimer      = 0.0

    stop_tension()

    if type(Audio_StopSlot) == "function" then
        Audio_StopSlot(EntityID, LAUGH_SLOT)
    end
end

local function force_player_look_at_girl()
    SHARED = SHARED or {}
    SHARED.POV = SHARED.POV or {}

    SHARED.POV.locked           = true
    SHARED.POV.lockTargetEntity = EntityID

    if DEBUG then
        print("[EvidenceGirlLookAt] Forced player to look at girl")
    end
end

local function activate_girl()
    if isActivated then
        return
    end

    isActivated = true
    show_girl()

    Audio_SetSlotVolume(EntityID, LAUGH_SLOT, LAUGH_VOL_MIN)
    Audio_PlaySlot(EntityID, LAUGH_SLOT)

    force_player_look_at_girl()

    if DEBUG then
        print("[EvidenceGirlLookAt] Activated from evidence completion")
    end
end

------------------------------------------------

function on_create(self)
    SHARED = SHARED or {}
    SHARED[ACTIVATION_FLAG] = SHARED[ACTIVATION_FLAG] or false

    isActivated       = false
    activationHandled = false
    isGirlVisible     = false

    tensionPlaying    = false
    isLooking         = false
    thresholdMet      = false
    stopLookingPlayed = false
    epicSoundPlayed   = false
    jinglePlayed      = false
    lookTimer         = 0.0
    silenceTimer      = 0.0
    frame             = 0
    rotateAfterPull   = false
    rotateTimer       = 0.0
    creditsCountdown  = false
    creditsTimer      = 0.0

    hide_girl()

    if DEBUG then
        print("[EvidenceGirlLookAt] Ready on entity " .. tostring(EntityID))
    end
end

function on_update(self, dt)
    dt = dt or 0.016

    if not activationHandled and SHARED and SHARED[ACTIVATION_FLAG] == true then
        activationHandled = true
        activate_girl()
    end

    if not isActivated then
        deactivate_look_state()
        return
    end

    if rotateAfterPull then
        rotateTimer = rotateTimer + dt
        if rotateTimer >= ROTATE_AFTER_PULL_DELAY then
            rotateAfterPull  = false
            Transform_SetRotation(EntityID, 0.0, 270.0, 0.0)
            creditsCountdown = true
            creditsTimer     = 0.0
            if DEBUG then
                print("[EvidenceGirlLookAt] Pull done -> rotated girl, starting credits countdown")
            end
        end
    end

    if creditsCountdown then
        creditsTimer = creditsTimer + dt
        if creditsTimer >= CREDITS_DELAY then
            creditsCountdown = false
            if DEBUG then
                print("[EvidenceGirlLookAt] Credits delay elapsed -> triggering ToEndCredit")
            end
            if StateMachine_SetTrigger then
                StateMachine_SetTrigger("ToEndCredit")
            end
        end
    end

    local cam    = get_camera()
    local girlTr = Query.GetTransform(EntityID)
    if not cam or not girlTr then return end

    local cx, cy, cz = cam:GetPosition()
    local gx, gy, gz = girlTr:GetPosition()
    gy = gy + GIRL_LOOK_HEIGHT_OFFSET

    local fx, fy, fz = get_pov_forward()

    local nx, ny, nz, dist = normalize(gx - cx, gy - cy, gz - cz)
    if dist <= 0.0001 then return end

    local d = dot(fx, fy, fz, nx, ny, nz)

    if DEBUG then
        frame = frame + 1
        if (frame % LOG_EVERY_N_FRAMES) == 0 then
            print(string.format(
                "[EvidenceGirlLookAt] dist=%.1f dot=%.4f looking=%s",
                dist, d, tostring(d >= LOOK_DOT_THRESHOLD)
            ))
        end
    end

    local inRange = dist <= MAX_DISTANCE

    if inRange and d >= LOOK_DOT_THRESHOLD then
        lookTimer = lookTimer + dt

        if not isLooking and lookTimer >= REQUIRED_LOOK_TIME then
            isLooking = true
            start_tension()

            if not jinglePlayed then
                jinglePlayed = true
                Audio_PlaySlot(EntityID, "SubtleJingle")
                if DEBUG then
                    print("[EvidenceGirlLookAt] Look confirmed -> play SubtleJingle")
                end
            end
        end

        if isLooking and not thresholdMet then
            Audio_SetSlotVolume(EntityID, LAUGH_SLOT, LAUGH_VOL_MAX)
        end

        if isLooking and not thresholdMet and lookTimer >= STOP_LOOKING_DELAY then
            thresholdMet = true
            silenceTimer = 0.0
            stop_tension()
            Audio_StopSlot(EntityID, LAUGH_SLOT)
            if DEBUG then
                print("[EvidenceGirlLookAt] Threshold reached -> silence starts")
            end
        end

        if thresholdMet then
            silenceTimer = silenceTimer + dt

            if not epicSoundPlayed and silenceTimer >= EPIC_SOUND_OFFSET then
                epicSoundPlayed = true
                Audio_PlaySlot(EntityID, "EpicSound")
                Audio_PlaySlot(EntityID, "Lightning")
            end

            if not stopLookingPlayed and silenceTimer >= SILENCE_BEFORE_JINGLE then
                stopLookingPlayed = true
                Audio_PlaySlot(EntityID, "StopLooking")

                SHARED = SHARED or {}
                SHARED.POV = SHARED.POV or {}
                SHARED.POV.locked           = true
                SHARED.POV.lockTargetEntity = EntityID
                SHARED.POV.shake            = true
                SHARED.POV.shakeDuration    = 0.8
                SHARED.POV.shakeAmplitude   = 18.0
                SHARED.POV.shakeSpeed       = 0.01
                SHARED.POV.shakePullAmount  = 1500.0

                rotateAfterPull = true
                rotateTimer     = 0.0
            end
        end
    else
        if isLooking or tensionPlaying or lookTimer > 0.0 then
            reset_look_state()
            if DEBUG then
                print("[EvidenceGirlLookAt] LOOK END -> reset")
            end
        else
            lookTimer = 0.0
            stop_tension()
        end
    end
end

function on_destroy(self)
    stop_tension()
    isActivated       = false
    activationHandled = false
    isLooking         = false
    thresholdMet      = false
    stopLookingPlayed = false
    epicSoundPlayed   = false
    jinglePlayed      = false
    lookTimer         = 0.0
    silenceTimer      = 0.0
    creditsCountdown  = false
    creditsTimer      = 0.0
end
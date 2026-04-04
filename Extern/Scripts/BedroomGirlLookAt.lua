----------------------------------------------------------------
-- BedroomGirlLookAt.lua
-- Attach to the bedroom girl entity.
-- Audio stops while the player is looking at her, resumes when they look away.
----------------------------------------------------------------

local DEBUG               = true
local LOG_EVERY_N_FRAMES  = 15

local MAX_DISTANCE            = 3000.0
local LOOK_DOT_THRESHOLD      = 0.93
local REQUIRED_LOOK_TIME      = 0.10   -- seconds to confirm look (starts ramp)
local STOP_LOOKING_DELAY      = 6.0    -- seconds of continuous look before silence + StopLooking
local SILENCE_BEFORE_JINGLE   = 0.0    -- seconds of silence after threshold before StopLooking plays
local EPIC_SOUND_OFFSET       = 0.0    -- seconds into silence when EpicSound plays (before StopLooking)
local GIRL_LOOK_HEIGHT_OFFSET = 60.0

local CRY_SLOTS = {
    { name = "Cry_01", duration = 3.5 },
    { name = "Cry_02", duration = 2.6 },
    { name = "Cry_03", duration = 2.1 },
    { name = "Cry_04", duration = 1.8 },
    { name = "Cry_05", duration = 1.4 }
}

local currentCryIndex = -1
local cryTimer = 0.0
local cryDuration = 0.0
local currentCrySlot = nil

local TENSION_SLOT     = "Tension"
local LAUGH_VOL_MIN    = 0.3   -- volume when player is not looking
local LAUGH_VOL_MAX    = 1.0   -- volume at full 3s look threshold

local TRIGGER_ZONE_ID  = 991   -- player must be inside this zone for look to activate

local tensionPlaying   = false
local isLooking         = false   -- player is currently looking at girl
local thresholdMet      = false   -- 3s look threshold reached; silence delay counting
local stopLookingPlayed = false   -- StopLooking already fired this look session
local epicSoundPlayed   = false   -- EpicSound already fired this look session
local lookTimer         = 0.0
local silenceTimer      = 0.0
local frame             = 0

local TENSION_COOLDOWN     = 3.0   -- seconds before tension can retrigger after stopping
local LOOK_DECAY_RATE      = 2.0   -- how many times faster lookTimer drains vs accumulates
local tensionCooldownTimer = 0.0

local VISIBLE_SCALE_X = 1.0
local VISIBLE_SCALE_Y = 1.0
local VISIBLE_SCALE_Z = 1.0

local isGirlVisible = false

local rotateAfterPull       = false   -- waiting to rotate girl after pull finishes
local rotateTimer           = 0.0
local ROTATE_AFTER_PULL_DELAY = 1.2   -- matches shakeDuration

-- Post-death zoom-in phase
local zoomAfterDeath   = false
local zoomTimer        = 0.0
local zoomTriggered      = false
local screamPlayed       = false
local preScreamShaken    = false
local screamShakeStopped = false
local ZOOM_DELAY          = 0.2   -- seconds after girl rotates before shift kicks in
local ZOOM_SHIFT_DURATION = 0.8   -- must match camShiftDuration below
local PRE_SCREAM_SHAKE_OFFSET = 0.0   -- shake triggers at same time as scream
local SCREAM_DURATION    = 2.0   -- match this to the length of your Scream audio clip

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

local function start_tension()
    if not tensionPlaying then
        Audio_PlaySlot(EntityID, TENSION_SLOT)
        tensionPlaying = true
        if DEBUG then
            print("[BedroomGirlLookAt] Tension sound started")
        end
    end
end

local function stop_tension()
    if tensionPlaying then
        Audio_StopSlot(EntityID, TENSION_SLOT)
        tensionPlaying = false
        tensionCooldownTimer = TENSION_COOLDOWN
        if DEBUG then
            print("[BedroomGirlLookAt] Tension sound stopped")
        end
    end
end

local function play_random_cry()
    if #CRY_SLOTS == 0 then return end

    local index
    repeat
        index = math.random(1, #CRY_SLOTS)
    until index ~= currentCryIndex

    currentCryIndex = index
    local cry = CRY_SLOTS[index]

    currentCrySlot = cry.name
    cryDuration = cry.duration
    cryTimer = 0.0

    Audio_SetSlotVolume(EntityID, currentCrySlot, LAUGH_VOL_MIN)
    Audio_PlaySlot(EntityID, currentCrySlot)

    if DEBUG then
        print("[BedroomGirlLookAt] Playing cry:", currentCrySlot)
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

    ------------------------------------------------
    -- Do NOT stop or restart the crying sequence here.
    -- Crying should continue naturally regardless of
    -- whether the player is looking at her or not.
    ------------------------------------------------

    if SHARED and SHARED.POV then
        SHARED.POV.shake = false
        SHARED.POV.freeShakeSustain = false
    end
end

local function deactivate_look_state()
    isLooking         = false
    thresholdMet      = false
    stopLookingPlayed = false
    epicSoundPlayed   = false
    lookTimer         = 0.0
    silenceTimer      = 0.0

    stop_tension()

    ------------------------------------------------
    -- Do NOT stop the crying sequence here.
    -- Let the room-level active/inactive logic decide that.
    ------------------------------------------------

    if SHARED and SHARED.POV then
        SHARED.POV.shake = false
        SHARED.POV.freeShake = false
        SHARED.POV.freeShakeSustain = false
    end
end

local function show_girl()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(VISIBLE_SCALE_X, VISIBLE_SCALE_Y, VISIBLE_SCALE_Z)
        isGirlVisible = true
        if DEBUG then
            print("[BedroomGirlLookAt] Girl shown")
        end
    end
end

local function hide_girl()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(0.0, 0.0, 0.0)
        isGirlVisible = false
        if DEBUG then
            print("[BedroomGirlLookAt] Girl hidden")
        end
    end
end

------------------------------------------------

function on_create(self)
    SHARED = SHARED or {}
    SHARED.BEDROOM_GIRL_ACTIVE = SHARED.BEDROOM_GIRL_ACTIVE or false
    SHARED.BEDROOM_CP_REACHED = SHARED.BEDROOM_CP_REACHED or false
    SHARED.BEDROOM_GIRL_DONE = SHARED.BEDROOM_GIRL_DONE or false

    isLooking            = false
    thresholdMet         = false
    stopLookingPlayed    = false
    epicSoundPlayed      = false
    tensionPlaying       = false
    lookTimer            = 0.0
    silenceTimer         = 0.0
    tensionCooldownTimer = 0.0
    frame                = 0
    isGirlVisible        = false

    hide_girl()

    if DEBUG then
        print("[BedroomGirlLookAt] Ready on entity " .. tostring(EntityID))
    end
end

function on_update(self, dt)
    dt = dt or 0.016

    if tensionCooldownTimer > 0.0 then
        tensionCooldownTimer = tensionCooldownTimer - dt
    end

    local girlDone   = SHARED and SHARED.BEDROOM_GIRL_DONE == true
    local girlActive = SHARED and SHARED.BEDROOM_GIRL_ACTIVE == true and not girlDone

    local inZone = SHARED and SHARED.INTERACT_IN_ZONE and SHARED.INTERACT_IN_ZONE[TRIGGER_ZONE_ID] == true

    if girlActive and not isGirlVisible then
        show_girl()
    elseif ((not girlActive) or girlDone) and isGirlVisible then
        hide_girl()
    end

    ------------------------------------------------
    -- HARD STOP if player NOT in bedroom / girl inactive
    ------------------------------------------------
    if not inZone or (not girlActive) or girlDone then
        if currentCrySlot and type(Audio_StopSlot) == "function" then
            Audio_StopSlot(EntityID, currentCrySlot)
        end

        currentCrySlot = nil
        deactivate_look_state()
        return
    end

    ------------------------------------------------
    -- START CRY when activated
    ------------------------------------------------
    if SHARED and SHARED.BEDROOM_START_CRY == true then
        SHARED.BEDROOM_START_CRY = false

        if currentCrySlot and type(Audio_StopSlot) == "function" then
            Audio_StopSlot(EntityID, currentCrySlot)
        end

        play_random_cry()

        if DEBUG then
            print("[BedroomGirlLookAt] Initial cry started")
        end
    end

    ------------------------------------------------
    -- CONTINUOUS CRY LOOP
    -- Keep crying sequence running regardless of whether
    -- the player is looking at her or not, until the
    -- actual threshold event stops it.
    ------------------------------------------------
    if not thresholdMet then
        if currentCrySlot == nil then
            play_random_cry()
        else
            cryTimer = cryTimer + dt

            if cryTimer >= cryDuration then
                if currentCrySlot and type(Audio_StopSlot) == "function" then
                    Audio_StopSlot(EntityID, currentCrySlot)
                end

                play_random_cry()
            end
        end
    end

    -- Rotate girl to 270 degrees after pull finishes
    if rotateAfterPull then
        rotateTimer = rotateTimer + dt
        if rotateTimer >= ROTATE_AFTER_PULL_DELAY then
            rotateAfterPull = false
            Transform_SetRotation(EntityID, 0.0, 270.0, 0.0)
            SHARED = SHARED or {}
            SHARED.PLAYER_DEAD = true

            -- Begin post-death shift phase (scream plays once shift completes)
            zoomAfterDeath   = true
            zoomTimer        = 0.0
            zoomTriggered    = false
            screamPlayed     = false
            preScreamShaken  = false
            screamShakeStopped = false

            if DEBUG then
                print("[BedroomGirlLookAt] Pull done -> rotated girl to 270 degrees, signalling game over")
            end
        end
    end

    -- Shift camera up from its current pulled position after she turns
    if zoomAfterDeath then
        zoomTimer = zoomTimer + dt
        if not zoomTriggered and zoomTimer >= ZOOM_DELAY then
            zoomTriggered = true
            SHARED = SHARED or {}
            SHARED.POV = SHARED.POV or {}
            SHARED.POV.camShift         = true
            SHARED.POV.camShiftUp       = 5.0   -- world units upward from pulled position
            SHARED.POV.camShiftForward  = 9.0   -- push forward slightly for zoom
            SHARED.POV.camShiftDuration = ZOOM_SHIFT_DURATION
            if DEBUG then
                print("[BedroomGirlLookAt] Death camera shift triggered")
            end
        end

        -- Vigorous sustained shake just before the scream
        if not preScreamShaken and zoomTimer >= ZOOM_DELAY + ZOOM_SHIFT_DURATION - PRE_SCREAM_SHAKE_OFFSET then
            preScreamShaken = true
            SHARED = SHARED or {}
            SHARED.POV = SHARED.POV or {}
            SHARED.POV.freeShakeSustain   = true
            SHARED.POV.freeShakeAmplitude = 40.0
            SHARED.POV.freeShakeSpeed     = 0.018
            if DEBUG then
                print("[BedroomGirlLookAt] Pre-scream shake triggered")
            end
        end

        -- Scream plays once the shift has fully completed
        if not screamPlayed and zoomTimer >= ZOOM_DELAY + ZOOM_SHIFT_DURATION then
            screamPlayed = true
            Audio_PlaySlot(EntityID, "Scream")
            if DEBUG then
                print("[BedroomGirlLookAt] Scream triggered")
            end
        end

        -- Stop shake when scream finishes
        if screamPlayed and not screamShakeStopped and zoomTimer >= ZOOM_DELAY + ZOOM_SHIFT_DURATION + SCREAM_DURATION then
            screamShakeStopped = true
            SHARED = SHARED or {}
            SHARED.POV = SHARED.POV or {}
            SHARED.POV.freeShakeSustain = false
            if DEBUG then
                print("[BedroomGirlLookAt] Scream shake stopped")
            end
        end
    end

    -- Only run look detection if player is inside the trigger zone
    local inZone = SHARED and SHARED.INTERACT_IN_ZONE and SHARED.INTERACT_IN_ZONE[TRIGGER_ZONE_ID] == true
    if not inZone then
        if isLooking or tensionPlaying or lookTimer > 0.0 then
            reset_look_state()
        else
            lookTimer = 0.0
            stop_tension()
        end
        return
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
                "[BedroomGirlLookAt] dist=%.1f dot=%.4f looking=%s",
                dist, d, tostring(d >= LOOK_DOT_THRESHOLD)
            ))
        end
    end

    local inRange = dist <= MAX_DISTANCE

    if inRange and d >= LOOK_DOT_THRESHOLD then
        lookTimer = lookTimer + dt

        -- Confirm look after brief delay
        if not isLooking and lookTimer >= REQUIRED_LOOK_TIME then
            isLooking = true
        end

        -- Delay tension audio until player has been looking for 1 second,
        -- and only if the cooldown from the last stop has expired
        if isLooking and not tensionPlaying and lookTimer >= 1.0 and tensionCooldownTimer <= 0.0 then
            start_tension()
        end

        -- Ramp cry + camera shake intensity as player stares
        if isLooking and not thresholdMet then
            ------------------------------------------------
            -- AUDIO INTENSITY
            ------------------------------------------------
            local t = math.min(lookTimer / STOP_LOOKING_DELAY, 1.0)

            local vol = LAUGH_VOL_MIN + (LAUGH_VOL_MAX - LAUGH_VOL_MIN) * t
            if currentCrySlot then
                Audio_SetSlotVolume(EntityID, currentCrySlot, vol)
            end

            ------------------------------------------------
            -- CAMERA SHAKE
            ------------------------------------------------
            SHARED = SHARED or {}
            SHARED.POV = SHARED.POV or {}

            SHARED.POV.freeShakeSustain = true

            local tSmooth = t * t

            SHARED.POV.freeShakeAmplitude = 0.3 + (2.2 * tSmooth)
            SHARED.POV.freeShakeSpeed = 0.08 - (0.05 * tSmooth)

            if SHARED.POV.freeShakeSpeed < 0.02 then
                SHARED.POV.freeShakeSpeed = 0.02
            end

            if DEBUG then
                print(string.format(
                    "[BedroomGirlLookAt] Subtle shake t=%.2f amp=%.2f speed=%.3f",
                    tSmooth,
                    SHARED.POV.freeShakeAmplitude,
                    SHARED.POV.freeShakeSpeed
                ))
            end
        end

        -- Threshold reached: stop cry, begin silence
        if isLooking and not thresholdMet and lookTimer >= STOP_LOOKING_DELAY then
            thresholdMet = true
            silenceTimer = 0.0
            stop_tension()
            if currentCrySlot then
                Audio_StopSlot(EntityID, currentCrySlot)
            end
            currentCrySlot = nil
            if DEBUG then
                print("[BedroomGirlLookAt] Threshold reached -> stop cry, silence starts")
            end
        end

        -- Silence countdown: EpicSound / StopLooking
        if thresholdMet then
            silenceTimer = silenceTimer + dt

            if not epicSoundPlayed and silenceTimer >= EPIC_SOUND_OFFSET then
                epicSoundPlayed = true
                Audio_PlaySlot(EntityID, "EpicSound")
                Audio_PlaySlot(EntityID, "Lightning")
                if DEBUG then
                    print("[BedroomGirlLookAt] EpicSound triggered")
                end
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

                if DEBUG then
                    print("[BedroomGirlLookAt] StopLooking + lock + shake triggered")
                end
            end
        end
    else
        -- Player left cone: decay timer slowly instead of instant reset
        if isLooking then
            isLooking = false
            stop_tension()
            if SHARED and SHARED.POV then
                SHARED.POV.freeShakeSustain = false
            end
            if DEBUG then
                print("[BedroomGirlLookAt] LOOK END -> decaying timer")
            end
        end

        if lookTimer > 0.0 then
            lookTimer = lookTimer - (dt * LOOK_DECAY_RATE)
            if lookTimer <= 0.0 then
                lookTimer = 0.0
                thresholdMet      = false
                stopLookingPlayed = false
                epicSoundPlayed   = false
                silenceTimer      = 0.0

                ------------------------------------------------
                -- Do NOT stop or restart cry here.
                -- Keep her crying sequence going naturally.
                ------------------------------------------------

                if DEBUG then
                    print("[BedroomGirlLookAt] Timer fully drained -> look reset only")
                end
            end
        elseif tensionPlaying then
            stop_tension()
        end
    end
end

function on_destroy(self)
    stop_tension()
    isLooking         = false
    thresholdMet      = false
    stopLookingPlayed = false
    epicSoundPlayed   = false
    lookTimer         = 0.0
    silenceTimer      = 0.0
end

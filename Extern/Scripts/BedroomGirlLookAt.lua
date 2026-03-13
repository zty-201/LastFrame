----------------------------------------------------------------
-- BedroomGirlLookAt.lua
-- Attach to the bedroom girl entity.
-- Audio stops while the player is looking at her, resumes when they look away.
----------------------------------------------------------------

local DEBUG               = true
local LOG_EVERY_N_FRAMES  = 15

local MAX_DISTANCE            = 3000.0
local LOOK_DOT_THRESHOLD      = 0.95
local REQUIRED_LOOK_TIME      = 0.10   -- seconds to confirm look (plays SubtleJingle, starts ramp)
local STOP_LOOKING_DELAY      = 6.0    -- seconds of continuous look before silence + StopLooking
local SILENCE_BEFORE_JINGLE   = 0.0    -- seconds of silence after threshold before StopLooking plays
local EPIC_SOUND_OFFSET       = 0.0    -- seconds into silence when EpicSound plays (before StopLooking)
local GIRL_LOOK_HEIGHT_OFFSET = 60.0

local LAUGH_SLOT       = "GirlLaughing"
local TENSION_SLOT     = "Tension"
local LAUGH_VOL_MIN    = 0.3   -- volume when player is not looking
local LAUGH_VOL_MAX    = 1.0   -- volume at full 3s look threshold

local TRIGGER_ZONE_ID  = 991   -- player must be inside this zone for look to activate

local tensionPlaying   = false
local isLooking         = false   -- player is currently looking at girl
local thresholdMet      = false   -- 3s look threshold reached; silence delay counting
local stopLookingPlayed = false   -- StopLooking already fired this look session
local epicSoundPlayed   = false   -- EpicSound already fired this look session
local jinglePlayed      = false   -- SubtleJingle fires only once ever
local lookTimer         = 0.0
local silenceTimer      = 0.0
local frame             = 0

local VISIBLE_SCALE_X = 1.0
local VISIBLE_SCALE_Y = 1.0
local VISIBLE_SCALE_Z = 1.0

local isGirlVisible = false

local rotateAfterPull       = false   -- waiting to rotate girl after pull finishes
local rotateTimer           = 0.0
local ROTATE_AFTER_PULL_DELAY = 1.2   -- matches shakeDuration

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
        if DEBUG then
            print("[BedroomGirlLookAt] Tension sound stopped")
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

    isLooking         = false
    thresholdMet      = false
    stopLookingPlayed = false
    epicSoundPlayed   = false
    jinglePlayed      = false
    tensionPlaying    = false
    lookTimer         = 0.0
    silenceTimer      = 0.0
    frame             = 0
    isGirlVisible     = false

    hide_girl()

    if DEBUG then
        print("[BedroomGirlLookAt] Ready on entity " .. tostring(EntityID))
    end
end

function on_update(self, dt)
    dt = dt or 0.016

    local girlDone   = SHARED and SHARED.BEDROOM_GIRL_DONE == true
    local girlActive = SHARED and SHARED.BEDROOM_GIRL_ACTIVE == true and not girlDone

    if girlActive and not isGirlVisible then
        show_girl()
    elseif ((not girlActive) or girlDone) and isGirlVisible then
        hide_girl()
    end

    if (not girlActive) or girlDone then
        deactivate_look_state()
        return
    end

    -- Rotate girl to 270 degrees after pull finishes
    if rotateAfterPull then
        rotateTimer = rotateTimer + dt
        if rotateTimer >= ROTATE_AFTER_PULL_DELAY then
            rotateAfterPull = false
            Transform_SetRotation(EntityID, 0.0, 270.0, 0.0)
            if DEBUG then
                print("[BedroomGirlLookAt] Pull done -> rotated girl to 270 degrees")
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

        -- Confirm look (0.10s): play SubtleJingle once, mark looking
        if not isLooking and lookTimer >= REQUIRED_LOOK_TIME then
            isLooking = true
            start_tension()

            if not jinglePlayed then
                jinglePlayed = true
                Audio_PlaySlot(EntityID, "SubtleJingle")
                if DEBUG then
                    print("[BedroomGirlLookAt] Look confirmed -> play SubtleJingle")
                end
            end
        end

        -- Ramp laugh volume up toward 3s threshold
        if isLooking and not thresholdMet then
            local t = math.min(lookTimer / STOP_LOOKING_DELAY, 1.0)
            local vol = LAUGH_VOL_MAX
            Audio_SetSlotVolume(EntityID, LAUGH_SLOT, vol)
        end

        -- 3s threshold: stop laugh, begin silence
        if isLooking and not thresholdMet and lookTimer >= STOP_LOOKING_DELAY then
            thresholdMet = true
            silenceTimer = 0.0
            stop_tension()
            Audio_StopSlot(EntityID, LAUGH_SLOT)
            if DEBUG then
                print("[BedroomGirlLookAt] 3s threshold -> stop laugh, silence starts")
            end
        end

        -- Silence countdown: EpicSound at 0.5s, StopLooking at 1s
        if thresholdMet then
            silenceTimer = silenceTimer + dt

            if not epicSoundPlayed and silenceTimer >= EPIC_SOUND_OFFSET then
                epicSoundPlayed = true
                Audio_PlaySlot(EntityID, "EpicSound")
                Audio_PlaySlot(EntityID, "Lightning")
                if DEBUG then
                    print("[BedroomGirlLookAt] 0.5s -> play EpicSound")
                end
            end

            if not stopLookingPlayed and silenceTimer >= SILENCE_BEFORE_JINGLE then
                stopLookingPlayed = true
                Audio_PlaySlot(EntityID, "StopLooking")
                -- Lock camera onto the doll + violent shake
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
                    print("[BedroomGirlLookAt] 1s -> play StopLooking + lock + shake")
                end
            end
        end
    else
        if isLooking or tensionPlaying or lookTimer > 0.0 then
            reset_look_state()
            if DEBUG then
                print("[BedroomGirlLookAt] LOOK END -> reset tension + resume laugh")
            end
        else
            lookTimer = 0.0
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
    jinglePlayed      = false
    lookTimer         = 0.0
    silenceTimer      = 0.0
end

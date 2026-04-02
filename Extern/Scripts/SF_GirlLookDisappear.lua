----------------------------------------------------------------
-- SF_GirlLookDisappear.lua
-- Girl starts hidden.
-- She becomes visible only after the canteen door is opened.
-- Then if the player looks at her, lightning triggers and she disappears.
----------------------------------------------------------------

local DEBUG = true
local DEBUG_LOOK = true
local LOG_EVERY_N_FRAMES = 15

local MAX_DISTANCE = 3000.0
local LOOK_DOT_THRESHOLD = 0.85
local REQUIRED_LOOK_TIME = 1.0
local GIRL_LOOK_HEIGHT_OFFSET = 60.0

-- SET THESE TO THE GIRL'S ACTUAL VISIBLE SCALE
local VISIBLE_SCALE_X = 1.0
local VISIBLE_SCALE_Y = 1.0
local VISIBLE_SCALE_Z = 1.0

local hasTriggered = false
local isVisible = false
local lookTimer = 0.0
local frame = 0

local SCARE_SOUND_ID = "ScareSound"
local THUNDER_SFX_ID = "Thunder"
local LAUGH_SFX_ID   = "GirlLaughing"

local VO_REACTION_SFX_ID = "VoiceOver"
local VO_DELAY = 1.5

local pendingVO = false
local voTimer = 0.0

------------------------------------------------
local function get_camera()
    local SHARED = rawget(_G, "SHARED")
    if SHARED and SHARED.POV and SHARED.POV.cam then
        return SHARED.POV.cam
    end
    return nil
end

------------------------------------------------
local function get_pov_forward()
    local SHARED = rawget(_G, "SHARED")
    if not SHARED or not SHARED.POV then
        return 0.0, 0.0, 1.0
    end

    local yaw = SHARED.POV.yaw or 0.0
    local pitch = SHARED.POV.pitch or 0.0

    local radYaw   = math.rad(yaw)
    local radPitch = math.rad(pitch)
    local cosPitch = math.cos(radPitch)

    local fx = cosPitch * math.sin(radYaw)
    local fy = math.sin(radPitch)
    local fz = cosPitch * math.cos(radYaw)

    return fx, fy, fz
end

------------------------------------------------
local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId)
        return true
    end
    return false
end

------------------------------------------------
local function normalize(x, y, z)
    local len = math.sqrt(x * x + y * y + z * z)
    if len <= 0.0001 then
        return 0.0, 0.0, 0.0, 0.0
    end
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
        isVisible = true

        if DEBUG then
            local sx, sy, sz = tr:GetScale()
            print(string.format(
                "[GhostGirl] show_girl -> scale set to (%.2f, %.2f, %.2f)",
                sx, sy, sz
            ))
        end
    end
end

local function hide_girl()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(0.0, 0.0, 0.0)
        isVisible = false
    end
end

------------------------------------------------
local function trigger_scare()
    if hasTriggered then return end
    hasTriggered = true

    hide_girl()

    -- Lightning trigger
    SHARED = SHARED or {}
    SHARED.LIGHTNING_CMD = SHARED.LIGHTNING_CMD or {}
    SHARED.LIGHTNING_EVENT_ID = (SHARED.LIGHTNING_EVENT_ID or 0) + 1

    SHARED.LIGHTNING_CMD["GHOST_GIRL_SCARE"] = {
        trigger = true,
        by = EntityID,
        eventId = SHARED.LIGHTNING_EVENT_ID
    }

    -- Play scare sounds
    play_3d_sfx(SCARE_SOUND_ID)
    play_3d_sfx(THUNDER_SFX_ID)
    play_3d_sfx(LAUGH_SFX_ID)

    -- Start delayed voice reaction
    pendingVO = true
    voTimer = 0.0

    if DEBUG then
        print("[GhostGirl] Scare triggered + sounds played + VO scheduled")
    end
end

------------------------------------------------
function on_create(self)
    SHARED = SHARED or {}
    SHARED.LIGHTNING_CMD = SHARED.LIGHTNING_CMD or {}
    SHARED.SF_GIRL_ACTIVE = SHARED.SF_GIRL_ACTIVE or false

    hasTriggered = false
    isVisible = false
    lookTimer = 0.0
    frame = 0

    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(0.0, 0.0, 0.0)
    end

    if DEBUG then
        print("[GhostGirl] Ready EntityID=" .. tostring(EntityID) .. " (starts hidden)")
    end
end

------------------------------------------------
function on_update(self, dt)
    dt = dt or 0.016

    -- Delayed voice reaction must still run even after scare has triggered
    if pendingVO then
        voTimer = voTimer + dt
        if voTimer >= VO_DELAY then
            pendingVO = false
            voTimer = 0.0
            play_3d_sfx(VO_REACTION_SFX_ID)

            if DEBUG then
                print("[GhostGirl] VoiceReaction played after delay")
            end
        end
    end

    -- After scare has triggered, stop the look-detection logic
    -- but do NOT stop the delayed VO logic above
    if hasTriggered then
        return
    end

    if SHARED and SHARED.SF_GIRL_ACTIVE == true and not isVisible then
        show_girl()
        if DEBUG then
            print("[GhostGirl] Girl revealed")
        end
    end

    if not isVisible then
        return
    end

    local cam = get_camera()
    local girlTr = Query.GetTransform(EntityID)
    if not cam or not girlTr then return end

    local cx, cy, cz = cam:GetPosition()
    local gx, gy, gz = girlTr:GetPosition()
    gy = gy + GIRL_LOOK_HEIGHT_OFFSET

    local fx, fy, fz = get_pov_forward()

    local nx, ny, nz, dist = normalize(gx - cx, gy - cy, gz - cz)
    if dist <= 0.0001 then return end

    local d = dot(fx, fy, fz, nx, ny, nz)

    if DEBUG_LOOK then
        frame = frame + 1
        if (frame % LOG_EVERY_N_FRAMES) == 0 then
            print(string.format(
                "[GhostGirlLook] dist=%.1f dot=%.4f looking=%s",
                dist, d, tostring(d >= LOOK_DOT_THRESHOLD)
            ))
        end
    end

    if dist > MAX_DISTANCE then
        lookTimer = 0.0
        return
    end

    if d >= LOOK_DOT_THRESHOLD then
        lookTimer = lookTimer + dt

        if lookTimer >= REQUIRED_LOOK_TIME then
            trigger_scare()
        end
    else
        lookTimer = 0.0
    end
end

function on_destroy(self)
end
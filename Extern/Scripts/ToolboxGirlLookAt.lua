----------------------------------------------------------------
-- ToolboxGirlLookAt.lua
-- Attach to entity 1007.
-- Triggered automatically after all 3 candles blow out
-- (LockManager sets SHARED.TOOLBOX_JUMPSCARE = true).
-- Immediately shows the entity, forces the camera to look at it,
-- plays the scare audio sequence, then kills the player.
----------------------------------------------------------------

local DEBUG = true

-- How long the laugh/tension builds before the pull+death
local BUILD_TIME = 3.0

local LAUGH_SLOT   = "GirlLaughing"
local TENSION_SLOT = "Tension"
local LAUGH_VOL_MIN = 0.3
local LAUGH_VOL_MAX = 1.0

local VISIBLE_SCALE_X = 1.0
local VISIBLE_SCALE_Y = 1.0
local VISIBLE_SCALE_Z = 1.0

local ROTATE_AFTER_PULL_DELAY = 1.2

local isActivated     = false
local buildTimer      = 0.0
local pullTriggered   = false
local rotateAfterPull = false
local rotateTimer     = 0.0
local tensionPlaying  = false

------------------------------------------------
local function show_entity()
    local tr = Query.GetTransform(EntityID)
    if tr then tr:SetScale(VISIBLE_SCALE_X, VISIBLE_SCALE_Y, VISIBLE_SCALE_Z) end
end

local function hide_entity()
    local tr = Query.GetTransform(EntityID)
    if tr then tr:SetScale(0.0, 0.0, 0.0) end
end

local function start_tension()
    if not tensionPlaying then
        Audio_PlaySlot(EntityID, TENSION_SLOT)
        tensionPlaying = true
    end
end

local function stop_tension()
    if tensionPlaying then
        Audio_StopSlot(EntityID, TENSION_SLOT)
        tensionPlaying = false
    end
end

local function activate()
    if isActivated then return end
    isActivated = true

    show_entity()

    -- Force camera to look at this entity
    SHARED = SHARED or {}
    SHARED.POV = SHARED.POV or {}
    SHARED.POV.locked           = true
    SHARED.POV.lockTargetEntity = EntityID

    -- Start laugh + tension
    Audio_SetSlotVolume(EntityID, LAUGH_SLOT, LAUGH_VOL_MIN)
    Audio_PlaySlot(EntityID, LAUGH_SLOT)
    start_tension()

    buildTimer    = 0.0
    pullTriggered = false

    if DEBUG then print("[ToolboxGirlLookAt] Activated on entity " .. tostring(EntityID)) end
end

------------------------------------------------

function on_create(self)
    SHARED = SHARED or {}
    SHARED.TOOLBOX_JUMPSCARE = false

    isActivated     = false
    buildTimer      = 0.0
    pullTriggered   = false
    rotateAfterPull = false
    rotateTimer     = 0.0
    tensionPlaying  = false

    hide_entity()

    if DEBUG then print("[ToolboxGirlLookAt] Ready on entity " .. tostring(EntityID)) end
end

function on_update(self, dt)
    dt = dt or 0.016

    -- Wait for LockManager to fire the jumpscare
    if not isActivated then
        if SHARED and SHARED.TOOLBOX_JUMPSCARE == true then
            activate()
        end
        return
    end

    -- Rotate after pull completes, then signal death
    if rotateAfterPull then
        rotateTimer = rotateTimer + dt
        if rotateTimer >= ROTATE_AFTER_PULL_DELAY then
            rotateAfterPull = false
            SHARED = SHARED or {}
            SHARED.PLAYER_DEAD = true
            if DEBUG then print("[ToolboxGirlLookAt] Pull done -> PLAYER_DEAD") end
        end
        return
    end

    if pullTriggered then return end

    -- Ramp up laugh volume over BUILD_TIME
    buildTimer = buildTimer + dt
    local t = math.min(buildTimer / BUILD_TIME, 1.0)
    Audio_SetSlotVolume(EntityID, LAUGH_SLOT, LAUGH_VOL_MIN + (LAUGH_VOL_MAX - LAUGH_VOL_MIN) * t)

    -- After BUILD_TIME: trigger the pull + death sequence
    if buildTimer >= BUILD_TIME then
        pullTriggered = true
        stop_tension()
        Audio_StopSlot(EntityID, LAUGH_SLOT)
        Audio_PlaySlot(EntityID, "EpicSound")
        Audio_PlaySlot(EntityID, "StopLooking")

        SHARED = SHARED or {}
        SHARED.POV = SHARED.POV or {}
        SHARED.POV.locked           = true
        SHARED.POV.lockTargetEntity = EntityID
        SHARED.POV.shake            = true
        SHARED.POV.shakeDuration    = 0.8
        SHARED.POV.shakeAmplitude   = 18.0
        SHARED.POV.shakeSpeed       = 0.01
        SHARED.POV.shakePullAmount  = 410.0

        rotateAfterPull = true
        rotateTimer     = 0.0

        if DEBUG then print("[ToolboxGirlLookAt] BUILD_TIME reached -> pull + death") end
    end
end

function on_destroy(self)
    stop_tension()
end

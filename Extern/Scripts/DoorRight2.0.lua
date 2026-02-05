-- Door_SharedControlled_Right.lua
-- RIGHT door opens ONCE, then permanently ignores interaction.
-- Main doors (Entity 26 / 509) additionally support scripted CLOSE + door close SFX + giggle.

local closedAngle = 0.0
local openAngle   = 80.0
local rotateSpeed = 20.0

local isOpen = false
local currentY = 0.0
local targetY = 0.0

local hasOpenedOnce = false

-- =========================
-- MAIN DOOR SPECIAL CASE
-- =========================
local MAIN_DOOR_IDS = {
    [26]  = true,
    [509] = true
}

-- Open SFX
local DOOR_OPEN_SFX_ID      = "OpenDoor"      -- normal doors
local MAINDOOR_OPEN_SFX_ID  = "OpenMaindoor"  -- doors 26/509

-- Close + giggle (main doors only)
local DOOR_CLOSE_SFX_ID  = "CloseMaindoor"
local GIRL_GIGGLE_SFX_ID = "GirlGiggle"
local GIGGLE_DELAY_SECONDS = 0.25

local gigglePending = false
local giggleTimer = 0.0

local function is_main_door()
    return MAIN_DOOR_IDS[EntityID] == true
end

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and type(slotId) == "string" then
        Audio_PlaySlot(EntityID, slotId) -- 3D sound from this door entity
        return true
    end
    return false
end

function on_create(Self)
    print("[DoorR] on_create EntityID:", EntityID)

    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[DoorR] ERROR: No Transform found")
        return
    end

    local _, ry, _ = tr:GetRotation()
    currentY = ry
    targetY = ry

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}

    SHARED.DOOR_OPENED_ONCE = SHARED.DOOR_OPENED_ONCE or {}
    SHARED.DOOR_OPENED_ONCE[EntityID] = false

    print(string.format("[DoorR] Start RotY=%.2f", currentY))
end

function on_update(Self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end

    -- =========================
    -- main door: delayed giggle
    -- =========================
    if gigglePending then
        giggleTimer = giggleTimer + dt
        if giggleTimer >= GIGGLE_DELAY_SECONDS then
            gigglePending = false
            giggleTimer = 0.0
            play_3d_sfx(GIRL_GIGGLE_SFX_ID)
        end
    end

    -- =========================
    -- consume door commands
    -- =========================
    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil

        local cmd = cmdObj.cmd

        -- After opened once:
        if hasOpenedOnce then
            -- Only MAIN doors allow scripted close after opening once
            if cmd == "close" and is_main_door() then
                isOpen = false
                targetY = closedAngle
                print("[DoorR] Main door scripted CLOSE allowed after one-time open")

                -- Close SFX + giggle chain (main door only)
                play_3d_sfx(DOOR_CLOSE_SFX_ID)
                gigglePending = true
                giggleTimer = 0.0
            else
                print("[DoorR] Ignored interaction (already opened once)")
            end
            -- Important: return so it doesn't fall through into open logic
            -- but rotation continues below.
        else
            -- First time open/toggle:
            if cmd == "open" or cmd == "toggle" then
                isOpen = true
                hasOpenedOnce = true
                targetY = openAngle

                SHARED.DOOR_OPENED_ONCE = SHARED.DOOR_OPENED_ONCE or {}
                SHARED.DOOR_OPENED_ONCE[EntityID] = true

                -- Pick open sound based on door type
                if is_main_door() then
                    play_3d_sfx(MAINDOOR_OPEN_SFX_ID)
                else
                    play_3d_sfx(DOOR_OPEN_SFX_ID)
                end

                print(string.format(
                    "[DoorR] OPENED ONCE by=%s -> permanently disabled",
                    tostring(cmdObj.by)
                ))
            else
                print("[DoorR] Ignored cmd before first open:", tostring(cmd))
            end
        end
    end

    -- =========================
    -- rotation
    -- =========================
    local diff = targetY - currentY
    if math.abs(diff) > 0.05 then
        local dir = (diff > 0) and 1 or -1
        currentY = currentY + dir * rotateSpeed * dt

        if dir > 0 then
            if currentY > targetY then currentY = targetY end
        else
            if currentY < targetY then currentY = targetY end
        end

        local rx, _, rz = tr:GetRotation()
        tr:SetRotation(rx, currentY, rz)
    end
end

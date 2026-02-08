-- Door_SharedControlled.lua
-- Attach to Door entity
-- Opens ONCE, then permanently ignores all interaction

local closedAngle = 0.0
local openAngle   = -80.0
local rotateSpeed = 20.0

local isOpen = false
local currentY = 0.0
local targetY = 0.0

local hasOpenedOnce = false

-- Door audio IDs (slot IDs)
local DOOR_OPEN_SFX_ID      = "OpenDoor"       -- normal doors
local MAINDOOR_OPEN_SFX_ID  = "OpenMaindoor"   -- doors 26 / 509
local MAINDOOR_CLOSE_SFX_ID = "CloseMaindoor"       -- main door closing sound (set to your actual ID)
local GIRL_GIGGLE_SFX_ID    = "GirlGiggle"      -- set to your actual ID

local GIGGLE_DELAY_SECONDS = 2.0

-- Which entities are the “main doors”
local MAIN_DOORS = {
    [26]  = true,
    [509] = true
}

local gigglePending = false
local giggleTimer = 0.0

local function is_main_door()
    return MAIN_DOORS[EntityID] == true
end

local function play_3d_sfx(slotId)
    if type(Audio_PlaySlot) == "function" and slotId ~= nil then
        Audio_PlaySlot(EntityID, slotId) -- 3D from THIS door entity
        return true
    end
    return false
end

function on_create(Self)
    print("[Door] on_create EntityID:", EntityID)

    local tr = Query.GetTransform(EntityID)
    if tr == nil then
        print("[Door] ERROR: No Transform found")
        return
    end

    local _, ry, _ = tr:GetRotation()
    currentY = ry
    targetY = ry

    SHARED.DOOR_CMD = SHARED.DOOR_CMD or {}

    -- ✅ NEW: shared state table for icons / other scripts
    SHARED.DOOR_OPENED_ONCE = SHARED.DOOR_OPENED_ONCE or {}
    SHARED.DOOR_OPENED_ONCE[EntityID] = false

    print(string.format("[Door] Start RotY=%.2f", currentY))
end

function on_update(Self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end
    dt = dt or 0.016

    -- Handle delayed giggle (main door only)
    if gigglePending then
        giggleTimer = giggleTimer + dt
        if giggleTimer >= GIGGLE_DELAY_SECONDS then
            gigglePending = false
            giggleTimer = 0.0
            play_3d_sfx(GIRL_GIGGLE_SFX_ID)
        end
    end

    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil

        local cmd = cmdObj.cmd

        -- MAIN DOOR: allow scripted close even after opening once
        if is_main_door() and cmd == "close" then
            isOpen = false
            targetY = closedAngle

            -- play close SFX + schedule giggle
            play_3d_sfx(MAINDOOR_CLOSE_SFX_ID)
            gigglePending = true
            giggleTimer = 0.0

            print("[Door] MAIN DOOR scripted CLOSE -> close sound + giggle queued")
            -- do NOT return; still let rotation run below
        else
            -- All other doors: once opened, ignore everything forever
            if hasOpenedOnce then
                print("[Door] Ignored interaction (already opened once)")
                return
            end

            -- Only allow open/toggle for normal doors (and main doors too)
            if cmd == "open" or cmd == "toggle" then
                isOpen = true
                hasOpenedOnce = true
                targetY = openAngle

                -- publish opened-once state
                SHARED.DOOR_OPENED_ONCE = SHARED.DOOR_OPENED_ONCE or {}
                SHARED.DOOR_OPENED_ONCE[EntityID] = true

                -- play the correct open sound
                if is_main_door() then
                    play_3d_sfx(MAINDOOR_OPEN_SFX_ID)
                else
                    play_3d_sfx(DOOR_OPEN_SFX_ID)
                end

                print(string.format("[Door] OPENED ONCE by=%s -> permanently disabled", tostring(cmdObj.by)))
            else
                print("[Door] Ignored cmd:", tostring(cmd))
            end
        end
    end

    -- Rotation logic (unchanged)
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

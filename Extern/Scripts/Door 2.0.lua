-- Door_SharedControlled.lua
-- Attach to Door entity
-- This door is no longer player-interactable.
-- It only supports a scripted close command.

local closedAngle = 0.0
local rotateSpeed = 180.0

local currentY = 0.0
local targetY = 0.0

-- Door audio IDs
local MAINDOOR_CLOSE_SFX_ID = "CloseMaindoor"
local SCARY_SFX_ID    = "Scary"

local GIGGLE_DELAY_SECONDS = 0.0

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
        Audio_PlaySlot(EntityID, slotId)
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

    print(string.format("[Door] Start RotY=%.2f", currentY))
end

function on_update(Self, dt)
    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end
    dt = dt or 0.016

    -- Handle delayed giggle
    if gigglePending then
        giggleTimer = giggleTimer + dt
        if giggleTimer >= GIGGLE_DELAY_SECONDS then
            gigglePending = false
            giggleTimer = 0.0
            play_3d_sfx(SCARY_SFX_ID)
        end
    end

    -- Only accept scripted close command
    if SHARED.DOOR_CMD and SHARED.DOOR_CMD[EntityID] then
        local cmdObj = SHARED.DOOR_CMD[EntityID]
        SHARED.DOOR_CMD[EntityID] = nil

        local cmd = cmdObj.cmd

        if cmd == "close" then
            targetY = closedAngle

            if is_main_door() then
                play_3d_sfx(MAINDOOR_CLOSE_SFX_ID)
                gigglePending = true
                giggleTimer = 0.0
                print("[Door] Scripted CLOSE -> close sound + giggle queued")
            else
                print("[Door] Scripted CLOSE")
            end
        else
            print("[Door] Ignored cmd:", tostring(cmd))
        end
    end

    -- Rotation logic
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
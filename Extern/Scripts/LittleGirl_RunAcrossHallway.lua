----------------------------------------------------------------
-- LittleGirl_RunAcrossHallway.lua
-- Attach to the little girl running at hallway
--
-- Unhides girl(running) when:
--  - Disc placed in music box, music halfway
----------------------------------------------------------------
local DEBUG = true

local RUN_AUDIO_SLOT = "Running"

local runActive = false
local runTimer = 0.0
local runDuration = 2
local runSpeed = 700.0

local currentX = 0.0
local currentY = 0.0
local currentZ = 0.0

local function show_girl()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(1.0, 1.0, 1.0)
        return true
    end
    return false
end

local function hide_girl()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(0.0, 0.0, 0.0)
        return true
    end
    return false
end

function on_create(self)
    SHARED.LITTLE_GIRL_RUN = SHARED.LITTLE_GIRL_RUN or false

    local tr = Query.GetTransform(EntityID)
    if tr then
        currentX, currentY, currentZ = tr:GetPosition()
    end

    hide_girl()

    if DEBUG then
        print("[LittleGirl] on_create EntityID:", EntityID)
    end
end

function on_update(self, dt)
    dt = dt or 0.016

    local tr = Query.GetTransform(EntityID)
    if tr == nil then return end

    if (not runActive) and SHARED and SHARED.LITTLE_GIRL_RUN == true then
        SHARED.LITTLE_GIRL_RUN = false

        runActive = true
        runTimer = 0.0

        currentX, currentY, currentZ = tr:GetPosition()
        show_girl()

        -- Play running sound
        if type(Audio_PlaySlot) == "function" then
            Audio_StopSlot(EntityID, RUN_AUDIO_SLOT)
            Audio_PlaySlot(EntityID, RUN_AUDIO_SLOT)
        end

        if DEBUG then
            print("[LittleGirl] Run started")
        end
    end

    if not runActive then
        return
    end

    runTimer = runTimer + dt

    -- Move forward based on the girl's rotation
    local rx, ry, rz = tr:GetRotation()

    local radYaw = math.rad(ry)

    local fx = math.sin(radYaw)
    local fz = math.cos(radYaw)

    currentX = currentX + fx * runSpeed * dt
    currentZ = currentZ + fz * runSpeed * dt

    tr:SetPosition(currentX, currentY, currentZ)

    if runTimer >= runDuration then
        runActive = false

        -- Stop running sound
        if type(Audio_StopSlot) == "function" then
            Audio_StopSlot(EntityID, RUN_AUDIO_SLOT)
        end

        hide_girl()

        if DEBUG then
            print("[LittleGirl] Run ended, hidden again")
        end
    end
end
-- MusicBoxDisc_Show.lua
-- Attach to the disc model entity that sits on the music box

local DEBUG = true

local shown = false
local spinning = false

local spinTimer = 0.0
local MUSIC_DURATION = 30.0   -- seconds

local ROTATE_SPEED = 180.0    -- degrees per second

local currentY = 0.0

local GIRL_TRIGGER_TIME = 6.0
local girlTriggered = false

------------------------------------------------

local function show_disc()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(1.0, 1.0, 1.0)

        local rx, ry, rz = tr:GetRotation()
        currentY = ry

        return true
    end
    return false
end

local function hide_disc()
    local tr = Query.GetTransform(EntityID)
    if tr then
        tr:SetScale(0.0, 0.0, 0.0)
        return true
    end
    return false
end

------------------------------------------------

function on_create(self)
    SHARED.MUSICBOX_DISC_PLACED = false
    SHARED.LITTLE_GIRL_RUN = false

    shown = false
    spinning = false
    spinTimer = 0.0
    girlTriggered = false

    hide_disc()

    if DEBUG then
        print("[MusicBoxDisc_Show] on_create EntityID:", EntityID)
    end
end

------------------------------------------------

function on_update(self, dt)
    dt = dt or 0.016

    if not shown then
        if SHARED and SHARED.MUSICBOX_DISC_PLACED == true then
            local ok = show_disc()
            shown = ok
            spinning = ok
            spinTimer = 0.0
            girlTriggered = false

            if DEBUG then
                print("[MusicBoxDisc_Show] Disc shown and spinning started")
            end
        end
        return
    end

    if not spinning then
        return
    end

    spinTimer = spinTimer + dt

    local tr = Query.GetTransform(EntityID)
    if tr then
        currentY = currentY + ROTATE_SPEED * dt
        local rx, _, rz = tr:GetRotation()
        tr:SetRotation(rx, currentY, rz)
    end

    -- Trigger little girl once at 10 seconds
    if (not girlTriggered) and spinTimer >= GIRL_TRIGGER_TIME then
        SHARED.LITTLE_GIRL_RUN = true
        girlTriggered = true

        if DEBUG then
            print("[MusicBoxDisc_Show] Triggered little girl run at 10s")
        end
    end

    if spinTimer >= MUSIC_DURATION then
        spinning = false

        if DEBUG then
            print("[MusicBoxDisc_Show] Music ended, disc stopped")
        end
    end
end